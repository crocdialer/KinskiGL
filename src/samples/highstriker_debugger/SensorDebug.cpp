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

    auto font_path = "Courier New Bold.ttf";

    fonts()[FONT_MEDIUM].load(font_path, 32);
    fonts()[FONT_LARGE].load(font_path, 64);

    register_property(m_serial_device_name);
    register_property(m_range_min_max);
    register_property(m_timeout_game_ready);
    register_property(m_sensor_refresh_rate);

    observe_properties();
    add_tweakbar_for_component(shared_from_this());

    // holds 10 uint16_t sensor-values
    m_sensor_vals.resize(10);

    // measure history for our 10 sensors
    m_measurements.resize(10, Measurement<float>(500));

    // buffer incoming bytes from serial connection
    m_serial_read_buf.resize(2048);

    // setup a recurring timer for sensor-refresh-rate measurement
    m_timer_sensor_refresh = Timer(io_service(), [this]()
    {
        *m_sensor_refresh_rate = m_sensor_refresh_count;
        m_sensor_refresh_count = 0;
    });
    m_timer_sensor_refresh.set_periodic();
    m_timer_sensor_refresh.expires_from_now(1.f);

    m_timer_game_ready = Timer(io_service(), [](){ LOG_DEBUG << "READY"; });

    if(!load_settings()){ save_settings(); }
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
    vec2 offset(55, 110), step(0, 90);
    std::vector<glm::vec3> points;

    float h = 80.f, w = gl::window_dimension().x - 2.f * offset.x;

    for(size_t i = 0; i < m_measurements.size(); i++)
    {
        float val = (float) m_measurements[i].last_value();
        if(!m_sensor_vals[i]){ continue; }

        // rectangle for current value
        gl::draw_quad(gl::COLOR_GRAY, vec2(val * w, h), offset);

        gl::draw_text_2D(as_string(100.f * val, 2) + "%", fonts()[FONT_MEDIUM], gl::COLOR_WHITE,
                         offset + vec2(val * w, 0));

        ////////////////////////////// measure history ///////////////////////////

        // generate point array for linestrip
        points.resize(m_measurements[i].history_size());

        for (size_t j = 0, sz = m_measurements[i].history_size(); j < sz; j += 2)
        {
            float x_val = offset.x + j / (float) sz * w;
            float y_val = gl::window_dimension().y - offset.y - h;

            points[sz - 1 - j] = vec3(x_val, y_val, 0.f);
            points[sz - 2 - j] = vec3(x_val, y_val + h * m_measurements[i][j], 0.f);
        }
        gl::draw_lines_2D(points, gl::COLOR_WHITE);

        offset += step;
    }

    // global average
    gl::draw_text_2D(as_string(100.f * m_sensor_last_avg, 2) + "%", fonts()[FONT_LARGE],
                     gl::COLOR_WHITE, vec2(45));

    // final score
    uint16_t final_score = (uint16_t)round(map_value<float>(m_sensor_last_avg, m_range_min_max->value().x,
                                                  m_range_min_max->value().y, 0, 999));
    gl::draw_text_2D("score: " + as_string(final_score), fonts()[FONT_LARGE], gl::COLOR_RED,
                     vec2(330, 45));


    if(m_timer_game_ready.has_expired())
    {
        if(final_score)
        {
            m_timer_game_ready.expires_from_now(*m_timeout_game_ready);
            auto color_highscore = gl::COLOR_RED; color_highscore.a = .3f;

            if(final_score == 999){ gl::draw_quad(color_highscore, gl::window_dimension()); }
        }
        else
        {
            auto color_ready = gl::COLOR_GREEN; color_ready.a = .3f;
            gl::draw_quad(color_ready, vec2(70), vec2(gl::window_dimension().x - 100, 25));
        }
    }
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
    LOG_DEBUG << "press: " << e.getCode();
}

/////////////////////////////////////////////////////////////////

void SensorDebug::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
    LOG_DEBUG << "release: " << e.getCode();
}

/////////////////////////////////////////////////////////////////

void SensorDebug::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
    LOG_DEBUG << "press: " << e.getX() << " - " << e.getY();
}

/////////////////////////////////////////////////////////////////

void SensorDebug::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
    LOG_DEBUG << "release: " << e.getX() << " - " << e.getY();
}

/////////////////////////////////////////////////////////////////

void SensorDebug::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
    LOG_DEBUG << "move: " << e.getX() << " - " << e.getY();
}

/////////////////////////////////////////////////////////////////

void SensorDebug::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
    LOG_DEBUG << "drag: " << e.getX() << " - " << e.getY();
}

/////////////////////////////////////////////////////////////////

void SensorDebug::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
    LOG_DEBUG << "wheel: " << e.getWheelIncrement().y;
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
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void SensorDebug::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);

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
                int num_active_panels = 0;

                for(size_t i = 0; i < m_sensor_vals.size(); i++)
                {
                    m_measurements[i].push((float)m_sensor_vals[i] / std::numeric_limits<uint16_t>::max());
                }

                float sum = 0.f;

                for(size_t i = 0; i < m_measurements.size(); i++)
                {
                    if(m_sensor_vals[i])
                    {
                        num_active_panels++;
                        sum += (float) m_measurements[i].last_value();
                    }
                }
//                if(m_timer_game_ready.has_expired())
                {
                    m_sensor_last_avg = sum / (num_active_panels ? num_active_panels : 1);
                }
            }
        }
    }
}
