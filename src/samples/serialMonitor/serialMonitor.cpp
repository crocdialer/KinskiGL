#include "app/ViewerApp.h"
#include "core/Serial.h"

#include "RtMidi.h"
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
    
    net::udp_server m_udp_server;
    Property_<uint32_t>::Ptr m_local_udp_port = Property_<uint32_t>::create("udp port", 11111);
    
    // Communication with Enttec DMXUSB Pro
    DMXController m_dmx_control;
    
    Property_<string>::Ptr m_arduino_device_name;
    
    // used for analog input measuring
    string m_input_prefix = "a_";
    
    std::vector<Measurement<float>> m_analog_in {   Measurement<float>("Harp 1 - 1"),
                                                    Measurement<float>("Harp 1 - 2"),
                                                    Measurement<float>("Harp 1 - 3"),
                                                    Measurement<float>("Harp 1 - 4"),
                                                    Measurement<float>("Harp 1 - 5"),
                                                    Measurement<float>("Harp 1 - 6"),
                                                    Measurement<float>("Harp 1 - 7"),
                                                    Measurement<float>("Harp 1 - 8")
                                                };
    std::vector<bool> m_channel_activity;
    
    // display plot for selected index
    RangedProperty<int>::Ptr m_selected_index;
    
    // used for data rendering
    vector<vec3> m_points;
    gl::OrthographicCamera::Ptr m_ortho_cam;
    
    // midi output
    Property_<uint32_t>::Ptr m_midi_start_note = Property_<uint32_t>::create("Midi start note", 48);
    RtMidiOutPtr m_midi_out {new RtMidiOut()};
    std::vector<unsigned char> m_midi_msg;
    
    // midi properties
    Property_<string>::Ptr m_midi_port_name;
    
    /*!
     * Used for notes played when idle, or for all notes, if m_midi_fixed_velocity is true
     */
    RangedProperty<int>::Ptr m_midi_velocity = RangedProperty<int>::create("Midi velocity", 127, 0, 127);
    
    Property_<bool>::Ptr m_midi_fixed_velocity = Property_<bool>::create("Use fixed velocity", true);
    Property_<bool>::Ptr m_midi_autoplay = Property_<bool>::create("Midi autoplay", false);
    RangedProperty<float>::Ptr m_midi_plug_multiplier =
        RangedProperty<float>::create("Midi plug multiplier", 1.f, 0.f, 10.f);
    MidiMap m_midi_map;
    
    // thresholds
    Property_<uint32_t>::Ptr m_thresh_low;// = 10,
    Property_<uint32_t>::Ptr m_thresh_high;// = 80;
    
    // DMX properties
    //RangedProperty<int>::Ptr m_dmx_red, m_dmx_green, m_dmx_blue;
    RangedProperty<uint32_t>::Ptr m_dmx_min_val = RangedProperty<uint32_t>::create("DMX min", 0, 0, 255);
    RangedProperty<uint32_t>::Ptr m_dmx_max_val = RangedProperty<uint32_t>::create("DMX max", 255, 0, 255);
    RangedProperty<float>::Ptr m_dmx_smoothness = RangedProperty<float>::create("DMX smoothness", .8, 0, 1);
    RangedProperty<uint32_t>::Ptr m_dmx_idle_max_val = RangedProperty<uint32_t>::create("DMX idle max", 25, 0, 255);
    Property_<float>::Ptr m_dmx_idle_speed = Property_<float>::create("DMX idle speed", 1.f);
    Property_<float>::Ptr m_dmx_idle_spread = Property_<float>::create("DMX idle spread", 0.1f);
    Property_<int>::Ptr m_dmx_start_index = Property_<int>::create("DMX start index", 1);
    RangedProperty<uint32_t>::Ptr m_dmx_extra_1 = RangedProperty<uint32_t>::create("DMX extra 1", 0, 0, 255),
    m_dmx_extra_2 = RangedProperty<uint32_t>::create("DMX extra 2", 0, 0, 255);
    
    
    // array to keep track of note_on events
    std::vector<bool> m_midi_note_on_array;
    
    //
    bool m_idle_active = false;
    
    // idle time in seconds
    Property_<float>::Ptr m_idle_time = Property_<float>::create("Idle timeout", 10.f);
    
    // time a note will play during idle
    Property_<float>::Ptr m_idle_note_duration = Property_<float>::create("Idle note duration", 3.f);
    
    // current idle note played
    int m_note_on = 0;

    // idle timer
    boost::asio::deadline_timer m_idle_timer{io_service()}, m_note_off_timer{io_service()};
    
    std::vector<gl::Texture> m_textures{4};
    
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
        
        m_arduino_device_name = Property_<string>::create("Arduino device name",
                                                          "/dev/tty.usbmodem1411");
        registerProperty(m_arduino_device_name);
        
        m_selected_index = RangedProperty<int>::create("selected index", 0, 0,
                                                       m_analog_in.size() - 1);
        registerProperty(m_selected_index);
        
        // udp server
        m_udp_server = net::udp_server(io_service(), std::bind(&SerialMonitorSample::got_message,
                                                               this, std::placeholders::_1));
        m_udp_server.start_listen(11111);
        
        // register midi properties
        m_midi_port_name = Property_<string>::create("Midi virtual port name", "Baumhafer");
        registerProperty(m_midi_port_name);
        registerProperty(m_midi_plug_multiplier);
        registerProperty(m_midi_velocity);
        registerProperty(m_midi_fixed_velocity);
        registerProperty(m_midi_autoplay);
        registerProperty(m_midi_start_note);
        
        // register DMX properties
        registerProperty(m_dmx_start_index);
        registerProperty(m_dmx_min_val);
        registerProperty(m_dmx_max_val);
        registerProperty(m_dmx_smoothness);
        registerProperty(m_dmx_idle_max_val);
        registerProperty(m_dmx_idle_speed);
        registerProperty(m_dmx_idle_spread);
        registerProperty(m_dmx_extra_1);
        registerProperty(m_dmx_extra_2);
        
        // register idle properties
        registerProperty(m_idle_time);
        registerProperty(m_idle_note_duration);
        
        m_thresh_low = Property_<uint32_t>::create("thresh low", 10);
        registerProperty(m_thresh_low);
        
        m_thresh_high = Property_<uint32_t>::create("thresh high", 80);
        registerProperty(m_thresh_high);
        
        observeProperties();
        create_tweakbar_from_component(shared_from_this());
        displayTweakBar(false);
        
        // drain the serial buffer before we start
        m_serial.drain();
        m_serial.flush();
        
        m_ortho_cam = gl::OrthographicCamera::create(0, windowSize().x, 0, windowSize().y, 0, 1);
        
        for(auto &m : m_analog_in)
        {
            m.set_filter(std::make_shared<FalloffFilter<float>>());
        }
        
        
        m_channel_activity.assign(m_analog_in.size(), false);
        
        m_textures[0] = gl::createTextureFromFile("harp_icon.png");
        
        // init midi output
        LOG_INFO<<"found "<<m_midi_out->getPortCount()<<" midi-outs";
        LOG_INFO<<"openening virtual midi-port: '"<<m_midi_port_name->value()<<"'";
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
        
        m_midi_note_on_array.resize(128, false);
        
        // restore our settings
        load_settings();
    }
    
    /////////////////////////////////////////////////////////////////
    
    void update(float timeDelta)
    {
        ViewerApp::update(timeDelta);
        
        // parse arduino input
        if(m_serial.isInitialized())
        {
            for(string line : m_serial.read_lines())
            {
                parse_line(line);
            }
        }
        
        // apply current state
        for(int i = 0; i < m_analog_in.size(); i++)
        {
            if(m_analog_in[i].last_value() > *m_thresh_high &&
               !m_channel_activity[i])
            {
                m_channel_activity[i] = true;
                int string_velocity = kinski::clamp<float>(*m_midi_plug_multiplier * 127 *
                                                           m_analog_in[i].last_value() / 1023.f,
                                                           0,
                                                           127);
                
                play_string(i, *m_midi_fixed_velocity ? *m_midi_velocity : string_velocity);
                
                // we got some clients, so reset idle timer
                if(*m_midi_autoplay)
                {
                    m_idle_active = false;
                    reset_idle_timer();
                };
            }
            else if(!m_idle_active && m_analog_in[i].last_value() < *m_thresh_low &&
                    m_channel_activity[i])
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
        
        // light control
        update_dmx();
    }
    
    /////////////////////////////////////////////////////////////////
    
    void draw()
    {
        // background icon
        gl::drawTexture(m_textures[0], vec2(256),
                        vec2(10, windowSize().y - 256));
        
        const auto &measure = m_analog_in[*m_selected_index];
        
        float play_head_x_pos = measure.current_index() * windowSize().x /
                                measure.history().size();
        gl::drawLine(vec2(play_head_x_pos, 0), vec2(play_head_x_pos, windowSize().y), gl::COLOR_BLACK);
        
        gl::setProjection(m_ortho_cam);
        
        gl::drawLineStrip(m_points, vec4(1) - clear_color());
        //gl::drawLines(m_points, gl::COLOR_BLACK, 15.f);
        
        auto measured_val = measure.last_value();
        float volt_value = 5.f * measured_val / 1023.f;
        
//        gl::drawQuad(gl::COLOR_OLIVE,
//                     vec2(80, measured_val / 2),
//                     vec2(windowSize().x - 100, windowSize().y - measured_val / 2));
        
        gl::drawText2D(measure.description() + " (" + as_string(m_selected_index->value()) +"): " +
                            as_string(volt_value, 2) + " V",
                       m_font_large,
                       gl::COLOR_BLACK, glm::vec2(30, 30));
        
        gl::drawText2D(" (" + as_string(measure.min()) +
                       " - " + as_string(measure.max()) + ")",
                       m_font_small,
                       gl::COLOR_BLACK, glm::vec2(50, 110));
        
        // string activity icons
        int icon_width = 25;
        vec2 offset(windowSize().x - 290, 120), step(icon_width + 8, 0);
        for(int i = 0; i < m_analog_in.size(); i++)
        {
            gl::drawQuad(m_channel_activity[i] ? gl::COLOR_DARK_RED : gl::Color(.2),
                         m_channel_activity[i] ? vec2(icon_width + 6) : vec2(icon_width),
                         offset - (m_channel_activity[i] ? vec2(3) : vec2(0)));
            offset += step;
        }
        
        // dmx activity icons
        offset = vec2(windowSize().x - 290, 160);
        for(int i = 0; i < m_analog_in.size(); i++)
        {
            gl::drawQuad(gl::Color(.2),
                         vec2(icon_width, 5 + m_dmx_control[*m_dmx_start_index + 2 * i]), offset);
            offset += step;
        }
    }
    
    /////////////////////////////////////////////////////////////////
    
    void resize(int w ,int h)
    {
        ViewerApp::resize(w, h);
        
        m_ortho_cam->right(w);
        m_ortho_cam->top(h);
        
        set_clear_color(clear_color());
    }
    
    /////////////////////////////////////////////////////////////////
    
    void keyPress(const KeyEvent &e)
    {
        ViewerApp::keyPress(e);
        
        int number = 0;
        switch (e.getCode())
        {
            case GLFW_KEY_C:
                m_arduino_device_name->notifyObservers();
                break;
                
            case GLFW_KEY_R:
                for(auto &measure : m_analog_in){measure.reset();}
                break;
                
            case GLFW_KEY_1:
            case GLFW_KEY_2:
            case GLFW_KEY_3:
            case GLFW_KEY_4:
            case GLFW_KEY_5:
            case GLFW_KEY_6:
            case GLFW_KEY_7:
            case GLFW_KEY_8:
                number = string_as<int>(as_string(e.getChar())) - 1;
                if(e.isAltDown())
                {
//                    m_idle_active = false;
//                    midi_mute_all();
                    play_string(number, *m_midi_velocity);
                }
                else
                {
                    *m_selected_index = number
                    + (e.isShiftDown() ? 8 : 0);
                }
                break;
            case GLFW_KEY_M:
                for (int i = 0; i < m_channel_activity.size(); i++) { m_channel_activity[i] = false; }
                midi_mute_all();
                break;
            default:
                break;
        }
    }
    
    /////////////////////////////////////////////////////////////////
    
    void keyRelease(const KeyEvent &e)
    {
        ViewerApp::keyRelease(e);

        switch (e.getChar())
        {
            case GLFW_KEY_N:
                stop_string(0);
                break;
            default:
                break;
        }
    }
    
    /////////////////////////////////////////////////////////////////
    
    void got_message(const std::vector<uint8_t> &the_message)
    {
        std::string msg = std::string(the_message.begin(), the_message.end());
        parse_line(msg);
    }
    
    /////////////////////////////////////////////////////////////////
    
    void tearDown()
    {
        midi_mute_all();
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
    
    void play_string(uint32_t ch, int velocity)
    {
        // search midi_map for our notes to play
        auto iter = m_midi_map.find(ch);
        
        // not found
        if(iter == m_midi_map.end())
        {
            LOG_ERROR<<"no midi notes defined for channel: "<<ch;
            return;
        }
        const auto &note_list = iter->second;
        //LOG_DEBUG<<"note_on: "<< ch << " -> " << (int)note_list.front();
        
        m_channel_activity[ch] = true;
        
        for(auto note : note_list)
        {
            midi_note_on(note, velocity);
        }
    }
    
    /////////////////////////////////////////////////////////////////
    
    void stop_string(uint32_t ch)
    {
        // search midi_map for our notes to play
        auto iter = m_midi_map.find(ch);
        
        // not found
        if(iter == m_midi_map.end())
        {
            LOG_ERROR<<"no midi notes defined for channel: "<<ch;
            return;
        }
        const auto &note_list = iter->second;
        //LOG_DEBUG<<"note_off: "<< ch << " -> " << (int)note_list.front();
        
        m_channel_activity[ch] = false;
        
        for(auto note : note_list)
        {
            midi_note_off(note);
        }
        
    }
    
    /////////////////////////////////////////////////////////////////
    
    void midi_note_on(uint32_t note, uint32_t velocity = 127)
    {
        LOG_DEBUG<<"note_on: "<< note << " : " << velocity;
        
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
    
    void midi_note_off(uint32_t note, uint32_t velocity = 127)
    {
        LOG_DEBUG<<"note_off: "<< note << " : " << velocity;
        
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
    
    void update_dmx()
    {
        if(m_idle_active)
        {
            float phase = *m_dmx_idle_speed * getApplicationTime();
            
            
            for (int i = 0; i < m_analog_in.size() * 2; i++)
            {
                float sin_val = (sinf(*m_dmx_idle_spread * i + phase) + 1) / 2.f;
                
                float map_val = map_value<float>(sin_val,
                                                 0, 1.f, *m_dmx_min_val, *m_dmx_idle_max_val);
                
                uint8_t dmx_val = map_val;
                
                if(m_channel_activity[i / 2])
                {
                    dmx_val = *m_dmx_max_val;
                }
                    
                m_dmx_control[*m_dmx_start_index + i] = kinski::mix(m_dmx_control[*m_dmx_start_index + i],
                                                                    dmx_val,
                                                                    1.f - *m_dmx_smoothness);
            }
        }
        else
        {
            for (int i = 0; i < m_analog_in.size(); i ++)
            {
                uint8_t dmx_val = m_channel_activity[i] ? *m_dmx_max_val : *m_dmx_min_val;
                
                m_dmx_control[*m_dmx_start_index + (2 * i)] = kinski::mix(m_dmx_control[*m_dmx_start_index + (2 * i)],
                                                                          dmx_val,
                                                                          1.f - *m_dmx_smoothness);
                
                m_dmx_control[*m_dmx_start_index + (2 * i + 1)] = kinski::mix(m_dmx_control[*m_dmx_start_index + (2 * i + 1)],
                                                                              dmx_val,
                                                                              1.f - *m_dmx_smoothness);
            }
        }
        
        m_dmx_control[*m_dmx_start_index + m_analog_in.size() * 2] = *m_dmx_extra_1;
        m_dmx_control[*m_dmx_start_index + m_analog_in.size() * 2 + 1] = *m_dmx_extra_2;
        
        m_dmx_control.update();
    }
    
    void reset_idle_timer()
    {
        m_idle_timer.expires_from_now(boost::posix_time::seconds(*m_idle_time));
        m_idle_timer.async_wait([&](const boost::system::error_code &error)
        {
            // Timer expired regularly
            if (!error)
            {
                m_idle_active = true;
                midi_mute_all();
                
                m_note_on = kinski::random(0, 8);
                play_string(m_note_on, *m_midi_velocity);
                
                // setup our note_off timer
                m_note_off_timer.expires_from_now(boost::posix_time::seconds(*m_idle_note_duration));
                m_note_off_timer.async_wait([&](const boost::system::error_code &error)
                                            {
                                                if (!error)
                                                {
                                                    stop_string(m_note_on);
                                                    m_note_on = -1;
                                                }
                                            });
                reset_idle_timer();
            }
        });
    }
    
    /////////////////////////////////////////////////////////////////
    
    void updateProperty(const Property::ConstPtr &theProperty)
    {
        ViewerApp::updateProperty(theProperty);
        
        if(theProperty == m_arduino_device_name)
        {
            if(m_arduino_device_name->value().empty())
                m_serial.setup(0, 115200);
            else
                m_serial.setup(*m_arduino_device_name, 115200);
        }
        else if (theProperty == m_midi_autoplay)
        {
            if(*m_midi_autoplay)
            {
                reset_idle_timer();
            }
            else
            {
                m_idle_active = false;
                m_idle_timer.cancel();
            }
        }
        else if (theProperty == m_dmx_min_val || theProperty == m_dmx_max_val)
        {
            try{m_dmx_idle_max_val->setRange(*m_dmx_min_val, *m_dmx_max_val);}
            catch(Exception &e){LOG_ERROR<<e.what();}
        }
        else if (theProperty == m_dmx_start_index)
        {
            for (int i = 0; i < 513; i++) { m_dmx_control[i] = 0;}
        }
        else if (theProperty == m_midi_start_note)
        {
            uint8_t val = *m_midi_start_note;
            
            // setup midimap
            m_midi_map[0] = {val, uint8_t(val + 7), uint8_t(val + 12)};
            m_midi_map[1] = {uint8_t(val + 2), uint8_t(val + 9), uint8_t(val + 14)};
            m_midi_map[2] = {uint8_t(val + 4), uint8_t(val + 11), uint8_t(val + 16)};
            m_midi_map[3] = {uint8_t(val + 5), uint8_t(val + 12), uint8_t(val + 17)};
            m_midi_map[4] = {uint8_t(val + 7), uint8_t(val + 14), uint8_t(val + 19)};
            m_midi_map[5] = {uint8_t(val + 9), uint8_t(val + 16), uint8_t(val + 21)};
            m_midi_map[6] = {uint8_t(val + 11), uint8_t(val + 18), uint8_t(val + 23)};
            m_midi_map[7] = {uint8_t(val + 12), uint8_t(val + 19), uint8_t(val + 24)};
            
            // make sure leftover notes are purged
            midi_mute_all();
        }
        else if(theProperty == m_local_udp_port)
        {
            m_udp_server.start_listen(*m_local_udp_port);
        }
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new SerialMonitorSample);
    LOG_INFO<<"local ip: " << net::local_ip();
    return theApp->run();
}
