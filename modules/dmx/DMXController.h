//
//  DMXController.h
//  gl
//
//  Created by Fabian on 18/11/13.
//
//

#ifndef __gl__DMXController__
#define __gl__DMXController__

#include "core/core.h"
#include "core/Serial.h"

namespace kinski
{
    class DMXController
    {
    public:
        
        DMXController(const std::string &the_device_name = "");
        
        void update();
        
        const Serial& serial() const {return m_serial;}
        Serial& serial() {return m_serial;}
        
        inline uint8_t& operator[](int address)
        {return m_dmx_values[clamp(address, 0, 511)];};
        
        inline const uint8_t& operator[](int address) const
        {return m_dmx_values[clamp(address, 0, 511)];};
        
        void set_device_name(const std::string &the_device_name);
        
    private:
        
        void transmit(uint8_t label, const uint8_t* data, size_t data_length);
        Serial m_serial;
        std::vector<uint8_t> m_dmx_values;
    };
}// namespace

#endif /* defined(__gl__DMXController__) */
