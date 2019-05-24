#pragma once

#include "crocore/Connection.hpp"
#include "CapacitiveSensor.hpp"
#include "DistanceSensor.hpp"

namespace kinski{ namespace sensors{
    

typedef std::function<void(const std::string&, crocore::ConnectionPtr)> device_cb_t;

void query_device(crocore::ConnectionPtr the_device, crocore::io_service_t &io, device_cb_t the_device_cb);
    
//! scan for available devices, group them by their ID and connect them
void scan_for_devices(crocore::io_service_t &io, device_cb_t);

//! scan for available serial devices, group them by their ID and connect them
void scan_for_serials(crocore::io_service_t &io, device_cb_t the_device_cb, uint32_t the_baudrate = 115200);
    
}}
