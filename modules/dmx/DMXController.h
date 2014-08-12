//
//  DMXController.h
//  gl
//
//  Created by Fabian on 18/11/13.
//
//

#ifndef __gl__DMXController__
#define __gl__DMXController__

#include "kinskiCore/Definitions.h"
#include "kinskiCore/Serial.h"

namespace kinski
{
    class DMXController
    {
    public:
        
        DMXController();
        
        void update();
        
        const Serial& serial() const {return m_serial;}
        
        inline uint8_t& operator[](int index)
        {return m_dmx_values[index];};
        
        inline const uint8_t& operator[](int index) const
        {return m_dmx_values[index];};
        
    private:
        
        void transmit(uint8_t label, const uint8_t* data, size_t data_length);
        Serial m_serial;
        std::vector<uint8_t> m_dmx_values;
    };
}// namespace

#endif /* defined(__gl__DMXController__) */
