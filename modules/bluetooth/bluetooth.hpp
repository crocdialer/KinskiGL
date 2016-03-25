#pragma once

#include "core/core.hpp"

namespace kinski{ namespace bluetooth{
    
    struct UUID;
    class Central;
    struct Peripheral;
    
    typedef std::function<void(Peripheral&, const UUID&, float)>
    PeripheralDiscoveredCallback;
    
    class Central
    {
    public:
        
        Central();
        void scan_for_peripherals();
        void connect_peripheral(const Peripheral &the_peripheral);
        void set_peripheral_discovered_cb(PeripheralDiscoveredCallback cb);
        
    private:
        std::shared_ptr<struct CentralImpl> m_impl;
    };
    
    struct UUID
    {
        char data[16];
    };
    
    struct Peripheral
    {
        UUID uuid;
        std::string name;
    };
    
}}//namespace