//
//  CapacitveSensor.hpp
//  kinskiGL
//
//  Created by Fabian on 26/01/16.
//
//
#pragma once
#include "core/core.h"

namespace kinski
{
    class CapacitiveSensor
    {
    public:
        
        typedef std::function<void(int)> Callback;
        
        CapacitiveSensor(const std::string &dev_name = "");
        
        bool connect(const std::string &dev_name = "");
        
        void update(float time_delta);
        
        uint16_t touch_state() const;
        
        uint16_t num_touchpads() const;
        
        //! return touch state for provided index,
        //  or "any" touch, if index is out of bounds or not provided
        bool is_touched(int the_index = -1) const;
        
        void set_touch_callback(Callback cb);
        void set_release_callback(Callback cb);
        
        float timeout_reconnect() const;
        void set_timeout_reconnect(float val);
        
        bool is_initialized() const;
        
    private:
        
        struct Impl;
        std::shared_ptr<Impl> m_impl;
    };
}