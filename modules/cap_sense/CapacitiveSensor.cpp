//
//  CapacitveSensor.cpp
//  kinskiGL
//
//  Created by Fabian on 26/01/16.
//
//

#include <cstring>
#include "CapacitiveSensor.hpp"
#include "core/Serial.hpp"

#define NUM_SENSOR_PADS 13
#define SERIAL_START_CODE 0x7E
#define SERIAL_END_CODE 0xE7

#define STD_TIMEOUT_RECONNECT 5.f
#include <thread>

namespace kinski
{
    struct CapacitiveSensor::Impl
    {
        Serial m_sensor_device;
        std::string m_device_name;
        std::vector<uint8_t> m_sensor_read_buf, m_sensor_accumulator;
        uint16_t m_touch_status = 0;
        float m_last_reading = 0.f;
        float m_timeout_reconnect = STD_TIMEOUT_RECONNECT;
        
        std::thread m_reconnect_thread;
        
        TouchCallback m_touch_callback, m_release_callback;
    };
    
    CapacitiveSensor::CapacitiveSensor(const std::string &dev_name):
    m_impl(new Impl)
    {
        m_impl->m_device_name = dev_name;
        m_impl->m_sensor_read_buf.resize(2048);
        
        if(!dev_name.empty() && !connect(dev_name))
        {
            LOG_ERROR << "unable to connect capacitve touch sensor";
        }
        
    }
    
    void CapacitiveSensor::update(float time_delta)
    {
        // init with unchanged status
        uint16_t current_touches = m_impl->m_touch_status;
        size_t bytes_to_read = 0;
        m_impl->m_last_reading += time_delta;
        
        if(m_impl->m_sensor_device.isInitialized())
        {
            size_t num_bytes = sizeof(m_impl->m_touch_status);
            
            bytes_to_read = std::min(m_impl->m_sensor_device.available(),
                                     m_impl->m_sensor_read_buf.size());
            
            if(bytes_to_read){ m_impl->m_last_reading = 0.f; }
            
            uint8_t *buf_ptr = &m_impl->m_sensor_read_buf[0];
            m_impl->m_sensor_device.readBytes(&m_impl->m_sensor_read_buf[0], bytes_to_read);
            bool reading_complete = false;
            
            for(uint32_t i = 0; i < bytes_to_read; i++)
            {
                uint8_t byte = *buf_ptr++;
                
                switch(byte)
                {
                    case SERIAL_END_CODE:
                        if(m_impl->m_sensor_accumulator.size() >= num_bytes)
                        {
                            memcpy(&current_touches, &m_impl->m_sensor_accumulator[0], num_bytes);
                            m_impl->m_sensor_accumulator.clear();
                            reading_complete = true;
                        }
                        else{ m_impl->m_sensor_accumulator.push_back(byte); }
                        break;
                        
                    case SERIAL_START_CODE:
                        if(m_impl->m_sensor_accumulator.empty()){ break; }
                        
                    default:
                        m_impl->m_sensor_accumulator.push_back(byte);
                        break;
                }
            }
        }
#if defined(KINSKI_RASPI)
        current_touches = swap_endian(current_touches);
#endif
        
        for (int i = 0; i < NUM_SENSOR_PADS; i++)
        {
            uint16_t mask = 1 << i;
            
            // pad is currently being touched
            if(mask & current_touches && !(mask & m_impl->m_touch_status))
            {
                if(m_impl->m_touch_callback){ m_impl->m_touch_callback(i); }
            }
            else if(mask & m_impl->m_touch_status && !(mask & current_touches))
            {
                if(m_impl->m_release_callback){ m_impl->m_release_callback(i); }
            }
        }
        m_impl->m_touch_status = current_touches;
        
        if((m_impl->m_timeout_reconnect > 0.f) && m_impl->m_last_reading > m_impl->m_timeout_reconnect)
        {
            LOG_WARNING << "no response from sensor: trying reconnect ...";
            m_impl->m_last_reading = 0.f;
            try { if(m_impl->m_reconnect_thread.joinable()) m_impl->m_reconnect_thread.join(); }
            catch (std::exception &e) { LOG_WARNING << e.what(); }
            m_impl->m_reconnect_thread = std::thread([this](){ connect(m_impl->m_device_name); });
            return;
        }
    }
    
    bool CapacitiveSensor::is_touched(int the_index) const
    {
        if(the_index < 0 || the_index >= NUM_SENSOR_PADS){ return m_impl->m_touch_status; }
        uint16_t mask = 1 << the_index;
        return m_impl->m_touch_status & mask;
    }
    
    uint16_t CapacitiveSensor::touch_state() const
    {
        return m_impl->m_touch_status;
    }
    
    uint16_t CapacitiveSensor::num_touchpads() const
    {
        return NUM_SENSOR_PADS;
    }
    
    bool CapacitiveSensor::connect(const std::string &dev_name)
    {
        if(dev_name.empty())
        {
//            m_impl->m_sensor_device.setup(0, 57600);
        }
        else{ m_impl->m_sensor_device.setup(dev_name, 57600); }
        
        m_impl->m_device_name = dev_name;
        
        // finally flush the newly initialized device
        if(m_impl->m_sensor_device.isInitialized())
        {
            m_impl->m_sensor_device.flush();
            m_impl->m_last_reading = 0.f;
            return true;
        }
        return false;
    }
    
    void CapacitiveSensor::set_touch_callback(TouchCallback cb)
    {
        m_impl->m_touch_callback = cb;
    }
    
    void CapacitiveSensor::set_release_callback(TouchCallback cb)
    {
        m_impl->m_release_callback = cb;
    }
    
    float CapacitiveSensor::timeout_reconnect() const
    {
        return m_impl->m_timeout_reconnect;
    }
    
    void CapacitiveSensor::set_timeout_reconnect(float val)
    {
        m_impl->m_timeout_reconnect = std::max(val, 0.f);
    }
    
    bool CapacitiveSensor::is_initialized() const
    {
        return m_impl->m_sensor_device.isInitialized();
    }
}