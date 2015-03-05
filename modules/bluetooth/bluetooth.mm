//
//  bluetooth.cpp
//
//  Created by Fabian on 3/5/15.
//
//

#include "bluetooth.h"
#include "CoreBluetooth/CoreBluetooth.h"

namespace kinski{ namespace bluetooth{

    struct Central::CentralImpl
    {
    
    };
    
    Central::Central():
    m_impl(new CentralImpl)
    {
    
    }
    
    std::list<Peripheral> Central::scan_for_peripherals()
    {
        return {};
    }
    
    void Central::connect_peripheral(const Peripheral &the_peripheral)
    {
    
    }
    
///////////////////////////////////////////////////////////////////////////////////////////
    
}}//namespace
