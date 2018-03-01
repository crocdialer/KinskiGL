//
//  DMXController.hpp
//  dmx
//
//  Created by Fabian on 18/11/13.
//
//

#pragma once

#include "core/core.hpp"

namespace kinski{ namespace dmx
{
    class DMXController
    {
    public:

        DMXController(io_service_t &io);
        ~DMXController();
        
        bool connect(const std::string &the_device_name);
        
        void update(float time_delta = 0.f);

        uint8_t& operator[](int address);
        const uint8_t& operator[](int address) const;
        
        const std::string& device_name() const;
        
        float timeout_reconnect() const;
        void set_timeout_reconnect(float val);
        
        bool is_initialized() const;

    private:
        std::unique_ptr<struct DMXControllerImpl> m_impl;
    };
}}// namespace
