//
//  SensorDebug.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "SensorDebug.h"

using namespace std;
using namespace kinski;
using namespace glm;

#define SERIAL_START_CODE 0x7E
#define SERIAL_END_CODE 0xE7

/////////////////////////////////////////////////////////////////

void SensorDebug::setup()
{
    ViewerApp::setup();
    
    fonts()[FONT_LARGE].load("Courier New Bold.ttf", 64);
    
    registerProperty(m_serial_device_name);
    registerProperty(m_sensor_refresh_rate);
    
    observeProperties();
    create_tweakbar_from_component(shared_from_this());
    
    // holds 10 uint16_t sensor-values
    m_sensor_vals.resize(10);
    
    // measure history for our 10 sensors
    m_measurements.resize(10);
    
    // buffer incoming bytes from serial connection
    m_serial_read_buf.resize(2048);
    
    m_sensor_refresh_timer = Timer(io_service(), [this]()
    {
        *m_sensor_refresh_rate = m_sensor_refresh_count;
        m_sensor_refresh_count = 0;
    });
    m_sensor_refresh_timer.set_periodic();
    m_sensor_refresh_timer.expires_from_now(1.f);
    
    load_settings();
}

/////////////////////////////////////////////////////////////////

void SensorDebug::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    // fetch new measurements
    update_sensor_values();
}

/////////////////////////////////////////////////////////////////

void SensorDebug::draw()
{
    // draw debug UI
    vec2 offset(0, 100), step(0, 35);
    float val = 0.f, sum = 0.f;
    uint32_t active_panels = 0;
    
    for(int i = 0; i < m_measurements.size(); i++)
    {
        val = (float) m_sensor_vals[i] / std::numeric_limits<uint16_t>::max();
        sum += val;
        if(m_sensor_vals[i]){ active_panels++; }
        
        gl::drawQuad(gl::COLOR_WHITE, vec2(val * windowSize().x, 30), offset);
        offset += step;
    }
    val = sum / (active_panels ? active_panels : 1);
    
    gl::drawText2D(as_string(100.f * val, 2) + "%", fonts()[FONT_LARGE], gl::COLOR_WHITE, vec2(15));
}

/////////////////////////////////////////////////////////////////

void SensorDebug::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void SensorDebug::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
}

/////////////////////////////////////////////////////////////////

void SensorDebug::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void SensorDebug::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void SensorDebug::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void SensorDebug::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void SensorDebug::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void SensorDebug::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void SensorDebug::got_message(const std::vector<uint8_t> &the_message)
{
    LOG_INFO << string(the_message.begin(), the_message.end());
}

/////////////////////////////////////////////////////////////////

void SensorDebug::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void SensorDebug::tearDown()
{
    LOG_PRINT<<"ciao sensor debugger";
}

/////////////////////////////////////////////////////////////////

void SensorDebug::updateProperty(const Property::ConstPtr &theProperty)
{
    ViewerApp::updateProperty(theProperty);
    
    if(theProperty == m_serial_device_name)
    {
        if(m_serial_device_name->value().empty()){ m_serial.setup(0, 57600); }
        else{ m_serial.setup(*m_serial_device_name, 57600); }
        m_serial.flush();
        m_serial.drain();
    }
}

/////////////////////////////////////////////////////////////////

void SensorDebug::update_sensor_values()
{
    size_t num_bytes = m_sensor_vals.size() * sizeof(m_sensor_vals[0]);
    
    // parse sensor input
    if(m_serial.isInitialized())
    {
        size_t bytes_to_read = std::min(m_serial.available(), m_serial_read_buf.size());
        uint8_t *buf_ptr = &m_serial_read_buf[0];
        
        m_serial.readBytes(&m_serial_read_buf[0], bytes_to_read);
        
        for(uint32_t i = 0; i < bytes_to_read; i++)
        {
            const uint8_t &byte = *buf_ptr++;
            bool reading_complete = false;
            
            switch(byte)
            {
                case SERIAL_END_CODE:
                    if(m_serial_accumulator.size() >= num_bytes)
                    {
                        memcpy(&m_sensor_vals[0], &m_serial_accumulator[0], num_bytes);
                        m_sensor_refresh_count++;
                        m_serial_accumulator.clear();
                        reading_complete = true;
                    }
                    else{ m_serial_accumulator.push_back(byte); }
                    break;
                    
                case SERIAL_START_CODE:
                    if(m_serial_accumulator.empty()){ break; }
                    
                default:
                    m_serial_accumulator.push_back(byte);
                    break;
            }
            
            if(reading_complete)
            {
                for(size_t i = 0; i < m_sensor_vals.size(); i++)
                {
                    m_measurements[i].push(m_sensor_vals[i]);
                }
            }
        }
    }
}
