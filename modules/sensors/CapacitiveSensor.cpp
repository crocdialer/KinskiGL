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
#define SERIAL_END_CODE '\n'

#define STD_TIMEOUT_RECONNECT 5.f
#include <thread>

namespace kinski
{
    struct CapacitiveSensor::Impl
    {
        UARTPtr m_sensor_device;
        std::string m_device_name;
        std::vector<uint8_t> m_sensor_read_buf, m_sensor_accumulator;
        bool m_dirty_params = true;
        uint16_t m_touch_status = 0;
        std::vector<float> m_proximity_values;
        float m_last_reading = 0.f;
        float m_timeout_reconnect = STD_TIMEOUT_RECONNECT;
        uint16_t m_thresh_touch = 12, m_thresh_release = 6;
        uint32_t m_charge_current = 16;
        
        std::thread m_reconnect_thread;
        
        TouchCallback m_touch_callback, m_release_callback;
    };
    
    CapacitiveSensor::CapacitiveSensor(UARTPtr the_uart_device):
    m_impl(new Impl)
    {
        m_impl->m_sensor_read_buf.resize(2048);
        m_impl->m_proximity_values.resize(NUM_SENSOR_PADS, 0.f);
        
        if(!connect(the_uart_device))
        {
            LOG_ERROR << "unable to connect capacitve touch sensor";
        }
    }
    
    CapacitiveSensor::~CapacitiveSensor()
    {
        if(m_impl->m_reconnect_thread.joinable()){ m_impl->m_reconnect_thread.join(); }
    }
    
    void CapacitiveSensor::update(float time_delta )
    {
        if(m_impl->m_dirty_params && is_initialized())
        {
            if(!update_config()){ LOG_WARNING << "could not update config"; }
            m_impl->m_dirty_params = false;
        }
        
        // init with unchanged status
        uint16_t current_touches = m_impl->m_touch_status;
        size_t bytes_to_read = 0;
        m_impl->m_last_reading += time_delta;
        
        while(m_impl->m_sensor_device && m_impl->m_sensor_device->available())
        {
            bytes_to_read = std::min(m_impl->m_sensor_device->available(),
                                     m_impl->m_sensor_read_buf.size());
            
            if(bytes_to_read){ m_impl->m_last_reading = 0.f; }
            
            uint8_t *buf_ptr = &m_impl->m_sensor_read_buf[0];
            m_impl->m_sensor_device->read_bytes(&m_impl->m_sensor_read_buf[0], bytes_to_read);
            
            for(uint32_t i = 0; i < bytes_to_read; i++)
            {
                uint8_t byte = *buf_ptr++;
                
                switch(byte)
                {
                    case SERIAL_END_CODE:
                    {
                        auto tokens = split(string(m_impl->m_sensor_accumulator.begin(),
                                                   m_impl->m_sensor_accumulator.end()));
                        m_impl->m_sensor_accumulator.clear();
                        
                        if(!tokens.empty())
                        {
                            current_touches = string_to<uint16_t>(tokens.front());
                            
                            for(uint32_t j = 1; j < tokens.size(); ++j)
                            {
                                m_impl->m_proximity_values[j - 1] = string_to<float>(tokens[j]);
                            }
                        }
                        break;
                    }
                    default:
                        m_impl->m_sensor_accumulator.push_back(byte);
                        break;
                }
            }
        }
        
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
//            LOG_WARNING << "no response from sensor: trying reconnect ...";
//            m_impl->m_last_reading = 0.f;
//            try { if(m_impl->m_reconnect_thread.joinable()) m_impl->m_reconnect_thread.join(); }
//            catch (std::exception &e) { LOG_WARNING << e.what(); }
//            m_impl->m_reconnect_thread = std::thread([this]()
//            {
//                if(!m_impl->m_device_name.empty())
//                {
//                    auto serial = Serial::create();
//                    serial->setup(m_impl->m_device_name, 57600);
//                    connect(serial);
//                }else if(m_impl->m_sensor_device)
//                {
////                    m_impl->m_sensor_device->close();
//                    connect(m_impl->m_sensor_device);
//                }
//            });
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
    
    const std::vector<float>& CapacitiveSensor::proximity_values() const
    {
        return m_impl->m_proximity_values;
    }
    
    uint16_t CapacitiveSensor::num_touchpads() const
    {
        return NUM_SENSOR_PADS;
    }
    
//    bool CapacitiveSensor::connect(const std::string &the_serial_dev_name)
//    {
//        auto serial = Serial::create();
//        m_impl->m_device_name = the_serial_dev_name;
//        serial->setup(the_serial_dev_name, 57600);
//        return connect(serial);
//    }
    
    bool CapacitiveSensor::connect(UARTPtr the_uart_device)
    {
        if(the_uart_device)
        {
            m_impl->m_sensor_device = the_uart_device;
            
            m_impl->m_sensor_device->flush();
            m_impl->m_sensor_accumulator.clear();
            m_impl->m_last_reading = 0.f;
            set_thresholds(m_impl->m_thresh_touch, m_impl->m_thresh_release);
            set_charge_current(m_impl->m_charge_current);
            return true;
        }
//        else
//        {
//            m_impl->m_sensor_device = Serial::create();
//            m_impl->m_sensor_device->setup();
//        }
        
        // finally flush the newly initialized device
//        if(m_impl->m_sensor_device->is_initialized())

        return false;
    }
    
    bool CapacitiveSensor::update_config()
    {
        int bytes_written = 0;
        
        if(m_impl->m_sensor_device && m_impl->m_sensor_device->is_initialized())
        {
            auto conf_str = to_string(m_impl->m_thresh_touch) + " " +
                            to_string(m_impl->m_thresh_release) + " " +
                            to_string(m_impl->m_charge_current) + "\n";
            
            bytes_written = m_impl->m_sensor_device->write(conf_str);
        }
        return bytes_written;
    }
    
    void CapacitiveSensor::set_thresholds(uint16_t the_touch_thresh, uint16_t the_rel_thresh)
    {
        
        m_impl->m_thresh_touch = the_touch_thresh;
        m_impl->m_thresh_release = the_rel_thresh;
        m_impl->m_dirty_params = true;
    }
    
    void CapacitiveSensor::thresholds(uint16_t& the_touch_thresh, uint16_t& the_rel_thresh) const
    {
        the_touch_thresh = m_impl->m_thresh_touch;
        the_rel_thresh = m_impl->m_thresh_release;
    }
    
    void CapacitiveSensor::set_charge_current(uint8_t the_charge_current)
    {
        m_impl->m_charge_current = clamp<uint32_t>(the_charge_current, 0, 63);
        m_impl->m_dirty_params = true;
    }
    
    uint32_t CapacitiveSensor::charge_current() const
    {
        return m_impl->m_charge_current;
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
        return m_impl->m_sensor_device && m_impl->m_sensor_device->is_initialized();
    }
}
