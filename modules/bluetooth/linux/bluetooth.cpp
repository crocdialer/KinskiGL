#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include "bluetooth.hpp"

namespace kinski{ namespace bluetooth{

///////////////////////////////////////////////////////////////////////////////////////////

struct CentralImpl
{
    PeripheralCallback
    peripheral_discovered_cb, peripheral_connected_cb, peripheral_disconnected_cb;

    CentralImpl()
    {

    }
    ~CentralImpl()
    {

    }
};

CentralPtr Central::create(){ return CentralPtr(new Central()); }

Central::Central():
m_impl(new CentralImpl){ }

void Central::discover_peripherals(std::set<UUID> the_service_uuids)
{
    LOG_DEBUG << "discover_peripherals";
}

void Central::stop_scanning()
{

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
