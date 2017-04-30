//
//  CapacitveSensor.hpp
//  kinskiGL
//
//  Created by Fabian on 26/01/16.
//
//
#pragma once

#include "core/core.hpp"
#include "core/Connection.hpp"

namespace kinski
{
    DEFINE_CLASS_PTR(CapacitiveSensor)
    
    class CapacitiveSensor
    {
    public:
        
        typedef std::function<void(int)> touch_cb_t;
        
        static std::string id();
        static CapacitiveSensorPtr create(ConnectionPtr the_uart_device = ConnectionPtr());
        virtual ~CapacitiveSensor();
        
        bool connect(ConnectionPtr the_device);
        
        uint16_t touch_state() const;
        
        const std::vector<float>& proximity_values() const;
        
        uint16_t num_touchpads() const;
        
        //! return touch state for provided index,
        //  or "any" touch, if index is out of bounds or not provided
        bool is_touched(int the_index = -1) const;
        
        void set_touch_callback(touch_cb_t cb);
        void set_release_callback(touch_cb_t cb);
        
        void set_thresholds(uint16_t the_touch_thresh, uint16_t the_rel_thresh);
        void thresholds(uint16_t& the_touch_thresh, uint16_t& the_rel_thresh) const;
        
        void set_charge_current(uint8_t the_charge_current);
        uint32_t charge_current() const;
        
        bool is_initialized() const;
        
    private:
        
        CapacitiveSensor();
        void receive_data(ConnectionPtr the_device, const std::vector<uint8_t> &the_data);
        bool update_config();
        
        std::unique_ptr<struct CapacitiveSensorImpl> m_impl;
    };
}
