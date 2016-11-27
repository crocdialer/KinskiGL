//
//  CapacitveSensor.cpp
//  kinskiGL
//
//  Created by Fabian on 26/01/16.
//
//

#include <cstring>
#include "CapacitiveSensor.hpp"
#include "core/CircularBuffer.hpp"
#include "core/Serial.hpp"

#define DEVICE_ID "CAPACITIVE_SENSOR"

#define NUM_SENSOR_PADS 13
#define SERIAL_END_CODE '\n'

namespace kinski
{
    struct CapacitiveSensorImpl
    {
        UARTPtr m_sensor_device;
        std::string m_device_name;
        CircularBuffer<uint8_t> m_sensor_accumulator{512};
        bool m_dirty_params = true;
        uint16_t m_touch_status = 0;
        std::vector<float> m_proximity_values;
        uint16_t m_thresh_touch = 12, m_thresh_release = 6;
        uint32_t m_charge_current = 16;
        
        CapacitiveSensor::touch_cb_t m_touch_callback, m_release_callback;
    };
    
    CapacitiveSensorPtr CapacitiveSensor::create(UARTPtr the_uart_device)
    {
        CapacitiveSensorPtr ret(new CapacitiveSensor());
        if(the_uart_device){ ret->connect(the_uart_device); }
        return ret;
    }
    
    CapacitiveSensor::CapacitiveSensor():
    m_impl(new CapacitiveSensorImpl)
    {
        m_impl->m_proximity_values.resize(NUM_SENSOR_PADS, 0.f);
    }
    
    CapacitiveSensor::~CapacitiveSensor()
    {
        
    }
    
    void CapacitiveSensor::receive_data(UARTPtr the_uart, const std::vector<uint8_t> &the_data)
    {
        if(m_impl->m_dirty_params && is_initialized())
        {
            if(!update_config()){ LOG_WARNING << "could not update config"; }
            m_impl->m_dirty_params = false;
        }
        
        // init with unchanged status
        uint16_t current_touches = m_impl->m_touch_status;
        
        for(uint8_t byte : the_data)
        {
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
        
        auto old_state = m_impl->m_touch_status;
        m_impl->m_touch_status = current_touches;
        
        for (int i = 0; i < NUM_SENSOR_PADS; i++)
        {
            uint16_t mask = 1 << i;
            
            // pad is currently being touched
            if(mask & current_touches && !(mask & old_state))
            {
                if(m_impl->m_touch_callback){ m_impl->m_touch_callback(i); }
            }
            else if(mask & old_state && !(mask & current_touches))
            {
                if(m_impl->m_release_callback){ m_impl->m_release_callback(i); }
            }
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
    
    bool CapacitiveSensor::connect(UARTPtr the_uart_device)
    {
        m_impl->m_sensor_device = the_uart_device;
        m_impl->m_sensor_accumulator.clear();
        
        if(the_uart_device /*&& the_uart_device->is_open()*/)
        {
            the_uart_device->drain();
            set_thresholds(m_impl->m_thresh_touch, m_impl->m_thresh_release);
            set_charge_current(m_impl->m_charge_current);
            m_impl->m_sensor_device->set_receive_cb(std::bind(&CapacitiveSensor::receive_data,
                                                              this,
                                                              std::placeholders::_1,
                                                              std::placeholders::_2));
            return true;
        }
        return false;
    }
    
    bool CapacitiveSensor::update_config()
    {
        int bytes_written = 0;
        
        if(m_impl->m_sensor_device && m_impl->m_sensor_device->is_open())
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
    
    void CapacitiveSensor::set_touch_callback(touch_cb_t cb)
    {
        m_impl->m_touch_callback = cb;
    }
    
    void CapacitiveSensor::set_release_callback(touch_cb_t cb)
    {
        m_impl->m_release_callback = cb;
    }
    
    bool CapacitiveSensor::is_initialized() const
    {
        return m_impl->m_sensor_device && m_impl->m_sensor_device->is_open();
    }
    
    std::string CapacitiveSensor::id()
    {
        return DEVICE_ID;
    }
}
