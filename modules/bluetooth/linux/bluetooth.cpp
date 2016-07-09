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

// advertising event types
#define ADV_IND                    0x00 // Connectable undirected advertising
#define ADV_DIRECT_IND             0x01 // Connectable high duty cycle directed advertising
#define ADV_SCAN_IND               0x02 // Scannable undirected advertising
#define ADV_NONCONN_IND            0x03 // Non connectable undirected advertising
#define ADV_DIRECT_IND_LOW         0x04 // Connectable low duty cycle directed advertising

// advertising data types
#define AD_FLAGS                   0X01
#define AD_SERVICES_16             0x02 // Incomplete List of 16-bit Service Class UUID
#define AD_SERVICES_16_COMPLETE    0x03	// Complete List of 16-bit Service Class UUIDs
#define AD_SERVICES_32             0x04	// Incomplete List of 32-bit Service Class UUIDs
#define AD_SERVICES_32_COMPLETE    0x05 // Complete List of 32-bit Service Class UUIDs
#define AD_SERVICES_128            0x06 // Incomplete List of 128-bit Services
#define AD_SERVICES_128_COMPLETE   0x07 // Complete List of 128-bit Services
#define AD_NAME_SHORT              0x08
#define AD_NAME_COMPLETE           0x09
#define AD_TX_LEVEL                0x0A	// Tx Power Level
#define AD_MANUFACTURE_SPECIFIC    0xFF

struct hci_state
{
    int device_id = 0;
    int device_handle = 0;
    struct hci_dev_info device_info;
    struct hci_filter original_filter;
    int state = 0;
    int has_error = 0;
};

// TODO: maps bdaddr_t <-> PeripheralPtr

hci_state hci_open_default_device();
void hci_start_scan(hci_state &the_hci_state);
void hci_stop_scan(hci_state &the_hci_state);

///////////////////////////////////////////////////////////////////////////////////////////

inline bool is_connectable(uint8_t the_ad_event_type)
{
    switch(the_ad_event_type)
    {
        case ADV_IND:
        case ADV_DIRECT_IND:
        case ADV_DIRECT_IND_LOW:
            return true;
    }
    return false;
}

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
        LOG_ERROR << "could not set device to non-blocking: " << strerror(errno);
    }
    if(!ret.has_error){ ret.state = HCI_STATE_OPEN; }

    if(hci_devinfo(ret.device_id, &ret.device_info) < 0)
    {
        ret.has_error = true;
        LOG_ERROR << "could not get device info";
    }
    return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////

void hci_start_scan(hci_state &the_hci_state)
{
    int err;
    uint8_t own_type = LE_PUBLIC_ADDRESS;
	uint8_t scan_type = 0x01;
	uint8_t filter_policy = 0x00;
	uint16_t interval = htobs(0x0010);
	uint16_t window = htobs(0x0010);
	// uint8_t filter_dup = 0x01;
    // uint8_t filter_type = 0;

    err = hci_le_set_scan_parameters(the_hci_state.device_handle, scan_type, interval, window,
						              own_type, filter_policy, 1000);

    if(err < 0)
    {
        the_hci_state.has_error = true;
        LOG_ERROR << "failed to set scan parameters: " << strerror(errno);
        return;
    }
    err = hci_le_set_scan_enable(the_hci_state.device_handle, 0x01, 1, 1000);

    if(err < 0)
    {
        the_hci_state.has_error = true;
        LOG_ERROR << "failed to enable scan: " << strerror(errno);
        return;
    }

    the_hci_state.state = HCI_STATE_SCANNING;

    socklen_t olen = sizeof(the_hci_state.original_filter);

    err = getsockopt(the_hci_state.device_handle, SOL_HCI, HCI_FILTER,
                     &the_hci_state.original_filter, &olen);
    if(err < 0)
    {
        the_hci_state.has_error = true;
        LOG_ERROR << "could not get socket options: " << strerror(errno);
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
        LOG_ERROR << "could not set socket options: " << strerror(errno);
        return;
    }
    the_hci_state.state = HCI_STATE_FILTERING;
}

///////////////////////////////////////////////////////////////////////////////////////////

