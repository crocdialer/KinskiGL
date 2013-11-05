#include "kinskiApp/ViewerApp.h"
#include "kinskiApp/AppServer.h"

#include "kinskiCore/Serial.h"

using namespace std;
using namespace kinski;
using namespace glm;

template <typename T> class Measurement
{
public:
    explicit Measurement(uint32_t history_size = 1000):
    m_min(std::numeric_limits<T>::max()),
    m_max(std::numeric_limits<T>::min())
    {
        m_current_measure_index = 0;
        m_measure_history.resize(history_size, T());
    }
    
    inline void push(const T &value)
    {
        m_measure_history[m_current_measure_index] = m_avg_filter.filter(value);
        m_current_measure_index = (m_current_measure_index + 1) % m_measure_history.size();
        m_min = std::min(value, m_min);
        m_max = std::max(value, m_max);
    }
    
    inline const std::vector<T>& history() const { return m_measure_history;}
    inline const int current_index() const { return m_current_measure_index;}
    inline const T& last_value() const
    {
        int previous_index = m_current_measure_index - 1;
        return previous_index < 0 ? m_measure_history.back() : m_measure_history[previous_index];
    }
    
    inline uint32_t filter_window_size() const {return m_avg_filter.window_size();}
    inline void filter_window_size(uint32_t sz) {return m_avg_filter.window_size(sz);}
    
    inline const T mean() const
    {
        T sum(0);
        for (const auto &val : m_measure_history){ sum += val; }
        return static_cast<T>(sum / (float)m_measure_history.size());
    }
    
    inline const T variance() const
    {
        T mean_val = mean();
        T sum(0);
        for (const auto &val : m_measure_history){ sum += (val - mean_val) * (val - mean_val); }
        return static_cast<T>(sum / (float)(float)m_measure_history.size());
    }
    
    inline const T standard_deviation() const {return sqrtf(variance());}
    
    inline const T min() const {return m_min;}
    
    inline const T max() const {return m_max;}
    
    inline void reset()
    {
        std::fill(m_measure_history.begin(), m_measure_history.end(), T(0));
        m_current_measure_index = 0;
        m_min = std::numeric_limits<T>::max();
        m_max = std::numeric_limits<T>::min();
    }
    
private:
    T m_min, m_max;
    MovingAverage<T> m_avg_filter;
    std::vector<T> m_measure_history;
    int m_current_measure_index;
};

class SerialMonitorSample : public ViewerApp
{
private:
    
    gl::Font m_font_large, m_font_small;
    
    // Serial communication with Arduino device
    Serial m_serial;
    
    // used for analog input measuring
    Measurement<float> m_analog_in[6];
    
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
        
        create_tweakbar_from_component(shared_from_this());
        displayTweakBar(false);
        
        m_serial.listDevices();
        //m_serial.setup();
        m_serial.setup("/dev/tty.usbmodemfd121", 57600);
        
        // drain the serial buffer before we start
        m_serial.drain();
        m_serial.flush();
        
        // init output vector array
        
        
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
                m_analog_in[0].push(string_as<float>(line));
                
                m_points.resize(m_analog_in[0].history().size(), vec3(0));
                for (int i = 0; i < m_analog_in[0].history().size(); i++)
                {
                    m_points[i].x = i * windowSize().x / m_analog_in[0].history().size();
                    m_points[i].y = m_analog_in[0].history()[i] / 2.f;
                }
            }
        }
    }
    
    void draw()
    {
        float play_head_x_pos = m_analog_in[0].current_index() * windowSize().x /
                                m_analog_in[0].history().size();
        gl::drawLine(vec2(play_head_x_pos, 0), vec2(play_head_x_pos, windowSize().y), gl::COLOR_BLACK);
        
        gl::setProjection(m_ortho_cam);
        gl::drawLineStrip(m_points, gl::COLOR_BLACK);
        
        auto measured_val = m_analog_in[0].last_value();
        
        gl::drawQuad(gl::COLOR_OLIVE,
                     vec2(80, measured_val / 2),
                     vec2(windowSize().x - 100, windowSize().y - measured_val / 2));
        
        gl::drawText2D("Baumhafer 0" + as_string(measured_val, 4),
                       m_font_large,
                       gl::COLOR_BLACK, glm::vec2(30, 30));
        
        gl::drawText2D(" (" + as_string(m_analog_in[0].min()) +
                       " - " + as_string(m_analog_in[0].max()) + ")",
                       m_font_small,
                       gl::COLOR_BLACK, glm::vec2(30, 90));
        
        gl::drawText2D(" mean: " + as_string(m_analog_in[0].mean(), 2),
                       m_font_small,
                       gl::COLOR_BLACK, glm::vec2(30, 110));
        
        gl::drawText2D(" standard deviation: " + as_string(m_analog_in[0].standard_deviation(), 2),
                       m_font_small,
                       gl::COLOR_BLACK, glm::vec2(30, 130));
    }
    
    void resize(int w ,int h)
    {
        m_ortho_cam->right(w);
        m_ortho_cam->top(h);
    }
    
    void keyPress(const KeyEvent &e)
    {
        ViewerApp::keyPress(e);
        
        if(e.getChar() == KeyEvent::KEY_c){ m_serial.setup(); }
    
    }
    
    void got_message(const std::string &the_message)
    {
        LOG_INFO<<the_message;
    }
    
    void tearDown()
    {
        LOG_PRINT<<"ciao serialMonitor";
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
