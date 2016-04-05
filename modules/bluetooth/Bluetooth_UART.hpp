//
//  Bluetooth_UART.hpp
//  kinskiGL
//
//  Created by Fabian on 02/04/16.
//
//

#pragma once

#include "core/UART.hpp"
#include "bluetooth.hpp"

namespace kinski{ namespace bluetooth{
    
class Bluetooth_UART : public UART
{
public:
    
    typedef std::function<void(Bluetooth_UART&, const std::vector<uint8_t>&)> ReceiveCallback;
    
    Bluetooth_UART();
    ~Bluetooth_UART();
    
    // UART interface
    bool setup() override;
    void close() override;
    size_t read_bytes(void *buffer, size_t sz) override;
    size_t write_bytes(const void *buffer, size_t sz) override;
    size_t available() override;
    void drain() override;
    bool is_initialized() const override;
    
    /////////////////////////////////////////////////////////////////////////////////
    
    void set_receive_cb(ReceiveCallback cb);
    
private:
    CentralPtr m_central;
    PeripheralPtr m_peripheral;
    std::vector<uint8_t> m_buffer;
    ReceiveCallback m_receive_cb;
};
    
}}// namespaces
