//
//  CapacitveSensor.hpp
//  kinskiGL
//
//  Created by Fabian on 26/01/16.
//
//
#pragma once

#include "core/core.hpp"
#include "core/UART.hpp"

namespace kinski
{
    class CapacitiveSensor
    {
    public:
        
        typedef std::function<void(int)> TouchCallback;
        
        CapacitiveSensor(UARTPtr the_uart_device = UARTPtr());
        virtual ~CapacitiveSensor();
        
//        bool connect(const std::string &the_serial_dev_name);
        bool connect(UARTPtr the_uart_device = UARTPtr());
        
        void update(float time_delta);
        
        uint16_t touch_state() const;
        
        const std::vector<float>& proximity_values() const;
        
        uint16_t num_touchpads() const;
        
        //! return touch state for provided index,
        //  or "any" touch, if index is out of bounds or not provided
        bool is_touched(int the_index = -1) const;
        
        void set_touch_callback(TouchCallback cb);
        void set_release_callback(TouchCallback cb);
        
        float timeout_reconnect() const;
        void set_timeout_reconnect(float val);
        
        void set_thresholds(uint16_t the_touch_thresh, uint16_t the_rel_thresh);
        void thresholds(uint16_t& the_touch_thresh, uint16_t& the_rel_thresh) const;
        
        void set_charge_current(uint8_t the_charge_current);
        uint32_t charge_current() const;
        
        bool is_initialized() const;
        
    private:
        
        bool update_config();
        
        struct Impl;
        std::shared_ptr<Impl> m_impl;
    };
}
