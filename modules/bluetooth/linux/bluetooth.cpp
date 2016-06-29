#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <thread>
#include <chrono>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include "bluetooth.hpp"

namespace kinski{ namespace bluetooth{

#define HCI_STATE_NONE       0
#define HCI_STATE_OPEN       2
#define HCI_STATE_SCANNING   3
#define HCI_STATE_FILTERING  4

#define EIR_FLAGS                   0X01
#define EIR_NAME_SHORT              0x08
#define EIR_NAME_COMPLETE           0x09
#define EIR_MANUFACTURE_SPECIFIC    0xFF

struct hci_state
{
    int device_id = 0;
    int device_handle = 0;
    struct hci_filter original_filter;
    int state = 0;
    int has_error = 0;
    // std::string error_message;
};

hci_state hci_open_default_device();
void hci_start_scan(hci_state &the_hci_state);
void hci_stop_scan(hci_state &the_hci_state);
void process_data(uint8_t *data, size_t data_len, le_advertising_info *info);


///////////////////////////////////////////////////////////////////////////////////////////

hci_state hci_open_default_device()
{
    hci_state ret;
    ret.device_id = hci_get_route(nullptr);
    ret.device_handle = hci_open_dev(ret.device_id);

    if(ret.device_id < 0 || ret.device_handle < 0)
    {
        LOG_ERROR << "could not open bluetooth device";
        ret.has_error = true;
    }
    int on = 1;

    if(ioctl(ret.device_handle, FIONBIO, (char *)&on) < 0)
    {
        ret.has_error = true;
        LOG_ERROR << "Could not set device to non-blocking: " << strerror(errno);
    }
    if(!ret.has_error){ ret.state = HCI_STATE_OPEN; }
    return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////

void hci_start_scan(hci_state &the_hci_state)
{
    if(hci_le_set_scan_parameters(the_hci_state.device_handle, 0x01, htobs(0x0010),
                                  htobs(0x0010), 0x00, 0x00, 1000) < 0)
    {
        the_hci_state.has_error = true;
        LOG_ERROR << "Failed to set scan parameters: " << strerror(errno);
        return;
    }

    if(hci_le_set_scan_enable(the_hci_state.device_handle, 0x01, 1, 1000) < 0)
    {
        the_hci_state.has_error = true;
        LOG_ERROR << "Failed to enable scan: " << strerror(errno);
        return;
    }

    the_hci_state.state = HCI_STATE_SCANNING;

    socklen_t olen = sizeof(the_hci_state.original_filter);

    if(getsockopt(the_hci_state.device_handle, SOL_HCI, HCI_FILTER,
                  &the_hci_state.original_filter, &olen) < 0)
    {
        the_hci_state.has_error = true;
        LOG_ERROR << "Could not get socket options: " << strerror(errno);
        return;
    }

    // Create and set the new filter
    struct hci_filter new_filter;

    hci_filter_clear(&new_filter);
    hci_filter_set_ptype(HCI_EVENT_PKT, &new_filter);
    hci_filter_set_event(EVT_LE_META_EVENT, &new_filter);

    if(setsockopt(the_hci_state.device_handle, SOL_HCI, HCI_FILTER, &new_filter,
                  sizeof(new_filter)) < 0)
    {
        the_hci_state.has_error = true;
        LOG_ERROR << "Could not set socket options: " << strerror(errno);
        return;
    }
    the_hci_state.state = HCI_STATE_FILTERING;
}

///////////////////////////////////////////////////////////////////////////////////////////

void hci_stop_scan(hci_state &the_hci_state)
{
    if(the_hci_state.state == HCI_STATE_FILTERING)
    {
        if(setsockopt(the_hci_state.device_handle, SOL_HCI, HCI_FILTER, &the_hci_state.original_filter,
                   sizeof(the_hci_state.original_filter)) < 0)
        {
            the_hci_state.has_error = true;
            LOG_ERROR << "Could not set socket options: " << strerror(errno);
            return;
        }
        the_hci_state.state = HCI_STATE_SCANNING;
    }

    if(hci_le_set_scan_enable(the_hci_state.device_handle, 0x00, 1, 1000) < 0)
    {
        the_hci_state.has_error = true;
        LOG_ERROR << "Disable scan failed: " << strerror(errno);
    }
    the_hci_state.state = HCI_STATE_OPEN;
}

int8_t get_rssi(bdaddr_t *bdaddr, struct hci_state current_hci_state)
{
    struct hci_dev_info di;
    if(hci_devinfo(current_hci_state.device_id, &di) < 0)
    {
        LOG_ERROR << "Can't get device info";
        return(-1);
    }

    uint16_t handle;
    // int hci_create_connection(int dd, const bdaddr_t *bdaddr, uint16_t ptype, uint16_t clkoffset, uint8_t rswitch, uint16_t *handle, int to);
    // HCI_DM1 | HCI_DM3 | HCI_DM5 | HCI_DH1 | HCI_DH3 | HCI_DH5
    if(hci_create_connection(current_hci_state.device_handle, bdaddr,
       htobs(di.pkt_type & ACL_PTYPE_MASK), 0, 0x01, &handle, 25000) < 0)
    {
        LOG_ERROR << "Can't create connection";
        // TODO close(dd);
        return(-1);
    }
    sleep(1);

    struct hci_conn_info_req *cr = (hci_conn_info_req*)malloc(sizeof(*cr) + sizeof(struct hci_conn_info));
    bacpy(&cr->bdaddr, bdaddr);
    cr->type = ACL_LINK;
    if(ioctl(current_hci_state.device_handle, HCIGETCONNINFO, (unsigned long) cr) < 0)
    {
        LOG_ERROR << "Get connection info failed";
        return(-1);
    }

    int8_t rssi;
    if(hci_read_rssi(current_hci_state.device_handle, htobs(cr->conn_info->handle), &rssi, 1000) < 0)
    {
        LOG_ERROR << "Read RSSI failed";
        return(-1);
    }
    free(cr);

    usleep(10000);
    hci_disconnect(current_hci_state.device_handle, handle, HCI_OE_USER_ENDED_CONNECTION, 10000);
    LOG_DEBUG << "RSSI return value: " << rssi;
    return rssi;
}

void process_data(uint8_t *data, size_t data_len, le_advertising_info *info)
{
    LOG_DEBUG << "Test: " << data << " and: " << data_len;

    if(data[0] == EIR_NAME_SHORT || data[0] == EIR_NAME_COMPLETE)
    {
        size_t name_len = data_len - 1;
        char *name = (char*)malloc(name_len + 1);
        memset(name, 0, name_len + 1);
        memcpy(name, &data[2], name_len);

        char addr[18];
        ba2str(&info->bdaddr, addr);

        LOG_DEBUG << "address: " << addr << " name: " << name;

        free(name);
    }
    else if(data[0] == EIR_FLAGS)
    {
        LOG_DEBUG << "Flag type: len: " << data_len;

        for(size_t i = 1; i < data_len; i++)
        {
            LOG_DEBUG << "\tFlag data: " << std::hex << data[i];
        }
    }
    else if(data[0] == EIR_MANUFACTURE_SPECIFIC)
    {
        LOG_DEBUG << "Manufacture specific type: len:" << data_len;

        // TODO: int company_id = data[current_index + 2]

        for(size_t i = 1; i < data_len; i++)
        {
            LOG_DEBUG << "\tData: " << std::hex << data[i];
        }
    }
    else{ LOG_DEBUG << "Unknown type: " << std::hex << data[0]; }
}

///////////////////////////////////////////////////////////////////////////////////////////

struct CentralImpl
{
    hci_state m_hci_state;
    bool running = false;
    std::thread worker_thread;
    PeripheralCallback
    peripheral_discovered_cb, peripheral_connected_cb, peripheral_disconnected_cb;

    CentralImpl()
    {
        m_hci_state = hci_open_default_device();
    }

    ~CentralImpl()
    {
        running = false;

        if(worker_thread.joinable())
        {
            try{ worker_thread.join(); }
            catch(std::exception &e){ LOG_WARNING << e.what(); }
        }

        // stop scanning for peripherals
        hci_stop_scan(m_hci_state);

        if(m_hci_state.state == HCI_STATE_OPEN)
        {
            hci_close_dev(m_hci_state.device_handle);
        }
        LOG_DEBUG << "CentralImpl desctructor";
    }
};

CentralPtr Central::create(){ return CentralPtr(new Central()); }

Central::Central():
m_impl(new CentralImpl){ }

void Central::discover_peripherals(std::set<UUID> the_service_uuids)
{
    hci_stop_scan(m_impl->m_hci_state);

    LOG_DEBUG << "discover_peripherals";
    hci_start_scan(m_impl->m_hci_state);

    m_impl->worker_thread = std::thread([this]()
    {
        m_impl->running = true;
        uint8_t buf[HCI_MAX_EVENT_SIZE];
	    evt_le_meta_event * meta_event;
	    le_advertising_info * info;
	    int len;

        while(m_impl->running)
        {
            len = read(m_impl->m_hci_state.device_handle, buf, sizeof(buf));

            if(len >= HCI_EVENT_HDR_SIZE)
            {
		        meta_event = (evt_le_meta_event*)(buf + HCI_EVENT_HDR_SIZE + 1);

		        if(meta_event->subevent == EVT_LE_ADVERTISING_REPORT)
                {
			        // uint8_t reports_count = *meta_event->data;
			        void * offset = meta_event->data + 1;
			        // while(reports_count--)
                    {
				        info = (le_advertising_info *)offset;
				        char addr[18];
				        ba2str(&(info->bdaddr), addr);

                        LOG_DEBUG << "Event: " << (int)info->evt_type;
                        LOG_DEBUG << "Length: " << (int)info->length;

                        if(info->length == 0){ continue; }

                        int current_index = 0;
                        int data_error = 0;

                        while(!data_error && current_index < info->length)
                        {
                            size_t data_len = info->data[current_index];

                            if(data_len + 1 > info->length)
                            {
                                LOG_ERROR << "EIR data length is longer than EIR packet length.";//%d + 1 > %d", data_len, info->length);
                                data_error = 1;
                            }
                            else
                            {
                                process_data(info->data + current_index + 1, data_len, info);
                                get_rssi(&info->bdaddr, m_impl->m_hci_state);
                                current_index += data_len + 1;
                            }
                        }
				        LOG_DEBUG << addr << " - RSSI: ?";// << (char)info->data[info->length];
				        offset = info->data + info->length + 2;
			        }
		        }
            }
        }//while(m_impl->running && !error)
        LOG_DEBUG << "thread ended";
    });
}

void Central::stop_scanning()
{
    hci_stop_scan(m_impl->m_hci_state);
}

void Central::connect_peripheral(const PeripheralPtr &p, PeripheralCallback cb)
{
    // connect peripheral
    if(p->connectable())
    {
        LOG_DEBUG << "connecting: " << p->name();
    }
}

void Central::disconnect_peripheral(const PeripheralPtr &the_peripheral)
{
    LOG_DEBUG << "disconnecting: " << the_peripheral->name();
}

void Central::disconnect_all()
{

}

std::set<PeripheralPtr> Central::peripherals() const
{
    std::set<PeripheralPtr> ret;
    return ret;
}

void Central::set_peripheral_discovered_cb(PeripheralCallback cb)
{
    m_impl->peripheral_discovered_cb = cb;
}

void Central::set_peripheral_connected_cb(PeripheralCallback cb)
{
    m_impl->peripheral_connected_cb = cb;
}

void Central::set_peripheral_disconnected_cb(PeripheralCallback cb)
{
    m_impl->peripheral_disconnected_cb = cb;
}

///////////////////////////////////////////////////////////////////////////////////////////

struct PeripheralImpl
{
    std::weak_ptr<Central> central_ref;
    UUID uuid;
    std::string name = "unknown";
    bool connectable = false;
    bool connected = false;
    float rssi;
    std::map<UUID, std::list<UUID>> known_services;
    Peripheral::ValueUpdatedCallback value_updated_cb;
};

PeripheralPtr Peripheral::create(CentralPtr the_central, UUID the_uuid)
{
    auto ret = PeripheralPtr(new Peripheral);
    ret->m_impl->uuid = the_uuid;
    ret->m_impl->central_ref = the_central;
    return ret;
}

Peripheral::Peripheral():m_impl(new PeripheralImpl()){}

const UUID& Peripheral::uuid() const{ return m_impl->uuid; }

const std::string& Peripheral::name() const{ return m_impl->name; }

void Peripheral::set_name(const std::string &the_name){ m_impl->name = the_name; }

bool Peripheral::is_connected() const{ return m_impl->connected; }
void Peripheral::set_connected(bool b){ m_impl->connected = b; }

bool Peripheral::connectable() const { return m_impl->connectable; }

void Peripheral::set_connectable(bool b){ m_impl->connectable = b; }

float Peripheral::rssi() const { return m_impl->rssi; }

void Peripheral::set_rssi(float the_rssi){ m_impl->rssi = the_rssi; }

void Peripheral::discover_services(std::set<UUID> the_uuids)
{
    if(!is_connected())
    {
        LOG_WARNING << "could not discover services, peripheral not connected";
        return;
    }
}

void Peripheral::write_value_for_characteristic(const UUID &the_characteristic,
                                                const std::vector<uint8_t> &the_data)
{

}

void Peripheral::read_value_for_characteristic(const UUID &the_characteristic,
                                               Peripheral::ValueUpdatedCallback cb)
{

}

void Peripheral::add_service(const UUID& the_service_uuid)
{
    auto it = m_impl->known_services.find(the_service_uuid);

    if(it == m_impl->known_services.end())
    {
        m_impl->known_services[the_service_uuid] = {};
    }
}

void Peripheral::add_characteristic(const UUID& the_service_uuid,
                                    const UUID& the_characteristic_uuid)
{
    add_service(the_service_uuid);
    m_impl->known_services[the_service_uuid].push_back(the_characteristic_uuid);
}

const std::map<UUID, std::list<UUID>>& Peripheral::known_services()
{
    return m_impl->known_services;
}

Peripheral::ValueUpdatedCallback Peripheral::value_updated_cb(){ return m_impl->value_updated_cb; }

void Peripheral::set_value_updated_cb(ValueUpdatedCallback cb){ m_impl->value_updated_cb = cb; }

}}// namespaces
