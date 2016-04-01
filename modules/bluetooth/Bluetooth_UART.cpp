//
//  Bluetooth_UART.cpp
//  kinskiGL
//
//  Created by Fabian on 02/04/16.
//
//

#include "Bluetooth_UART.hpp"

namespace kinski{ namespace bluetooth{
    
    namespace{ }
    
#define UART_SERVICE_UUID bluetooth::UUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e")
#define UART_CHARACTERISTIC_TX bluetooth::UUID("6e400002-b5a3-f393-e0a9-e50e24dcca9e")
#define UART_CHARACTERISTIC_RX bluetooth::UUID("6e400003-b5a3-f393-e0a9-e50e24dcca9e")

    bool Bluetooth_UART::setup()
    {
        m_central = bluetooth::Central::create();
        m_central->set_peripheral_discovered_cb([this](bluetooth::CentralPtr c,
                                                       bluetooth::PeripheralPtr p)
        {
            LOG_DEBUG << p->name() << " - " << p->rssi() /*<< " (" << p->uuid << ")"*/;
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
                    LOG_DEBUG << string(the_data.begin(), the_data.end());
                    m_buffer.insert(m_buffer.end(), the_data.begin(), the_data.end());
                }
            });
        });
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
        size_t num_bytes = std::min(m_buffer.size(), sz);
        memcpy(buffer, &m_buffer[0], num_bytes);
        m_buffer.clear();
        return num_bytes;
    }

///////////////////////////////////////////////////////////////////////////////
    
    size_t Bluetooth_UART::write_string(const std::string &the_str)
    {
        return write_bytes(&the_str[0], the_str.size());
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
    
    size_t Bluetooth_UART::write_bytes(const std::vector<uint8_t> &the_data)
    {
        return write_bytes(&the_data[0], the_data.size());
    }
    
///////////////////////////////////////////////////////////////////////////////
    
    size_t Bluetooth_UART::available()
    {
        return m_buffer.size();
    }
    
///////////////////////////////////////////////////////////////////////////////
    
    void Bluetooth_UART::drain()
    {
        m_buffer.clear();
    }

///////////////////////////////////////////////////////////////////////////////
    
    bool Bluetooth_UART::is_initialized() const
    {
        return m_central.get();
    }
    
///////////////////////////////////////////////////////////////////////////////
    
}}