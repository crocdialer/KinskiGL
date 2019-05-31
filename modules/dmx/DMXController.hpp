//
//  DMXController.hpp
//  dmx
//
//  Created by Fabian on 18/11/13.
//
//

#pragma once

#include "crocore/crocore.hpp"

using namespace crocore;

namespace dmx
{
    class Controller
    {
    public:

        Controller(io_service_t &io);
        ~Controller();
        
        bool connect(const std::string &the_device_name);
        
        void update(float time_delta = 0.f);

        uint8_t& operator[](int address);
        const uint8_t& operator[](int address) const;
        
        const std::string& device_name() const;
        
        float timeout_reconnect() const;
        void set_timeout_reconnect(float val);
        
        bool is_initialized() const;

    private:
        std::unique_ptr<struct ControllerImpl> m_impl;
    };
}// namespace
