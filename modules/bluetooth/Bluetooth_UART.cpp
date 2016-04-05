//
//  Bluetooth_UART.cpp
//  kinskiGL
//
//  Created by Fabian on 02/04/16.
//
//

#include "Bluetooth_UART.hpp"
#include <mutex>

namespace kinski{ namespace bluetooth{
    
    namespace{ std::mutex mutex; }
    
#define UART_SERVICE_UUID bluetooth::UUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e")
#define UART_CHARACTERISTIC_TX bluetooth::UUID("6e400002-b5a3-f393-e0a9-e50e24dcca9e")
#define UART_CHARACTERISTIC_RX bluetooth::UUID("6e400003-b5a3-f393-e0a9-e50e24dcca9e")
    
    Bluetooth_UART::Bluetooth_UART(){}
    
    Bluetooth_UART::~Bluetooth_UART()
    {
        close();
    }
    
    bool Bluetooth_UART::setup()
    {
        m_central = bluetooth::Central::create();
        m_central->set_peripheral_discovered_cb([this](bluetooth::CentralPtr c,
                                                       bluetooth::PeripheralPtr p)
        {
//            LOG_DEBUG << p->name() << " - " << p->rssi() /*<< " (" << p->uuid << ")"*/;
            if(p->connectable()){ c->connect_peripheral(p); }
        });
        
        m_central->set_peripheral_connected_cb([this](bluetooth::CentralPtr c,
                                                      bluetooth::PeripheralPtr p)
        {
            m_peripheral = p;
            m_central->stop_scanning();
            p->discover_services({UART_SERVICE_UUID});
            
            p->set_value_updated_cb([this](const bluetooth::UUID &the_uuid,
                                           const std::vector<uint8_t> &the_data)
            {
                if(the_uuid == UART_CHARACTERISTIC_RX)
                {
                    LOG_TRACE_2 << string(the_data.begin(), the_data.end());
                    std::unique_lock<std::mutex> lock(mutex);
                    m_buffer.insert(m_buffer.end(), the_data.begin(), the_data.end());
    
                    // fire receive callback
                    if(m_receive_cb)
                    {
                        m_receive_cb(*this, m_buffer);
                        m_buffer.clear();
                    }
                }
            });
        });
        m_central->discover_peripherals({UART_SERVICE_UUID});
        return true;
    }
    
///////////////////////////////////////////////////////////////////////////////
    
    void Bluetooth_UART::close()
    {
        m_peripheral.reset();
        m_central.reset();
    }
    
///////////////////////////////////////////////////////////////////////////////
    
    size_t Bluetooth_UART::read_bytes(void *buffer, size_t sz)
    {
        std::unique_lock<std::mutex> lock(mutex);
        size_t num_bytes = std::min(m_buffer.size(), sz);
        memcpy(buffer, &m_buffer[0], num_bytes);
        std::vector<uint8_t> tmp(m_buffer.end() - num_bytes, m_buffer.end());
        m_buffer = tmp;
        return num_bytes;
    }

///////////////////////////////////////////////////////////////////////////////
    
    size_t Bluetooth_UART::write_bytes(const void *buffer, size_t sz)
    {
        if(m_peripheral)
        {
            m_peripheral->write_value_for_characteristic(UART_CHARACTERISTIC_TX,
                                                         std::vector<uint8_t>((uint8_t*)buffer,
                                                                              (uint8_t*)buffer + sz));
            return sz;
        }
        return 0;
    }
    
///////////////////////////////////////////////////////////////////////////////
    
    size_t Bluetooth_UART::available()
    {
        std::unique_lock<std::mutex> lock(mutex);
        return m_buffer.size();
    }
    
///////////////////////////////////////////////////////////////////////////////
    
    void Bluetooth_UART::drain()
    {
        std::unique_lock<std::mutex> lock(mutex);
        m_buffer.clear();
    }

///////////////////////////////////////////////////////////////////////////////
    
    bool Bluetooth_UART::is_initialized() const
    {
        return m_peripheral.get();
    }
    
///////////////////////////////////////////////////////////////////////////////
    
    void Bluetooth_UART::set_receive_cb(ReceiveCallback cb)
    {
        m_receive_cb = cb;
    }
}}