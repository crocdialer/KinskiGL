//
//  SensorDebug.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#pragma once

#include "app/ViewerApp.h"
#include "core/Serial.hpp"
#include "core/Measurement.hpp"

namespace kinski
{
    class SensorDebug : public ViewerApp
    {
    private:
        
        enum FontEnum{FONT_CONSOLE = 0, FONT_MEDIUM = 1, FONT_LARGE = 2};
        
        //////////////////////// sensor input ///////////////////////////////////
        
        Serial m_serial;
        
        std::vector<uint16_t> m_sensor_vals;
        std::vector<Measurement<float>> m_measurements;
        
        std::vector<uint8_t> m_serial_accumulator, m_serial_read_buf;
        
        float m_sensor_last_avg = 0.f;
        
        uint32_t m_sensor_refresh_count = 0;
        Timer m_timer_sensor_refresh, m_timer_game_ready;
        
        Property_<string>::Ptr
        m_serial_device_name = Property_<string>::create("serial device name", "");
        
        Property_<int>::Ptr
        m_sensor_refresh_rate = Property_<int>::create("sensor refresh rate", 0);
        
        RangedProperty<float>::Ptr
        m_timeout_game_ready = RangedProperty<float>::create("timeout game_ready", 1.f, 0.f, 60.f);
        
        Property_<gl::vec2>::Ptr
        m_range_min_max = Property_<gl::vec2>::create("sensor range min/max", gl::vec2(0.f, 1.f));
        
        
        /////////////////////////////////////////////////////////////////////////
        
        void update_sensor_values();
        
    public:
        
        void setup() override;
        void update(float timeDelta) override;
        void draw() override;
        void resize(int w ,int h) override;
        void keyPress(const KeyEvent &e) override;
        void keyRelease(const KeyEvent &e) override;
        void mousePress(const MouseEvent &e) override;
        void mouseRelease(const MouseEvent &e) override;
        void mouseMove(const MouseEvent &e) override;
        void mouseDrag(const MouseEvent &e) override;
        void mouseWheel(const MouseEvent &e) override;
        void got_message(const std::vector<uint8_t> &the_message) override;
        void fileDrop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void tearDown() override;
        void update_property(const Property::ConstPtr &theProperty) override;
    };
}// namespace kinski