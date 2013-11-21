#include "kinskiApp/ViewerApp.h"
#include "kinskiApp/AppServer.h"
#include "kinskiCore/Serial.h"

#include "RtMidi.h"
#include "Measurement.h"
#include "DMXController.h"

using namespace std;
using namespace kinski;
using namespace glm;

typedef std::shared_ptr<RtMidiIn> RtMidiInPtr;
typedef std::shared_ptr<RtMidiOut> RtMidiOutPtr;

typedef std::map<uint32_t, std::list<uint8_t>> MidiMap;

class SerialMonitorSample : public ViewerApp
{
private:
    
    gl::Font m_font_large, m_font_small;
    
    // Serial communication with Arduino device
    Serial m_serial;
    
    // Communication with Enttec DMXUSB Pro
    DMXController m_dmx_control;
    
    // used for analog input measuring
    string m_input_prefix = "analog_";
    std::vector<Measurement<float>> m_analog_in {   Measurement<float>("String 1"),
                                                    Measurement<float>("String 2"),
                                                    Measurement<float>("String 3"),
                                                    Measurement<float>("String 4"),
                                                    Measurement<float>("String 5"),
                                                    Measurement<float>("String 6"),
                                                    Measurement<float>("String 7"),
                                                    Measurement<float>("String 8")
                                                };
    std::vector<bool> m_channel_activity {16};
    
    // display plot for selected index
    RangedProperty<int>::Ptr m_selected_index;
    
    // used for data rendering
    vector<vec3> m_points;
    gl::OrthographicCamera::Ptr m_ortho_cam;
    
    // midi output
    RtMidiOutPtr m_midi_out {new RtMidiOut()};
    std::vector<unsigned char> m_midi_msg;
    
    // midi properties
    Property_<string>::Ptr m_midi_port_name;
    Property_<int>::Ptr m_midi_channel, m_midi_note, m_midi_velocity;
    MidiMap m_midi_map;
    
    // thresholds
    uint32_t m_thresh_low = 10, m_thresh_high = 80;
    
    // array to keep track of note_on events
    std::vector<bool> m_midi_note_on_array {128, false};
    
    float m_time_accum = 0;
    int m_note_on = 0;
    
public:
    
    /////////////////////////////////////////////////////////////////
    
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
        
        m_midi_port_name = Property_<string>::create("Midi virtual port name", "Baumhafer");
        registerProperty(m_midi_port_name);
        
        m_midi_channel = Property_<int>::create("Midi channel", 0);
        registerProperty(m_midi_channel);
        
        m_midi_note = Property_<int>::create("Midi note", 0);
        registerProperty(m_midi_note);
        
        m_midi_velocity = Property_<int>::create("Midi velocity", 0);
        registerProperty(m_midi_velocity);
        
        observeProperties();
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
        
        m_channel_activity.resize(16, false);
        
        // init midi output
        LOG_INFO<<"found "<<m_midi_out->getPortCount()<<" midi-outs";
        LOG_INFO<<"openening virtual midi-port: '"<<*m_midi_port_name<<"'";
        m_midi_out->openVirtualPort(*m_midi_port_name);
        
        // mtc message !?
        m_midi_msg.resize(2);
        m_midi_msg[0] = 0xF1;
        m_midi_msg[1] = 60;
        m_midi_out->sendMessage( &m_midi_msg );
        
        // setup midimap
        m_midi_map[0] = {48, 55, 60}; // C-1 G-1 C0
        m_midi_map[1] = {50, 57, 62}; // D-1 A-1 D0
        m_midi_map[2] = {52, 59, 64}; // E-1 H-1 E0
        m_midi_map[3] = {53, 60, 65}; // F-1 C0 F0
        m_midi_map[4] = {55, 62, 67}; // G-1 D0 G0
        m_midi_map[5] = {57, 64, 69}; // A-1 E0 A0
        m_midi_map[6] = {59, 66, 71}; // H-1 F#0 H0
        m_midi_map[7] = {60, 67, 72}; // C0 G0 C1
        
