//
//  SyphonConnector.h
//  gl
//
//  Created by Fabian on 5/3/13.
//
//

#ifndef __kinski__Bluetooth__
#define __kinski__Bluetooth__

#include "core/Definitions.h"

namespace kinski{ namespace bluetooth{
    
    class UUID;
    class Central;
    class Peripheral;
    
    typedef std::function<void(const Central&, Peripheral&, std::vector<uint8_t>, float)>
    DiscoveredPeripheralCallback;
    
    class Central
    {
    public:
        Central();
        void scan_for_peripherals();
        void connect_peripheral(const Peripheral &the_peripheral);
        
    private:
        struct CentralImpl;
        std::shared_ptr<CentralImpl> m_impl;
    };
    
    class Peripheral
    {
        std::string m_name;
    };
    
}}//namespace

#endif /* defined(__kinski__Bluetooth__) */
