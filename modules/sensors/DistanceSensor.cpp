//
//  DistanceSensor.cpp
//  kinskiGL
//
//  Created by Fabian on 22/03/16.
//
//

#include <cstring>
#include <thread>
#include "core/Serial.hpp"
#include "DistanceSensor.hpp"

#define DEVICE_ID "DISTANCE_SENSOR"
#define SERIAL_END_CODE '\n'
#define STD_TIMEOUT_RECONNECT 5.f

namespace kinski{
    
    struct DistanceSensorImpl
    {
        UARTPtr m_sensor_device;
        std::vector<uint8_t> m_sensor_read_buf, m_sensor_accumulator;
        uint16_t m_distance = 0;
        float m_last_reading = 0.f;
        float m_timeout_reconnect = STD_TIMEOUT_RECONNECT;
        std::thread m_reconnect_thread;
        
        DistanceSensor::distance_cb_t m_distance_callback;
    };
    
    DistanceSensorPtr DistanceSensor::create(UARTPtr the_uart_device)
    {
        DistanceSensorPtr ret(new DistanceSensor());
        if(the_uart_device){ ret->connect(the_uart_device); }
        return ret;
    }
    
    DistanceSensor::DistanceSensor():
    m_impl(new DistanceSensorImpl)
    {
        m_impl->m_sensor_read_buf.resize(2048);
    }
    
    DistanceSensor::~DistanceSensor()
    {
    
    }
    
    bool DistanceSensor::connect(UARTPtr the_uart_device)
    {
        if(the_uart_device && the_uart_device->is_open())
        {
            m_impl->m_sensor_device = the_uart_device;
            m_impl->m_sensor_accumulator.clear();
            m_impl->m_last_reading = 0.f;
            
            m_impl->m_sensor_device->set_receive_cb(std::bind(&DistanceSensor::receive_data,
                                                              this,
                                                              std::placeholders::_1,
                                                              std::placeholders::_2));
            return true;
        }
        return false;
    }
    
    void DistanceSensor::receive_data(UARTPtr the_uart, const std::vector<uint8_t> &the_data)
    {
        bool reading_complete = false;
        uint16_t distance_val = 0;
            
        for(uint8_t byte : the_data)
        {
            switch(byte)
            {
                case SERIAL_END_CODE:
                    distance_val = string_to<uint16_t>(string(m_impl->m_sensor_accumulator.begin(),
                                                              m_impl->m_sensor_accumulator.end()));
                    m_impl->m_sensor_accumulator.clear();
                    reading_complete = true;
                    break;
                    
                default:
                    m_impl->m_sensor_accumulator.push_back(byte);
                    break;
            }
        }
        
        if(reading_complete)
        {
            m_impl->m_distance = distance_val;
            if(m_impl->m_distance_callback){ m_impl->m_distance_callback(m_impl->m_distance); }
        }
    }
    
    uint16_t DistanceSensor::distance() const
    {
        return m_impl->m_distance;
    }
    
    void DistanceSensor::set_distance_callback(distance_cb_t cb)
    {
        m_impl->m_distance_callback = cb;
    }
    
    bool DistanceSensor::is_initialized() const
    {
        return m_impl->m_sensor_device && m_impl->m_sensor_device->is_open();
    }
    
    std::string DistanceSensor::id()
    {
        return DEVICE_ID;
    }
}
