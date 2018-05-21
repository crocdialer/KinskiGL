//
//  EmptySample.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "EmptySample.hpp"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void EmptySample::setup()
{
    ViewerApp::setup();
    // register properties here
    observe_properties();
    load_settings();
}

/////////////////////////////////////////////////////////////////

void EmptySample::update(float timeDelta)
{
    ViewerApp::update(timeDelta);

    // construct ImGui window for this frame
    if(display_tweakbar())
    {
        gui::draw_component_ui(shared_from_this());
    }
}

/////////////////////////////////////////////////////////////////

void EmptySample::draw()
{
    gl::clear();
    gl::set_matrices(camera());
    if(*m_draw_grid){ gl::draw_grid(50, 50); }
}

/////////////////////////////////////////////////////////////////

void EmptySample::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void EmptySample::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);
}

/////////////////////////////////////////////////////////////////

void EmptySample::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void EmptySample::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
}

/////////////////////////////////////////////////////////////////

void EmptySample::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
}

/////////////////////////////////////////////////////////////////

void EmptySample::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void EmptySample::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void EmptySample::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void EmptySample::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void EmptySample::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void EmptySample::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void EmptySample::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void EmptySample::teardown()
{
    BaseApp::teardown();
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void EmptySample::update_property(const Property::ConstPtr &the_property)
{
    ViewerApp::update_property(theProperty);
}
