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
        
        struct DMXUSBPROParamsType
        {
            unsigned char FirmwareLSB;
            unsigned char FirmwareMSB;
            unsigned char BreakTime;
            unsigned char MaBTime;
            unsigned char RefreshRate;
        };
        
        struct DMXUSBPROSetParamsType
        {
            unsigned char UserSizeLSB;
            unsigned char UserSizeMSB;
            unsigned char BreakTime;
            unsigned char MaBTime;
            unsigned char RefreshRate;
        };
        
        inline const std::vector<uint8_t>& values() const {return m_dmx_values;}
        inline std::vector<uint8_t>& values(){return m_dmx_values;}
        
        void update();
        
    private:
        
        void set_values(const std::vector<uint8_t> &values);
        void transmit(uint8_t label, const uint8_t* data, size_t data_length);
        Serial m_serial;
        std::vector<uint8_t> m_dmx_values;
        
    };
}// namespace

#endif /* defined(__kinskiGL__DMXController__) */
