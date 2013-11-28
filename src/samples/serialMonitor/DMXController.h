//
//  DMXController.h
//  kinskiGL
//
//  Created by Fabian on 18/11/13.
//
//

#ifndef __kinskiGL__DMXController__
#define __kinskiGL__DMXController__

#include "kinskiCore/Definitions.h"
#include "kinskiCore/Serial.h"

namespace kinski
{
    class DMXController
    {
    public:
        
        DMXController();
        
        inline const std::vector<uint8_t>& values() const {return m_dmx_values;}
        inline std::vector<uint8_t>& values(){return m_dmx_values;}
        
        void set_values(const std::vector<uint8_t> &values);
        void set_num_active_channels(int sz);
        void update();
        
        const Serial& serial() const {return m_serial;}
        
        uint8_t& operator[](int index)
        {return m_dmx_values[kinski::clamp<int>(index, 1, 512)];};
        
    private:
        
        void transmit(uint8_t label, const uint8_t* data, size_t data_length);
        Serial m_serial;
        std::vector<uint8_t> m_dmx_values;
        uint32_t m_num_active_channels;
        
    };
}// namespace

#endif /* defined(__kinskiGL__DMXController__) */
