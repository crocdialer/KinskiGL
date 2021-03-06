// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "App.hpp"

#include <thread>
#include <mutex>
#include <boost/asio/io_service.hpp>

using namespace std;

// 1 double per second
using duration_t = std::chrono::duration<double>;

// explicit template instantiation for some vec types
template class crocore::Property_<glm::vec2>;
template class crocore::Property_<glm::vec3>;
template class crocore::Property_<glm::vec4>;

namespace kinski
{

App::App(int argc, char *argv[]):
Component(argc ? crocore::fs::get_filename_part(argv[0]) : "KinskiApp"),
m_framesDrawn(0),
m_lastMeasurementTimeStamp(0.0),
m_framesPerSec(0.f),
m_timingInterval(1.0),
m_running(false),
m_fullscreen(false),
m_display_gui(false),
m_cursorVisible(true),
m_max_fps(60.f),
m_main_queue(0),
m_background_queue(2)
{
    srand(clock());
    for(int i = 0; i < argc; i++){ m_args.push_back(argv[i]); }
}

App::~App()
{

}

int App::run()
{
    try{init(); m_running = GL_TRUE;}
    catch(std::exception &e){LOG_ERROR<<e.what();}
    double time_stamp = 0.0;

    // Main loop
    while(m_running)
    {
        // update application time
        time_stamp = get_application_time();

        // poll io_service if no seperate worker-threads exist
        if(!m_main_queue.num_threads()) m_main_queue.poll();

        // poll input events
        poll_events();

        // time elapsed since last frame
        double time_delta = time_stamp - m_lastTimeStamp;

        // call update callback
        update(time_delta);

        m_lastTimeStamp = time_stamp;

        if(needs_redraw())
        {
            // call draw callback
            draw_internal();

            // Swap front and back rendering buffers
            swap_buffers();
        }

        // perform fps-timing
        timing(time_stamp);

        // post draw hook
        post_draw();

        // Check if ESC key was pressed or window was closed or whatever
        m_running = is_running();

        // fps managment
        float current_fps = 1.f / time_delta;

        if(current_fps > m_max_fps)
        {
            double sleep_secs = std::max(0.0, (1.0 / m_max_fps - time_delta));
            this_thread::sleep_for(duration_t(sleep_secs));
        }
    }

    // manage teardown, save stuff etc.
    teardown();

    return EXIT_SUCCESS;
}

void App::set_window_size(const glm::vec2 &size)
{
    if(running())
        gl::set_window_dimension(size);
}

void App::draw_internal()
{
    draw();
}

void App::timing(double timeStamp)
{
    m_framesDrawn++;

    double diff = timeStamp - m_lastMeasurementTimeStamp;

    if(diff > m_timingInterval)
    {
        m_framesPerSec = m_framesDrawn / diff;
        m_framesDrawn = 0;
        m_lastMeasurementTimeStamp = timeStamp;
    }
}

bool App::is_loading() const
{
    return crocore::Task::num_tasks();
}

/////////////////////////// Joystick ///////////////////////////////////////

JoystickState::ButtonMap JoystickState::s_button_map =
{
    {Mapping::ANALOG_LEFT_H, 0},
    {Mapping::ANALOG_LEFT_V, 1},
    {Mapping::ANALOG_RIGHT_H, 2},
    {Mapping::ANALOG_RIGHT_V, 3},
    {Mapping::TRIGGER_LEFT, 4},
    {Mapping::TRIGGER_RIGHT, 5},
    {Mapping::DPAD_H, 6},
    {Mapping::DPAD_V, 7},

    {Mapping::BUTTON_A, 0},
    {Mapping::BUTTON_B, 1},
    {Mapping::BUTTON_X, 3},
    {Mapping::BUTTON_Y, 4},
    {Mapping::BUTTON_MENU, 11},
    {Mapping::BUTTON_BACK, 11},
    {Mapping::BUTTON_BUMPER_LEFT, 6},
    {Mapping::BUTTON_BUMPER_RIGHT, 7},
    {Mapping::BUTTON_STICK_LEFT, 13},
    {Mapping::BUTTON_STICK_RIGHT, 14}
};

JoystickState::JoystickState(const std::string &n,
                             const std::vector<uint8_t>& b,
                             const std::vector<float>& a):
m_name(n),
m_buttons(b),
m_axis(a)
{

}

const std::string& JoystickState::name() const { return m_name; };
const std::vector<uint8_t>& JoystickState::buttons() const { return m_buttons; };
const std::vector<float>& JoystickState::axis() const { return m_axis; };

const gl::vec2 JoystickState::analog_left() const
{
    uint32_t index_h = s_button_map[Mapping::ANALOG_LEFT_H], index_v = s_button_map[Mapping::ANALOG_LEFT_V];
    return gl::vec2(fabs(m_axis[index_h]) > m_dead_zone ? m_axis[index_h] : 0.f,
                    fabs(m_axis[index_v]) > m_dead_zone ? m_axis[index_v] : 0.f);
}

const gl::vec2 JoystickState::analog_right() const
{
    uint32_t index_h = s_button_map[Mapping::ANALOG_RIGHT_H], index_v = s_button_map[Mapping::ANALOG_RIGHT_V];
    return gl::vec2(fabs(m_axis[index_h]) > m_dead_zone ? m_axis[index_h] : 0.f,
                    fabs(m_axis[index_v]) > m_dead_zone ? m_axis[index_v] : 0.f);
}

const gl::vec2 JoystickState::trigger() const
{
    uint32_t index_h = s_button_map[Mapping::TRIGGER_LEFT], index_v = s_button_map[Mapping::TRIGGER_RIGHT];
    return gl::vec2(fabs(m_axis[index_h]) > m_dead_zone ? m_axis[index_h] : 0.f,
                    fabs(m_axis[index_v]) > m_dead_zone ? m_axis[index_v] : 0.f);
}

const gl::vec2 JoystickState::dpad() const
{
    uint32_t index_h = s_button_map[Mapping::DPAD_H], index_v = s_button_map[Mapping::DPAD_V];
    return gl::vec2(fabs(m_axis[index_h]) > m_dead_zone ? m_axis[index_h] : 0.f,
                    fabs(m_axis[index_v]) > m_dead_zone ? m_axis[index_v] : 0.f);
}

}
