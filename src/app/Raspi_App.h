#pragma once

#include "App.h"
#include "OutstreamGL.h"

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
};

}
