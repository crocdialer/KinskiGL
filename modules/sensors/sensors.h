#pragma once

#include "core/UART.hpp"
#include "CapacitiveSensor.hpp"
#include "DistanceSensor.hpp"

namespace kinski{ namespace sensors{
    

typedef std::function<void(const std::string&, UARTPtr)> device_cb_t;

//! scan for available devices, group them by their ID and connect them
void scan_for_devices(boost::asio::io_service &io, device_cb_t);

//! scan for available serial devices, group them by their ID and connect them
void scan_for_serials(boost::asio::io_service &io, device_cb_t the_device_cb);
    
}}
