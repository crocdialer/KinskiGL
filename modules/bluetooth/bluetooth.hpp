#pragma once

#include "core/core.hpp"
#include "UUID.hpp"

namespace kinski{ namespace bluetooth
{

    typedef std::shared_ptr<class Central> CentralPtr;
    typedef std::shared_ptr<class Peripheral> PeripheralPtr;
    typedef std::function<void(CentralPtr, PeripheralPtr)> PeripheralCallback;

    class Central : public std::enable_shared_from_this<Central>
    {
    public:

        static CentralPtr create();

        void discover_peripherals(const std::set<UUID>& the_service_uuids = {});
        void stop_scanning();

        void connect_peripheral(const PeripheralPtr &the_peripheral,
                                PeripheralCallback cb = PeripheralCallback());
        void disconnect_peripheral(const PeripheralPtr &the_peripheral);
        void disconnect_all();
        void set_peripheral_discovered_cb(PeripheralCallback cb);
        void set_peripheral_connected_cb(PeripheralCallback cb);
        void set_peripheral_disconnected_cb(PeripheralCallback cb);

        std::set<PeripheralPtr> peripherals() const;

    private:
        std::shared_ptr<struct CentralImpl> m_impl;
        Central();
    };

    class Peripheral : public std::enable_shared_from_this<Peripheral>
    {
    public:

        typedef std::function<void(const UUID&, const std::vector<uint8_t>&)> ValueUpdatedCallback;

        const std::string& name() const;
        void set_name(const std::string &the_name);

        bool is_connected() const;
        // void set_connected(bool b);

        bool connectable() const;
        void set_connectable(bool b);

        float rssi() const;
        void set_rssi(float the_rssi);

        void discover_services(const std::set<UUID>& the_uuids = {});

        void write_value_for_characteristic(const UUID &the_characteristic,
                                            const std::vector<uint8_t> &the_data);

        void read_value_for_characteristic(const UUID &the_characteristic,
                                           ValueUpdatedCallback cb);

        ValueUpdatedCallback value_updated_cb();
        void set_value_updated_cb(ValueUpdatedCallback cb);

        void add_service(const UUID& the_service_uuid);
        void add_characteristic(const UUID& the_service_uuid, const UUID& the_characteristic_uuid);

        const std::map<UUID, std::set<UUID>>& known_services();

    private:
        friend CentralImpl;
        Peripheral();
        std::shared_ptr<struct PeripheralImpl> m_impl;
    };

    enum CharacteristicProperty
    {
        CharacteristicPropertyBroadcast = 0x01,
    	CharacteristicPropertyRead = 0x02,
    	CharacteristicPropertyWriteWithoutResponse = 0x04,
    	CharacteristicPropertyWrite = 0x08,
    	CharacteristicPropertyNotify = 0x10,
    	CharacteristicPropertyIndicate= 0x20,
    	CharacteristicPropertyAuthenticatedSignedWrites = 0x40,
    	CharacteristicPropertyExtendedProperties = 0x80
    };

}}//namespace
