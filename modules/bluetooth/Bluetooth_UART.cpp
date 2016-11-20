//
//  Bluetooth_UART.cpp
//  kinskiGL
//
//  Created by Fabian on 02/04/16.
//
//

#include <mutex>
#include <cstring>
#include "Bluetooth_UART.hpp"

namespace kinski{ namespace bluetooth{

    namespace
    {
        std::mutex mutex;

        const bluetooth::UUID
        UART_SERVICE_UUID = bluetooth::UUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e"),
        UART_CHARACTERISTIC_TX = bluetooth::UUID("6e400002-b5a3-f393-e0a9-e50e24dcca9e"),
        UART_CHARACTERISTIC_RX = bluetooth::UUID("6e400003-b5a3-f393-e0a9-e50e24dcca9e");
    }
    
    struct Bluetooth_UARTImpl
    {
        CentralPtr m_central;
        PeripheralPtr m_peripheral;
        CircularBuffer<uint8_t> m_buffer{512 * (1 << 10)};
        Bluetooth_UART::connection_cb_t m_connect_cb, m_disconnect_cb;
        Bluetooth_UART::receive_cb_t m_receive_cb;
    };
    
    Bluetooth_UARTPtr Bluetooth_UART::create()
    {
        auto ret = Bluetooth_UARTPtr(new Bluetooth_UART());
        return ret;
    }

    Bluetooth_UART::Bluetooth_UART():m_impl(new Bluetooth_UARTImpl){}

    Bluetooth_UART::~Bluetooth_UART()
    {
        close();
    }

    bool Bluetooth_UART::open()
    {
        m_impl->m_central = bluetooth::Central::create();
        m_impl->m_central->set_peripheral_discovered_cb([this](bluetooth::CentralPtr c,
                                                       bluetooth::PeripheralPtr p)
        {
            if(p->connectable()){ c->connect_peripheral(p); }
        });

        m_impl->m_central->set_peripheral_connected_cb([this](bluetooth::CentralPtr c,
                                                              bluetooth::PeripheralPtr p)
        {
            m_impl->m_peripheral = p;
            m_impl->m_central->stop_scanning();
            p->discover_services({UART_SERVICE_UUID});

            p->set_value_updated_cb([this](const bluetooth::UUID &the_uuid,
                                           const std::vector<uint8_t> &the_data)
            {
                if(the_uuid == UART_CHARACTERISTIC_RX)
                {
                    LOG_TRACE_3 << string(the_data.begin(), the_data.end());

                    // fire receive callback
                    if(m_impl->m_receive_cb){ m_impl->m_receive_cb(shared_from_this(), the_data); }
                    else
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        std::copy(the_data.begin(), the_data.end(),
                                  std::back_inserter(m_impl->m_buffer));
                    }
                }
            });
            
            m_impl->m_central->set_peripheral_disconnected_cb([this](CentralPtr c, PeripheralPtr per)
            {
                if(per == m_impl->m_peripheral && m_impl->m_disconnect_cb)
                {
                    m_impl->m_disconnect_cb(shared_from_this());
                }
            });

            if(m_impl->m_connect_cb){ m_impl->m_connect_cb(shared_from_this()); }
        });
        m_impl->m_central->discover_peripherals({UART_SERVICE_UUID});
        return true;
    }

///////////////////////////////////////////////////////////////////////////////

    void Bluetooth_UART::close()
    {
        m_impl->m_central->disconnect_all();
        m_impl->m_peripheral.reset();
        m_impl->m_central.reset();
    }

///////////////////////////////////////////////////////////////////////////////

    size_t Bluetooth_UART::read_bytes(void *buffer, size_t sz)
    {
        std::unique_lock<std::mutex> lock(mutex);
        size_t num_bytes = std::min(m_impl->m_buffer.size(), sz);
        std::copy(m_impl->m_buffer.begin(), m_impl->m_buffer.begin() + num_bytes, (uint8_t*)buffer);
        auto tmp = std::vector<uint8_t>(m_impl->m_buffer.begin() + num_bytes,
                                        m_impl->m_buffer.end());
        m_impl->m_buffer.assign(tmp.begin(), tmp.end());
        return num_bytes;
    }

///////////////////////////////////////////////////////////////////////////////

    size_t Bluetooth_UART::write_bytes(const void *buffer, size_t sz)
    {
        if(m_impl->m_peripheral)
        {
            m_impl->m_peripheral->write_value_for_characteristic(UART_CHARACTERISTIC_TX,
                                                                 std::vector<uint8_t>((uint8_t*)buffer,
                                                                                      (uint8_t*)buffer + sz));
            return sz;
        }
        return 0;
    }

///////////////////////////////////////////////////////////////////////////////

    size_t Bluetooth_UART::available() const
    {
        std::unique_lock<std::mutex> lock(mutex);
        return m_impl->m_buffer.size();
    }

///////////////////////////////////////////////////////////////////////////////

    void Bluetooth_UART::drain()
    {
        std::unique_lock<std::mutex> lock(mutex);
        m_impl->m_buffer.clear();
    }

///////////////////////////////////////////////////////////////////////////////

    bool Bluetooth_UART::is_open() const
    {
        return m_impl->m_peripheral && m_impl->m_peripheral->is_connected();
    }

///////////////////////////////////////////////////////////////////////////////

    std::string Bluetooth_UART::description() const
    {
        return m_impl->m_peripheral->name();
    }

///////////////////////////////////////////////////////////////////////////////

    void Bluetooth_UART::set_receive_cb(receive_cb_t cb)
    {
        m_impl->m_receive_cb = cb;
    }

///////////////////////////////////////////////////////////////////////////////
    
    void Bluetooth_UART::set_connect_cb(connection_cb_t cb)
    {
        m_impl->m_connect_cb = cb;
    }
    
///////////////////////////////////////////////////////////////////////////////
    
    void Bluetooth_UART::set_disconnect_cb(connection_cb_t cb)
    {
        m_impl->m_disconnect_cb = cb;
    }
}}
