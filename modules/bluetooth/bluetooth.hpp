#pragma once

#include "core/core.hpp"

namespace kinski{ namespace bluetooth{
    
    typedef std::shared_ptr<class Central> CentralPtr;
    typedef std::shared_ptr<struct Peripheral> PeripheralPtr;
    typedef std::function<void(CentralPtr, PeripheralPtr, float)> PeripheralDiscoveredCallback;
    typedef std::function<void(CentralPtr, PeripheralPtr)> PeripheralConnectedCallback;
    
    class Central : public std::enable_shared_from_this<Central>
    {
    public:
        
        static CentralPtr create();
        
        void scan_for_peripherals();
        void connect_peripheral(const PeripheralPtr &the_peripheral);
        void set_peripheral_discovered_cb(PeripheralDiscoveredCallback cb);
        void set_peripheral_connected_cb(PeripheralConnectedCallback cb);
        
        std::set<PeripheralPtr> peripherals() const;
        
    private:
        Central();
        std::shared_ptr<struct CentralImpl> m_impl;
    };
    
    struct Peripheral
    {
        char uuid[16];
        std::string name = "unknown";
        bool is_connectable = false;
    };
    
}}//namespace