        m_dmx_control.set_num_active_channels(2);
    }
    
    /////////////////////////////////////////////////////////////////
    
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
        
        for(int i = 0; i < 2/*m_analog_in.size()*/; i++)
        {
            if(m_analog_in[i].last_value() > m_thresh_high && !m_channel_activity[i])
            {
                m_channel_activity[i] = true;
                play_string(i);
            }
            else if(m_analog_in[i].last_value() < m_thresh_low && m_channel_activity[i])
            {
                m_channel_activity[i] = false;
                stop_string(i);
            }
        }
        
        const auto &measure = m_analog_in[*m_selected_index];
        
        m_points.resize(measure.history().size(), vec3(0));
        for (int i = 0; i < measure.history().size(); i++)
        {
            m_points[i].x = i * windowSize().x / measure.history().size();
            m_points[i].y = measure.history()[i] / 2.f;
        }
        
        // send midi-events in 2 sec interval
//        m_time_accum += timeDelta;
//        if(m_time_accum > 2.f)
//        {
//            if(m_note_on >= 0)
//            {
//                stop_string(m_note_on);
//                m_note_on = -1;
//            }
//            else
//            {
//                m_note_on = kinski::random(0, 8);
//                play_string(m_note_on);
//            }
//            m_time_accum = 0;
//        }
        
        // send DMX events
        //TODO: use less frequent intervals here
        if(m_dmx_control.serial().isInitialized())
        {
            // send dmx-values
            for(int i = 0; i < m_analog_in.size(); i++)
            {
                m_dmx_control.values()[i] = static_cast<uint8_t>(255 * m_analog_in[i].last_value() / 1023.f);
            }
            m_dmx_control.update();
        }
    }
    
    /////////////////////////////////////////////////////////////////
    
    void draw()
    {
        const auto &measure = m_analog_in[*m_selected_index];
        
        float play_head_x_pos = measure.current_index() * windowSize().x /
                                measure.history().size();
        gl::drawLine(vec2(play_head_x_pos, 0), vec2(play_head_x_pos, windowSize().y), gl::COLOR_BLACK);
        
        gl::setProjection(m_ortho_cam);
        
        gl::drawLineStrip(m_points, gl::COLOR_BLACK);
        //gl::drawLines(m_points, gl::COLOR_BLACK, 15.f);
        
        auto measured_val = measure.last_value();
        float volt_value = 5.f * measured_val / 1023.f;
        
        gl::drawQuad(gl::COLOR_OLIVE,
                     vec2(80, measured_val / 2),
                     vec2(windowSize().x - 100, windowSize().y - measured_val / 2));
        
        gl::drawText2D(measure.description() + " (" + as_string(m_selected_index->value()) +"): " +
                            as_string(volt_value, 2) + " V",
                       m_font_large,
                       gl::COLOR_BLACK, glm::vec2(30, 30));
        
        gl::drawText2D(" (" + as_string(measure.min()) +
                       " - " + as_string(measure.max()) + ")",
                       m_font_small,
                       gl::COLOR_BLACK, glm::vec2(50, 110));
        
        gl::drawText2D(" mean: " + as_string(measure.mean(), 2),
                       m_font_small,
                       gl::COLOR_BLACK, glm::vec2(50, 130));
        
        gl::drawText2D(" standard deviation: " + as_string(measure.standard_deviation(), 2),
                       m_font_small,
                       gl::COLOR_BLACK, glm::vec2(50, 150));
    }
    
    /////////////////////////////////////////////////////////////////
    
    void resize(int w ,int h)
    {
        ViewerApp::resize(w, h);
        
        m_ortho_cam->right(w);
        m_ortho_cam->top(h);
    }
    
    /////////////////////////////////////////////////////////////////
    
    void keyPress(const KeyEvent &e)
    {
        ViewerApp::keyPress(e);
        
        //note_on(0);
        
        switch (e.getChar())
        {
            case KeyEvent::KEY_c:
                m_serial.setup("/dev/tty.usbmodemfd121", 57600);
                break;
                
            case KeyEvent::KEY_r:
                for(auto &measure : m_analog_in){measure.reset();}
                break;
                
            case KeyEvent::KEY_1:
            case KeyEvent::KEY_2:
            case KeyEvent::KEY_3:
            case KeyEvent::KEY_4:
            case KeyEvent::KEY_5:
            case KeyEvent::KEY_6:
            case KeyEvent::KEY_7:
            case KeyEvent::KEY_8:
                *m_selected_index = string_as<int>(as_string(e.getChar())) - 1;
                break;
            
            case KeyEvent::KEY_n:
                play_string(0);
                break;
            default:
                break;
        }
    }
    
    /////////////////////////////////////////////////////////////////
    
    void keyRelease(const KeyEvent &e)
    {
        ViewerApp::keyRelease(e);
        //note_off(0);
        
        switch (e.getChar())
        {
            case KeyEvent::KEY_n:
                stop_string(0);
                break;
            default:
                break;
        }
    }
    
    /////////////////////////////////////////////////////////////////
    
    void got_message(const std::string &the_message)
    {
        LOG_INFO<<the_message;
    }
    
    /////////////////////////////////////////////////////////////////
    
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
        
        parsed_index = string_as<int>(tokens[0].substr(m_input_prefix.size()));
        
        // return if parsed index is out of bounds
        if(parsed_index < 0 || parsed_index >= m_analog_in.size()) return;
        
        m_analog_in[parsed_index].push(string_as<int>(tokens[1]));
    }
    
    /////////////////////////////////////////////////////////////////
    
    void play_string(uint32_t ch)
    {
        auto iter = m_midi_map.find(ch);
        
        // search midi_map for our notes to play
        if(iter == m_midi_map.end())
        {
            // not found
            LOG_ERROR<<"no midi notes defined for channel: "<<ch;
            return;
        }
        const auto &note_list = iter->second;
        LOG_DEBUG<<"note_on: "<< ch << " -> " << (int)note_list.front();
        
        for(auto note : note_list)
        {
            midi_note_on(note);
        }
    }
    
    /////////////////////////////////////////////////////////////////
    
    void stop_string(uint32_t ch)
    {
        auto iter = m_midi_map.find(ch);
        
        // search midi_map for our notes to play
        if(iter == m_midi_map.end())
        {
            // not found
            LOG_ERROR<<"no midi notes defined for channel: "<<ch;
            return;
        }
        const auto &note_list = iter->second;
        LOG_DEBUG<<"note_off: "<< ch << " -> " << (int)note_list.front();
        
        for(auto note : note_list)
        {
            midi_note_off(note);
        }
    }
    
    /////////////////////////////////////////////////////////////////
    
    void midi_note_on(uint8_t note, uint8_t velocity = 127)
    {
        // assert correct size
        m_midi_msg.resize(3);
        
        // note already on, first fire a note-off event
        if(m_midi_note_on_array[note]){midi_note_off(note);}
        
        m_midi_note_on_array[note] = true;
        
        //Note On: 144, 64, 90
        m_midi_msg[0] = 144;
        m_midi_msg[1] = note;
        m_midi_msg[2] = velocity;
        m_midi_out->sendMessage( &m_midi_msg );
    }
    
    /////////////////////////////////////////////////////////////////
    
    void midi_note_off(uint8_t note, uint8_t velocity = 127)
    {
        //nothing to do, note already off
        if(!m_midi_note_on_array[note]) return;
        
        // assert correct size
        m_midi_msg.resize(3);
        
        m_midi_note_on_array[note] = false;
        
        // Note Off: 128, 64, 40
        m_midi_msg[0] = 128;
        m_midi_msg[1] = note;
        m_midi_msg[2] = velocity;
        m_midi_out->sendMessage( &m_midi_msg );
    }
    
    /////////////////////////////////////////////////////////////////
    
    void midi_mute_all()
    {
        for (int i = 0; i < m_midi_note_on_array.size(); i++)
        {
            if(m_midi_note_on_array[i]){ midi_note_off(i); }
        }
    }
    
    /////////////////////////////////////////////////////////////////
    
    void updateProperty(const Property::ConstPtr &theProperty)
    {
        ViewerApp::updateProperty(theProperty);
        
        if(theProperty == m_midi_note)
        {
            midi_note_off(0);
            m_midi_map[0] = {*m_midi_note};
        }
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
