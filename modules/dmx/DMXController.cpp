//
//  DMXController.cpp
//  gl
//
//  Created by Fabian on 18/11/13.
//
//
#include <thread>
#include "DMXController.hpp"
#include "core/Serial.hpp"
#include "core/file_functions.hpp"

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

#define STD_TIMEOUT_RECONNECT 0.f

namespace kinski{ namespace dmx
{
    struct DMXControllerImpl
    {
        SerialPtr m_serial;
        std::string m_device_name;
        std::vector<uint8_t> m_dmx_values;
        float m_last_reading = 0.f, m_timeout_reconnect = STD_TIMEOUT_RECONNECT;
        std::thread m_reconnect_thread;
        
        DMXControllerImpl(io_service_t &io):
        m_serial(Serial::create(io)){}
        
        void transmit(uint8_t label, const uint8_t* data, size_t data_length)
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
            int bytes_written = m_serial->write_bytes(&bytes[0], bytes.size());
            if(bytes_written > 0){ m_last_reading = 0.f; }
        }
    };

    DMXController::DMXController(io_service_t &io):
    m_impl(new DMXControllerImpl(io))
    {
        connect("");
        m_impl->m_dmx_values.resize(513, 0);
    }
    
    DMXController::~DMXController()
    {
        try { if(m_impl->m_reconnect_thread.joinable()) m_impl->m_reconnect_thread.join(); }
        catch (std::exception &e) { LOG_WARNING << e.what(); }
    }
    
    void DMXController::update(float time_delta)
    {
        m_impl->m_last_reading += time_delta;
        
        // update only when serial connection is initialized
        if(m_impl->m_serial->is_open())
        {
            // send values
            m_impl->transmit(SET_DMX_TX_MODE, &m_impl->m_dmx_values[0], m_impl->m_dmx_values.size());
            
            // receive values
//            m_impl->transmit(SET_DMX_RX_MODE, nullptr, 0);
        }
        
        if(m_impl->m_timeout_reconnect > 0 && (m_impl->m_last_reading > m_impl->m_timeout_reconnect))
        {
            LOG_WARNING << "no response from sensor: trying reconnect ...";
            m_impl->m_last_reading = 0.f;
            try { if(m_impl->m_reconnect_thread.joinable()) m_impl->m_reconnect_thread.join(); }
            catch (std::exception &e) { LOG_WARNING << e.what(); }
            m_impl->m_reconnect_thread = std::thread([this](){ connect(m_impl->m_device_name); });
            return;
        }
    }
    
    const std::string& DMXController::device_name() const
    {
        return m_impl->m_device_name;
    }
    
    bool DMXController::connect(const std::string &the_device_name)
    {
        std::vector<std::string> dev_name_patterns = {"tty.usbserial-EN", "ttyUSB"};
        std::string found_name;
        
        if(the_device_name.empty())
        {
            for(const auto &dev : fs::get_directory_entries("/dev"))
            {
                for(const auto &pattern : dev_name_patterns)
                {
                    if(dev.find(pattern) != string::npos){ found_name = dev; break; }
                }
                if(!found_name.empty()){ break; }
            }
        }else{ found_name = the_device_name; }
        
        // finally flush the newly initialized device
        if(!found_name.empty() && m_impl->m_serial->open(found_name, 57600))
        {
            m_impl->m_last_reading = 0.f;
            m_impl->m_device_name = found_name;
            LOG_DEBUG << "successfully connected dmx-device: " << found_name;
            return true;
        }
        LOG_ERROR << "no DMX-usb device found";
        return false;
    }

    uint8_t& DMXController::operator[](int address)
    {
        return m_impl->m_dmx_values[clamp(address, 0, 512)];
    }

    const uint8_t& DMXController::operator[](int address) const
    {
        return m_impl->m_dmx_values[clamp(address, 0, 512)];
    }
    
    float DMXController::timeout_reconnect() const
    {
        return m_impl->m_timeout_reconnect;
    }
    
    void DMXController::set_timeout_reconnect(float val)
    {
        m_impl->m_timeout_reconnect = std::max(val, 0.f);
    }
    
    bool DMXController::is_initialized() const
    {
        return m_impl->m_serial->is_open();
    }

}}// namespace
