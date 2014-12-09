//
//  DMXController.cpp
//  gl
//
//  Created by Fabian on 18/11/13.
//
//
#include "DMXController.h"
#include "core/Logger.h"

// Enttec Pro definitions
#define GET_WIDGET_PARAMS 3
#define GET_WIDGET_SN 10
#define GET_WIDGET_PARAMS_REPLY 3
#define SET_WIDGET_PARAMS 4
#define SET_DMX_RX_MODE 5
#define SET_DMX_TX_MODE 6
#define SEND_DMX_RDM_TX 7
#define RECEIVE_DMX_ON_CHANGE 8
#define RECEIVED_DMX_COS_TYPE 9
#define ONE_BYTE 1
#define DMX_START_CODE 0x7E
#define DMX_END_CODE 0xE7
#define OFFSET 0xFF
#define DMX_HEADER_LENGTH 4
#define BYTE_LENGTH 8
#define HEADER_RDM_LABEL 5
#define NO_RESPONSE 0
#define DMX_PACKET_SIZE 512

namespace kinski
{
    DMXController::DMXController(const std::string &the_device_name)
    {
        std::list<std::string> device_names = {"/dev/tty.usbserial-EN138300", the_device_name};
        bool success = false;
        for (const auto &n : device_names)
        {
            if(m_serial.setup(n, 57600))
            {
                success = true;
                break;
            }
        }
        LOG_ERROR_IF(!success) << "No DMX-Usb device found";
        m_dmx_values.resize(512, 0);
    }
    
    void DMXController::update()
    {
        // update only when serial connection is initialized
        if(m_serial.isInitialized())
            transmit(SET_DMX_TX_MODE, &m_dmx_values[0], m_dmx_values.size());
    }
    
    void DMXController::transmit(uint8_t label, const uint8_t* data, size_t data_length)
    {
        vector<uint8_t> bytes =
        {
            DMX_START_CODE,
            label,
            static_cast<unsigned char>(data_length & 0xFF),
            static_cast<unsigned char>((data_length >> 8) & 0xFF)
        };
        bytes.insert(bytes.end(), data, data + data_length);
        bytes.push_back(DMX_END_CODE);
        
        // write our data block
        m_serial.writeBytes(&bytes[0], bytes.size());
    }
    
    void DMXController::set_device_name(const std::string &the_device_name)
    {
        m_serial.setup(the_device_name, 57600);
    }
    
}// namespace
