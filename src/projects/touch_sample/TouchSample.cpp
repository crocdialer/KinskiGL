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
    observe_properties();
    register_property(m_circle_radius);
    add_tweakbar_for_component(shared_from_this());
    load_settings();
}

/////////////////////////////////////////////////////////////////

void TouchSample::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    textures()[0] = m_noise.simplex(getApplicationTime());
}

/////////////////////////////////////////////////////////////////

void TouchSample::draw()
{
    for(const auto &t : m_current_touches)
    {
        gl::draw_circle(t->m_position, *m_circle_radius, color_palette[t->m_slot_index]);
    }
    if(displayTweakBar()){ draw_textures(textures()); }
}

/////////////////////////////////////////////////////////////////

void TouchSample::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void TouchSample::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
}

/////////////////////////////////////////////////////////////////

void TouchSample::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void TouchSample::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void TouchSample::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void TouchSample::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
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

void TouchSample::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void TouchSample::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void TouchSample::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void TouchSample::tearDown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void TouchSample::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
}
