//
//  Bluetooth_UART.hpp
//  kinskiGL
//
//  Created by Fabian on 02/04/16.
//
//

#pragma once

#include "core/Connection.hpp"
#include "core/CircularBuffer.hpp"
#include "bluetooth.hpp"

namespace kinski{ namespace bluetooth{

typedef std::shared_ptr<class Bluetooth_UART> Bluetooth_UARTPtr;
    
class Bluetooth_UART : public Connection, public std::enable_shared_from_this<Bluetooth_UART>
{
public:
    
    static Bluetooth_UARTPtr create();
    virtual ~Bluetooth_UART();
    
    // Connection interface
    bool open() override;
    void close() override;
    size_t read_bytes(void *buffer, size_t sz) override;
    size_t write_bytes(const void *buffer, size_t sz) override;
    size_t available() const override;
    void drain() override;
    bool is_open() const override;
    std::string description() const override;
    void set_receive_cb(receive_cb_t the_cb = receive_cb_t()) override;
    void set_connect_cb(connection_cb_t cb = connection_cb_t()) override;
    void set_disconnect_cb(connection_cb_t cb = connection_cb_t()) override;

    
private:
    Bluetooth_UART();
    std::unique_ptr<struct Bluetooth_UARTImpl> m_impl;
};
    
}}// namespaces
