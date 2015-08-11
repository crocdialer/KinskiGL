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

namespace kinski
{
    class SensorDebug : public ViewerApp
    {
    private:
        
        Serial m_serial;
        
        std::vector<uint16_t> m_sensor_vals;
        std::vector<uint8_t> m_serial_accumulator, m_serial_read_buf;
        
        Property_<string>::Ptr
        m_serial_device_name = Property_<string>::create("serial device name", "");
        
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