void hci_stop_scan(hci_state &the_hci_state)
{
    int result;

    if(the_hci_state.state == HCI_STATE_FILTERING)
    {
        result = setsockopt(the_hci_state.device_handle, SOL_HCI, HCI_FILTER,
                            &the_hci_state.original_filter, sizeof(the_hci_state.original_filter));
        if(result < 0)
        {
            the_hci_state.has_error = true;
            LOG_ERROR << "Could not set socket options: " << strerror(errno);
            return;
        }
        the_hci_state.state = HCI_STATE_SCANNING;
    }

    result = hci_le_set_scan_enable(the_hci_state.device_handle, 0x00, 1, 1000);
    
    if(result < 0)
    {
        the_hci_state.has_error = true;
        LOG_ERROR << "Disable scan failed: " << strerror(errno);
    }
    the_hci_state.state = HCI_STATE_OPEN;
}

// void connect(bdaddr_t *bdaddr, const hci_state& the_hci_state)
// {
//     uint16_t handle;
//     // int hci_create_connection(int dd, const bdaddr_t *bdaddr, uint16_t ptype, uint16_t clkoffset, uint8_t rswitch, uint16_t *handle, int to);
//     // HCI_DM1 | HCI_DM3 | HCI_DM5 | HCI_DH1 | HCI_DH3 | HCI_DH5
//
//     if(hci_create_connection(the_hci_state.device_handle, bdaddr,
//        htobs(the_hci_state.device_info.pkt_type & ACL_PTYPE_MASK), (1 << 15), 0, &handle, 5000) < 0)
//     {
//         LOG_ERROR << "could not create connection";
//     }
//     sleep(1);
//
//     struct hci_conn_info_req *cr = (hci_conn_info_req*)malloc(sizeof(*cr) + sizeof(struct hci_conn_info));
//     bacpy(&cr->bdaddr, bdaddr);
//     cr->type = ACL_LINK;
//
//     if(ioctl(the_hci_state.device_handle, HCIGETCONNINFO, (unsigned long) cr) < 0)
//     {
//         LOG_ERROR << "get connection info failed";
//     }
//
//     int8_t rssi = -1;
//
//     // if(hci_read_rssi(the_hci_state.device_handle, htobs(cr->conn_info->handle), &rssi, 1000) < 0)
//     // {
//     //     LOG_ERROR << "Read RSSI failed";
//     // }
//     free(cr);
//
//     usleep(10000);
//     hci_disconnect(the_hci_state.device_handle, handle, HCI_OE_USER_ENDED_CONNECTION, 10000);
// }

///////////////////////////////////////////////////////////////////////////////////////////

struct CentralImpl
{
    std::weak_ptr<Central> central_ref;
    hci_state m_hci_state;
    bool m_running = false;
    std::thread worker_thread;
    PeripheralCallback
    peripheral_discovered_cb, peripheral_connected_cb, peripheral_disconnected_cb;

    std::set<UUID> m_uuid_set;

    CentralImpl()
    {
        m_hci_state = hci_open_default_device();
    }

    ~CentralImpl()
    {
        m_running = false;

        // stop scanning for peripherals
        hci_stop_scan(m_hci_state);

        if(worker_thread.joinable())
        {
            try{ worker_thread.join(); }
            catch(std::exception &e){ LOG_WARNING << e.what(); }
        }

        if(m_hci_state.state == HCI_STATE_OPEN)
        {
            hci_close_dev(m_hci_state.device_handle);
        }
        LOG_DEBUG << "CentralImpl desctructor";
    }

