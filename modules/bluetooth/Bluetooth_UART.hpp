//
//  Bluetooth_UART.hpp
//  kinskiGL
//
//  Created by Fabian on 02/04/16.
//
//

#pragma once

#include "bluetooth.hpp"

namespace kinski{ namespace bluetooth{
    
class Bluetooth_UART
{
public:
    
    typedef std::function<void(Bluetooth_UART&, const std::vector<uint8_t>&)> ReceiveCallback;
    
    Bluetooth_UART();
    
    ~Bluetooth_UART();
    
    bool setup();
    
    void close();
    
    size_t read_bytes(void *buffer, size_t sz);
    
    size_t write_string(const std::string &the_str);
    
    size_t write_bytes(const void *buffer, size_t sz);
    
    size_t write_bytes(const std::vector<uint8_t> &the_data);
    
    size_t available();
    
    void drain();
    
    bool is_initialized() const;
    
    void set_receive_cb(ReceiveCallback cb);
    
private:
    CentralPtr m_central;
    PeripheralPtr m_peripheral;
    std::vector<uint8_t> m_buffer;
    ReceiveCallback m_receive_cb;
};
    
}}// namespaces
