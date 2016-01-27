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
        
        void update(float time_delta);
        
        //! return touch state for provided index,
        //  or "any" touch, if index is out of bounds or not provided
        bool is_touched(int the_index = -1);
        
        bool connect(const std::string &dev_name = "");
        
        void set_touch_callback(Callback cb);
        void set_release_callback(Callback cb);
        
    private:
        
        struct Impl;
        std::shared_ptr<Impl> m_impl;
    };
}