    void process_data(uint8_t *data, size_t data_len, le_advertising_info *info)
    {
        // LOG_DEBUG << "connectable: " << is_connectable(info->evt_type);

        uint8_t data_type = data[0];

        char addr[18];
        ba2str(&info->bdaddr, addr);
        bool emit_event = false;

        // last octet contains RSSI as int8_t
        int8_t peripheral_rssi = info->data[info->length];

        string peripheral_name = "(unknown)";
        std::set<UUID> service_uuids;

        switch(info->evt_type)
        {
            case HCI_EVENT_PKT:
                emit_event = true;

            case HCI_SCODATA_PKT:
            case HCI_ACLDATA_PKT:
            case HCI_VENDOR_PKT:
            case HCI_COMMAND_PKT:
            default:
                break;
        }

        if(data_type == AD_NAME_SHORT || data_type == AD_NAME_COMPLETE)
        {
            size_t name_len = data_len - 1;
            char *name = (char*)malloc(name_len + 1);
            memset(name, 0, name_len + 1);
            memcpy(name, &data[1], name_len);
            peripheral_name = name;
            free(name);
            // emit_event = true;
        }
        else if(data_type == AD_SERVICES_128 || data_type == AD_SERVICES_128_COMPLETE)
        {
            uint8_t bytes[16];
            swap_endian(bytes, &data[1], 16);
            UUID service_uuid(bytes, UUID::UUID_128);
            service_uuids.insert(service_uuid);
        }
        else if(data_type == AD_SERVICES_16 || data_type == AD_SERVICES_16_COMPLETE)
        {
            uint8_t bytes[2];
            swap_endian(bytes, &data[1], 2);
            UUID service_uuid(bytes, UUID::UUID_16);
            service_uuids.insert(service_uuid);
        }
        else if(data_type == AD_FLAGS)
        {
            for(size_t i = 1; i < data_len; i++)
            {
                LOG_TRACE << "\tflag data: " << (int)*((int8_t*)(data + i));
            }
        }
        else if(data_type == AD_MANUFACTURE_SPECIFIC)
        {
            LOG_TRACE << "Manufacture specific type: len:" << data_len;

            for(size_t i = 1; i < data_len; i++)
            {
                LOG_TRACE << "\tdata: " << std::setfill('0') << std::setw(2) << std::hex << (int)data[i];
            }
        }
        else if(data_type == AD_TX_LEVEL)
        {
            LOG_TRACE << "AD_TX_LEVEL: " << (int)data[1];
        }
        else
        {
             LOG_DEBUG << "event: " << (int)info->evt_type << " -- type: " << (int)data[0] << " -- length: " << data_len;
        }
        // LOG_DEBUG << "address: " << addr << " name: " << peripheral_name;

        auto central = central_ref.lock();
        auto peripheral = Peripheral::create(central);
        peripheral->set_name(peripheral_name);
        peripheral->set_rssi(peripheral_rssi);
        peripheral->set_connectable(is_connectable(info->evt_type));

        bool filter_service = !m_uuid_set.empty();

        for(const auto &s : service_uuids)
        {
             peripheral->add_service(s);
             if(is_in(s, m_uuid_set)){ filter_service = false; }
        }

        // fire discovery callback
        if(emit_event && central && peripheral && peripheral_discovered_cb && !filter_service)
        {
            peripheral_discovered_cb(central, peripheral);
        }
    }

    void thread_func()
    {
        m_running = true;
        uint8_t buf[HCI_MAX_EVENT_SIZE];
	    evt_le_meta_event * meta_event;
	    le_advertising_info * info;
	    int len;

        while(m_running)
        {
            len = read(m_hci_state.device_handle, buf, sizeof(buf));

            if(len >= HCI_EVENT_HDR_SIZE)
            {
		        meta_event = (evt_le_meta_event*)(buf + HCI_EVENT_HDR_SIZE + 1);

		        if(meta_event->subevent == EVT_LE_ADVERTISING_REPORT)
                {
                    uint8_t* meta_data = meta_event->data;
                    int reports_count = meta_data[0];
			        void* offset = meta_data + 1;

                    LOG_TRACE << "received " << reports_count << " ad-reports";

                    // iterate advertising reports
                    while(reports_count--)
                    {
					    info = (le_advertising_info *)offset;
					    char addr[18];
					    ba2str(&(info->bdaddr), addr);

                        if(info->length == 0){ continue; }

                        int current_index = 0;
                        int data_error = 0;

                        // iterate advertising data packets
                        while(!data_error && current_index < info->length)
                        {
                            uint8_t* data = info->data;
                            size_t data_len = data[current_index];

                            if(data_len + 1 > info->length)
                            {
                                LOG_ERROR << "AD data length is longer than AD packet length.";
                                data_error = 1;
                            }
                            else
                            {
                                process_data(data + current_index + 1, data_len, info);
                                current_index += data_len + 1;
                            }
                        }
					    offset = info->data + info->length + 2;
				    }
		        }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }//while(m_impl->m_running && !error)
        LOG_DEBUG << "thread ended";
    }
};

///////////////////////////////////////////////////////////////////////////////////////////

CentralPtr Central::create()
{
    CentralPtr ret(new Central());
    ret->m_impl->central_ref = ret;
    return ret;
}

Central::Central():
m_impl(new CentralImpl){ }

void Central::discover_peripherals(std::set<UUID> the_service_uuids)
{
    m_impl->m_uuid_set = the_service_uuids;
    hci_stop_scan(m_impl->m_hci_state);

    LOG_DEBUG << "discover_peripherals";
    hci_start_scan(m_impl->m_hci_state);

    m_impl->worker_thread = std::thread(std::bind(&CentralImpl::thread_func, m_impl.get()));
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

PeripheralPtr Peripheral::create(CentralPtr the_central)
{
    auto ret = PeripheralPtr(new Peripheral);
    ret->m_impl->central_ref = the_central;
    return ret;
}

Peripheral::Peripheral():m_impl(new PeripheralImpl()){}

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
