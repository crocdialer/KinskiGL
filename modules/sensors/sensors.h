#pragma once

#include "core/Connection.hpp"
#include "CapacitiveSensor.hpp"
#include "DistanceSensor.hpp"

namespace kinski{ namespace sensors{
    

typedef std::function<void(const std::string&, ConnectionPtr)> device_cb_t;

void query_device(ConnectionPtr the_device, boost::asio::io_service &io, device_cb_t the_device_cb);
    
//! scan for available devices, group them by their ID and connect them
void scan_for_devices(boost::asio::io_service &io, device_cb_t);

//! scan for available serial devices, group them by their ID and connect them
void scan_for_serials(boost::asio::io_service &io, device_cb_t the_device_cb);
    
}}
