#pragma once

#include "core/core.hpp"

namespace kinski{ namespace bluetooth
{
    
    typedef std::shared_ptr<class Central> CentralPtr;
    typedef std::shared_ptr<class Peripheral> PeripheralPtr;
    typedef std::function<void(CentralPtr, PeripheralPtr)> PeripheralCallback;
    
    class Central : public std::enable_shared_from_this<Central>
    {
    public:
        
        static CentralPtr create();
        
        void scan_for_peripherals();
        
        void connect_peripheral(const PeripheralPtr &the_peripheral,
                                PeripheralCallback cb = PeripheralCallback());
        void disconnect_peripheral(const PeripheralPtr &the_peripheral);
        
        void set_peripheral_discovered_cb(PeripheralCallback cb);
        void set_peripheral_connected_cb(PeripheralCallback cb);
        void set_peripheral_disconnected_cb(PeripheralCallback cb);
        
        std::set<PeripheralPtr> peripherals() const;
        
    private:
        Central();
        std::shared_ptr<struct CentralImpl> m_impl;
    };
    
    class Peripheral
    {
    public:
        
        static PeripheralPtr create(uint8_t *the_uuid);
        
        std::string uuid() const;
        std::string name() const;
        void set_name(const std::string &the_name);
        
        bool is_connectable() const;
        void set_connectable(bool b);
        
        float rssi() const;
        void set_rssi(float the_rssi);
        
    private:
        Peripheral();
        std::unique_ptr<struct PeripheralImpl> m_impl;
    };
    
}}//namespace