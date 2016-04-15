#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <thread>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include "bluetooth.hpp"

namespace kinski{ namespace bluetooth{

#define HCI_STATE_NONE       0
#define HCI_STATE_OPEN       2
#define HCI_STATE_SCANNING   3
#define HCI_STATE_FILTERING  4

struct hci_state
{
    int device_id = 0;
    int device_handle = 0;
    struct hci_filter original_filter;
    int state = 0;
    int has_error = 0;
    // std::string error_message;
};

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
        the_hci_state.state = HCI_STATE_SCANNING;
        setsockopt(the_hci_state.device_handle, SOL_HCI, HCI_FILTER, &the_hci_state.original_filter,
                   sizeof(the_hci_state.original_filter));
    }

    if(hci_le_set_scan_enable(the_hci_state.device_handle, 0x00, 1, 1000) < 0)
    {
        the_hci_state.has_error = true;
        LOG_ERROR << "Disable scan failed: " << strerror(errno);
    }
    the_hci_state.state = HCI_STATE_OPEN;
}

///////////////////////////////////////////////////////////////////////////////////////////

struct CentralImpl
{
    hci_state m_hci_state;

    std::thread discover_peripherals_thread;
    PeripheralCallback
    peripheral_discovered_cb, peripheral_connected_cb, peripheral_disconnected_cb;

    CentralImpl()
    {
        m_hci_state = hci_open_default_device();
    }

    ~CentralImpl()
    {
        if(discover_peripherals_thread.joinable())
        {
            try{ discover_peripherals_thread.join(); }
            catch(std::exception &e){ LOG_WARNING << e.what(); }
        }

        // stop scanning for peripherals
        hci_stop_scan(m_hci_state);

        if(m_hci_state.state == HCI_STATE_OPEN)
        {
            hci_close_dev(m_hci_state.device_handle);
        }
    }
};

CentralPtr Central::create(){ return CentralPtr(new Central()); }

Central::Central():
m_impl(new CentralImpl){ }

void Central::discover_peripherals(std::set<UUID> the_service_uuids)
{
    LOG_DEBUG << "discover_peripherals";
    hci_start_scan(m_impl->m_hci_state);

    // m_impl->discover_peripherals_thread = std::thread([this]()
    // {
    //
    // });
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
