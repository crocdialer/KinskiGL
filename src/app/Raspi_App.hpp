// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "core/Timer.hpp"
#include "App.hpp"
#include "OutstreamGL.hpp"

struct ESContext;

namespace kinski
{

class Raspi_App : public App
{
 public:

    Raspi_App(int argc = 0, char *argv[] = nullptr);
    virtual ~Raspi_App();

    void set_window_size(const glm::vec2 &size) override;
    double getApplicationTime() override;

    void set_cursor_position(float x, float y) override;
    gl::vec2 cursor_position() const override;

    const gl::OutstreamGL& outstream_gl() const {return m_outstream_gl;};
    gl::OutstreamGL& outstream_gl(){return m_outstream_gl;};

    std::vector<WindowPtr> windows() const { return {}; }

    void set_lcd_backlight(bool b) const;

 private:

    // internal initialization. performed when run is invoked
    void init() override;
    void draw_internal() override;
    void swapBuffers() override;
    void pollEvents() override;

    timeval m_startTime;
    std::unique_ptr<ESContext> m_context;
    gl::OutstreamGL m_outstream_gl;

    // input file descriptors
    int m_mouse_fd, m_keyboard_fd, m_touch_fd;

    Timer m_timer_device_scan;
};

}
