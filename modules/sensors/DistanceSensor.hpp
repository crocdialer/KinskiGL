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
    DEFINE_CLASS_PTR(DistanceSensor)
    
    class DistanceSensor
    {
    public:
        
        typedef std::function<void(int)> distance_cb_t;
        
        static std::string id();
        static DistanceSensorPtr create(ConnectionPtr the_device = ConnectionPtr());
        virtual ~DistanceSensor();
        
        bool connect(ConnectionPtr the_device);
        uint32_t distance() const;
        void set_distance_callback(distance_cb_t cb);
        bool is_initialized() const;
        
    private:
        DistanceSensor();
        void receive_data(ConnectionPtr the_device, const std::vector<uint8_t> &the_data);
        std::unique_ptr<struct DistanceSensorImpl> m_impl;
    };
}// namespace
