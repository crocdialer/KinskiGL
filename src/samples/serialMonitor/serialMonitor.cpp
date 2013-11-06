#include "kinskiApp/ViewerApp.h"
#include "kinskiApp/AppServer.h"
#include "kinskiCore/Serial.h"
#include "Measurement.h"

using namespace std;
using namespace kinski;
using namespace glm;

class SerialMonitorSample : public ViewerApp
{
private:
    
    gl::Font m_font_large, m_font_small;
    
    // Serial communication with Arduino device
    Serial m_serial;
    
    // used for analog input measuring
    string m_input_prefix = "analog_";
    std::vector<Measurement<float>> m_analog_in {8};
    
    // display plot for selected index
    RangedProperty<int>::Ptr m_selected_index;
    
    // used for data rendering
    vector<vec3> m_points;
    gl::OrthographicCamera::Ptr m_ortho_cam;
    
public:
    
    void setup()
    {
        ViewerApp::setup();
        
        // prepare 2 font objects
        string font_name = "Courier New Bold.ttf";
        m_font_small.load(font_name, 24);
        m_font_large.load(font_name, 80);
        
        outstream_gl().set_font(m_font_small);
        outstream_gl().set_color(gl::COLOR_BLACK);
        set_clear_color(gl::COLOR_WHITE);
        
        m_selected_index = RangedProperty<int>::create("selected index", 0, 0,
                                                       m_analog_in.size() - 1);
        registerProperty(m_selected_index);
        
        create_tweakbar_from_component(shared_from_this());
        displayTweakBar(false);
        
        m_serial.listDevices();
        m_serial.setup("/dev/tty.usbmodemfd121", 57600);
        
        // drain the serial buffer before we start
        m_serial.drain();
        m_serial.flush();
        
        // restore our settings
        load_settings();
        
        m_ortho_cam.reset(new gl::OrthographicCamera(0, windowSize().x, 0, windowSize().y, 0, 1));
    }
    
    void update(float timeDelta)
    {
        ViewerApp::update(timeDelta);
        
        if(m_serial.isInitialized())
        {
            for(string line : m_serial.read_lines())
            {
                parse_line(line);
            }
        }
        
        const auto &measure = m_analog_in[*m_selected_index];
        
        m_points.resize(measure.history().size(), vec3(0));
        for (int i = 0; i < measure.history().size(); i++)
        {
            m_points[i].x = i * windowSize().x / measure.history().size();
            m_points[i].y = measure.history()[i] / 2.f;
        }
    }
    
    void draw()
    {
        const auto &measure = m_analog_in[*m_selected_index];
        
        float play_head_x_pos = measure.current_index() * windowSize().x /
                                measure.history().size();
        gl::drawLine(vec2(play_head_x_pos, 0), vec2(play_head_x_pos, windowSize().y), gl::COLOR_BLACK);
        
        gl::setProjection(m_ortho_cam);
        gl::drawLineStrip(m_points, gl::COLOR_BLACK);
        
        auto measured_val = measure.last_value();
        
        gl::drawQuad(gl::COLOR_OLIVE,
                     vec2(80, measured_val / 2),
                     vec2(windowSize().x - 100, windowSize().y - measured_val / 2));
        
        gl::drawText2D("Baumhafer(" + as_string(m_selected_index->value()) +"): " +
                            as_string(measured_val / 1023.f, 2) + " V1",
                       m_font_large,
                       gl::COLOR_BLACK, glm::vec2(30, 30));
        
        gl::drawText2D(" (" + as_string(measure.min()) +
                       " - " + as_string(measure.max()) + ")",
                       m_font_small,
                       gl::COLOR_BLACK, glm::vec2(30, 90));
        
        gl::drawText2D(" mean: " + as_string(measure.mean(), 2),
                       m_font_small,
                       gl::COLOR_BLACK, glm::vec2(30, 110));
        
        gl::drawText2D(" standard deviation: " + as_string(measure.standard_deviation(), 2),
                       m_font_small,
                       gl::COLOR_BLACK, glm::vec2(30, 130));
    }
    
    void resize(int w ,int h)
    {
        ViewerApp::resize(w, h);
        
        m_ortho_cam->right(w);
        m_ortho_cam->top(h);
    }
    
    void keyPress(const KeyEvent &e)
    {
        ViewerApp::keyPress(e);
        
        switch (e.getChar())
        {
            case KeyEvent::KEY_c:
                m_serial.setup("/dev/tty.usbmodemfd121", 57600);
                break;
            case KeyEvent::KEY_0:
            case KeyEvent::KEY_1:
            case KeyEvent::KEY_2:
            case KeyEvent::KEY_3:
            case KeyEvent::KEY_4:
            case KeyEvent::KEY_5:
            case KeyEvent::KEY_6:
            case KeyEvent::KEY_7:
                *m_selected_index = string_as<int>(as_string(e.getChar()));
                break;
                
            default:
                break;
        }
    
    }
    
    void got_message(const std::string &the_message)
    {
        LOG_INFO<<the_message;
    }
    
    void tearDown()
    {
        LOG_PRINT<<"ciao serialMonitor";
    }
    
    /////////////////////////////////////////////////////////////////
    
    void parse_line(const std::string &line)
    {
        std::istringstream ss(line);
        int parsed_index = -1;
        
        vector<string> tokens = split(line);
        
        // return if number of tokens doesn´t match or our prefix is not found
        if(tokens.size() < 2 || tokens[0].find(m_input_prefix) == string::npos) return;
        
        parsed_index = kinski::clamp<int>(string_as<int>(tokens[0].substr(m_input_prefix.size())),
                                          0,
                                          m_analog_in.size() - 1);
        m_analog_in[parsed_index].push(string_as<int>(tokens[1]));
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new SerialMonitorSample);
    theApp->setWindowSize(768, 256);
    AppServer s(theApp);
    LOG_INFO<<"Running on IP: " << AppServer::local_ip();
    
    return theApp->run();
}
