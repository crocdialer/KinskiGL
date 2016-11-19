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

typedef std::shared_ptr<class Bluetooth_UART> Bluetooth_UARTPtr;
    
class Bluetooth_UART : public UART, public std::enable_shared_from_this<Bluetooth_UART>
{
public:
    
    static Bluetooth_UARTPtr create();
    virtual ~Bluetooth_UART();
    
    // UART interface
    bool setup() override;
    void close() override;
    size_t read_bytes(void *buffer, size_t sz) override;
    size_t write_bytes(const void *buffer, size_t sz) override;
    size_t available() const override;
    void drain() override;
    void flush(bool flush_in = true, bool flush_out = true) override;
    bool is_initialized() const override;
    std::string description() const override;
    
    /////////////////////////////////////////////////////////////////////////////////
    
    void set_connect_cb(connect_cb_t cb) override;
    void set_receive_cb(receive_cb_t cb) override;
    
private:
    Bluetooth_UART();
    CentralPtr m_central;
    PeripheralPtr m_peripheral;
    std::vector<uint8_t> m_buffer;
    connect_cb_t m_connect_cb;
    receive_cb_t m_receive_cb;
};
    
}}// namespaces
