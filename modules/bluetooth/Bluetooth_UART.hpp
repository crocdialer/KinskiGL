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

class Bluetooth_UART;
typedef std::shared_ptr<Bluetooth_UART> Bluetooth_UART_Ptr;
    
class Bluetooth_UART : public UART, public std::enable_shared_from_this<Bluetooth_UART>
{
public:
    
    typedef std::function<void(Bluetooth_UART_Ptr)> ConnectCallback;
    typedef std::function<void(Bluetooth_UART_Ptr, const std::vector<uint8_t>&)> ReceiveCallback;
    
    static Bluetooth_UART_Ptr create();
    virtual ~Bluetooth_UART();
    
    // UART interface
    bool setup() override;
    void close() override;
    size_t read_bytes(void *buffer, size_t sz) override;
    size_t write_bytes(const void *buffer, size_t sz) override;
    size_t available() override;
    void drain() override;
    void flush(bool flush_in = true, bool flush_out = true) override;
    bool is_initialized() const override;
    std::string description() override;
    
    /////////////////////////////////////////////////////////////////////////////////
    
    void set_connect_cb(ConnectCallback cb);
    void set_receive_cb(ReceiveCallback cb);
    
private:
    Bluetooth_UART();
    CentralPtr m_central;
    PeripheralPtr m_peripheral;
    std::vector<uint8_t> m_buffer;
    ConnectCallback m_connect_cb;
    ReceiveCallback m_receive_cb;
};
    
}}// namespaces
