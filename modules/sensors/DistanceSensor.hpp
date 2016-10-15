//
//  DistanceSensor.hpp
//  kinskiGL
//
//  Created by Fabian on 22/03/16.
//
//

#pragma once

#include "core/core.hpp"

namespace kinski
{
    class DistanceSensor
    {
    public:
        
        typedef std::function<void(int)> callback_t;
        
        DistanceSensor(const std::string &dev_name = "");
        
        bool connect(const std::string &dev_name = "");
        void update(float time_delta);
        uint16_t distance() const;
        
        float timeout_reconnect() const;
        void set_timeout_reconnect(float val);
        
        void set_motion_callback(callback_t cb);
        bool is_initialized() const;
        
    private:
        struct Impl;
        std::shared_ptr<Impl> m_impl;
    };
}// namespace
