//
//  TouchSample.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "TouchSample.hpp"

using namespace std;
using namespace kinski;
using namespace glm;

namespace
{
    gl::Color color_palette[10] =
    {
        gl::COLOR_DARK_RED,
        gl::COLOR_OLIVE,
        gl::COLOR_ORANGE,
        gl::COLOR_PURPLE,
        gl::COLOR_WHITE,
        gl::COLOR_RED,
        gl::COLOR_GREEN,
        gl::COLOR_BLUE,
        gl::COLOR_YELLOW,
        gl::COLOR_GRAY
    };
}

/////////////////////////////////////////////////////////////////

void TouchSample::setup()
{
    ViewerApp::setup();
    fonts()[FONT_LARGE].load(fonts()[FONT_NORMAL].path(), 63.f);
    observe_properties();
    register_property(m_circle_radius);
    add_tweakbar_for_component(shared_from_this());
    m_noise.set_scale(vec2(0.02f));
    load_settings();
}

/////////////////////////////////////////////////////////////////

void TouchSample::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    textures()[TEXTURE_SIMPLEX] = m_noise.simplex(get_application_time() * .3f);
}

/////////////////////////////////////////////////////////////////

void TouchSample::draw()
{
    // offscreen render pass
    auto touches_tex = gl::render_to_texture(m_offscreen_fbo, [this]()
    {
        gl::clear();
        
        for(const auto &t : m_current_touches)
        {
            gl::draw_circle(t->m_position, *m_circle_radius, color_palette[t->m_slot_index]);
        }
    });
    
    // draw with simplex texture as mask
    gl::draw_texture_with_mask(touches_tex, textures()[TEXTURE_SIMPLEX], gl::window_dimension());
    
    // draw info text
    gl::draw_text_2D(name(), fonts()[FONT_LARGE], gl::COLOR_WHITE, gl::vec2(20));
    
    if(display_tweakbar()){ draw_textures(textures()); }
}

/////////////////////////////////////////////////////////////////

void TouchSample::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
    m_offscreen_fbo = gl::Fbo(w, h);
}

/////////////////////////////////////////////////////////////////

void TouchSample::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);
}

/////////////////////////////////////////////////////////////////

void TouchSample::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void TouchSample::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
}

/////////////////////////////////////////////////////////////////

void TouchSample::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
}

/////////////////////////////////////////////////////////////////

void TouchSample::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void TouchSample::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{
    m_current_touches = the_touches;
}

/////////////////////////////////////////////////////////////////

void TouchSample::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{
    m_current_touches = the_touches;
}

/////////////////////////////////////////////////////////////////

void TouchSample::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{
    m_current_touches = the_touches;
}

/////////////////////////////////////////////////////////////////

void TouchSample::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void TouchSample::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void TouchSample::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void TouchSample::teardown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void TouchSample::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
}
