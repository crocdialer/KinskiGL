#pragma once

#include "core/core.hpp"

namespace kinski{ namespace bluetooth
{
    
    typedef std::shared_ptr<class Central> CentralPtr;
    typedef std::shared_ptr<class Peripheral> PeripheralPtr;
    typedef std::function<void(CentralPtr, PeripheralPtr)> PeripheralCallback;
    
    class UUID
    {
    public:
        UUID();
        UUID(const std::string &the_str);
        explicit UUID(uint8_t the_bytes[16]);
        
        const uint8_t* as_bytes() const;
        const std::string as_string() const;
        
        bool operator==(const UUID &the_other) const;
        bool operator!=(const UUID &the_other) const;
        bool operator<(const UUID &the_other) const;
        
    private:
        uint8_t m_data[16];
    };
    
    class Central : public std::enable_shared_from_this<Central>
    {
    public:
        
        static CentralPtr create();
        
        void discover_peripherals(std::set<UUID> the_service_uuids = {});
        void stop_scanning();
        
        void connect_peripheral(const PeripheralPtr &the_peripheral,
                                PeripheralCallback cb = PeripheralCallback());
        void disconnect_peripheral(const PeripheralPtr &the_peripheral);
        
        void set_peripheral_discovered_cb(PeripheralCallback cb);
        void set_peripheral_connected_cb(PeripheralCallback cb);
        void set_peripheral_disconnected_cb(PeripheralCallback cb);
        
        std::set<PeripheralPtr> peripherals() const;
        
    private:
        Central();
        std::unique_ptr<struct CentralImpl> m_impl;
    };
    
    class Peripheral : public std::enable_shared_from_this<Peripheral>
    {
    public:
        
        typedef std::function<void(const UUID&, const std::vector<uint8_t>&)> ValueUpdatedCallback;
        
        static PeripheralPtr create(CentralPtr the_central, uint8_t the_uuid[16]);
        
        std::string uuid() const;
        std::string name() const;
        void set_name(const std::string &the_name);
        
        bool is_connected() const;
        void set_connected(bool b);
        
        bool connectable() const;
        void set_connectable(bool b);
        
        float rssi() const;
        void set_rssi(float the_rssi);
        
        void discover_services(std::set<UUID> the_uuids = {});
        
        void write_value_for_characteristic(const UUID &the_characteristic,
                                            const std::vector<uint8_t> &the_data);
        
        void read_value_for_characteristic(const UUID &the_characteristic,
                                           ValueUpdatedCallback cb);
        
        void set_value_updated_cb(ValueUpdatedCallback cb);
        
//        std::map<Service, std::list<Characteristic>> characteristics_map();
        
    private:
        Peripheral();
        std::unique_ptr<struct PeripheralImpl> m_impl;
    };
    
}}//namespace