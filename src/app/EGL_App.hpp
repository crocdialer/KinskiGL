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

namespace kinski
{

class EGL_App : public App
{
 public:

    EGL_App(int argc = 0, char *argv[] = nullptr);
    virtual ~EGL_App();

    void set_window_size(const glm::vec2 &size) override;
    double get_application_time() override;

    void set_cursor_position(float x, float y) override;
    gl::vec2 cursor_position() const override;

    void teardown() override{};

    const gl::OutstreamGL& outstream_gl() const {return m_outstream_gl;};
    gl::OutstreamGL& outstream_gl(){return m_outstream_gl;};

    std::vector<WindowPtr> windows() const { return {}; }

    void set_lcd_backlight(bool b) const;

 private:

    // internal initialization. performed when run is invoked
    void init() override;
    void draw_internal() override;
    void swap_buffers() override;
    void poll_events() override;

    timeval m_startTime;
    std::unique_ptr<struct ESContext> m_context;
    gl::OutstreamGL m_outstream_gl;

    // input file descriptors
    int m_mouse_fd, m_keyboard_fd, m_touch_fd;

    Timer m_timer_device_scan;
};

//TODO: find a cleaner solution to crossplattform happiness
namespace gui
{
//! draw a kinski::Component using ImGui
void draw_component_ui(const ComponentConstPtr &the_component);

void draw_textures_ui(const std::vector<gl::Texture> &the_textures);

void draw_lights_ui(const std::vector<gl::LightPtr> &the_lights);

void draw_material_ui(const gl::MaterialPtr &the_mat);
void draw_materials_ui(const std::vector<gl::MaterialPtr> &the_materials);

void draw_mesh_ui(const gl::MeshPtr &the_mesh);
}

}
