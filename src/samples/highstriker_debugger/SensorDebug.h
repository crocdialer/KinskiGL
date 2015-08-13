//
//  SensorDebug.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#ifndef __gl__SensorDebug__
#define __gl__SensorDebug__

#include "app/ViewerApp.h"
#include "core/Serial.h"
#include "core/Measurement.hpp"

namespace kinski
{
    class SensorDebug : public ViewerApp
    {
    private:
        
        enum FontEnum{FONT_CONSOLE = 0, FONT_LARGE = 1};
        
        //////////////////////// sensor input ///////////////////////////////////
        
        Serial m_serial;
        
        std::vector<uint16_t> m_sensor_vals;
        std::vector<Measurement<uint16_t>> m_measurements;
        
        std::vector<uint8_t> m_serial_accumulator, m_serial_read_buf;
        
        uint32_t m_sensor_refresh_count = 0;
        Timer m_sensor_refresh_timer;
        
        Property_<string>::Ptr
        m_serial_device_name = Property_<string>::create("serial device name", "");
        
        Property_<int>::Ptr
        m_sensor_refresh_rate = Property_<int>::create("sensor refresh rate", 0);
        
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
        void updateProperty(const Property::ConstPtr &theProperty) override;
    };
}// namespace kinski

#endif /* defined(__gl__SensorDebug__) */
