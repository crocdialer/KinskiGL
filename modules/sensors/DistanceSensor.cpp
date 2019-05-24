//
//  DistanceSensor.cpp
//  kinskiGL
//
//  Created by Fabian on 22/03/16.
//
//

//#include <cstring>
#include "crocore/CircularBuffer.hpp"
#include "crocore/Serial.hpp"
#include "DistanceSensor.hpp"

#define DEVICE_ID "DISTANCE_SENSOR"
#define SERIAL_END_CODE '\n'

namespace kinski{
    
    struct DistanceSensorImpl
    {
        crocore::ConnectionPtr m_sensor_device;
        crocore::CircularBuffer<uint8_t> m_sensor_accumulator{512};
        uint32_t m_distance = 0;
        
        DistanceSensor::distance_cb_t m_distance_callback;
    };
    
    DistanceSensorPtr DistanceSensor::create(crocore::ConnectionPtr the_device)
    {
        DistanceSensorPtr ret(new DistanceSensor());
        if(the_device){ ret->connect(the_device); }
        return ret;
    }
    
    DistanceSensor::DistanceSensor():
    m_impl(new DistanceSensorImpl)
    {
        
    }
    
    DistanceSensor::~DistanceSensor()
    {
    
    }
    
    bool DistanceSensor::connect(crocore::ConnectionPtr the_device)
    {
        m_impl->m_sensor_device = the_device;
        m_impl->m_sensor_accumulator.clear();
        
        if(the_device && the_device->is_open())
        {
            the_device->set_receive_cb(std::bind(&DistanceSensor::receive_data,
                                                 this,
                                                 std::placeholders::_1,
                                                 std::placeholders::_2));
            return true;
        }
        return false;
    }
    
    void DistanceSensor::receive_data(crocore::ConnectionPtr the_device, const std::vector<uint8_t> &the_data)
    {
        bool reading_complete = false;
        uint32_t distance_val = 0;
            
        for(uint8_t byte : the_data)
        {
            switch(byte)
            {
                case SERIAL_END_CODE:
                {
                    auto str = std::string(m_impl->m_sensor_accumulator.begin(), m_impl->m_sensor_accumulator.end());
                    distance_val = crocore::string_to<uint32_t>(str);
                    m_impl->m_sensor_accumulator.clear();
                    reading_complete = true;
                    break;
                }
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
    
    uint32_t DistanceSensor::distance() const
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
