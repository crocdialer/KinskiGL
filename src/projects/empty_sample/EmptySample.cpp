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
    observe_properties();
    add_tweakbar_for_component(shared_from_this());
    load_settings();
}

/////////////////////////////////////////////////////////////////

void EmptySample::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void EmptySample::draw()
{
    gl::set_matrices(camera());
    if(*m_draw_grid){ gl::draw_grid(50, 50); }
    
    gl::draw_points_2D({gl::vec2(100, 200)}, gl::COLOR_ORANGE, 50.f);
}

/////////////////////////////////////////////////////////////////

void EmptySample::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void EmptySample::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
}

/////////////////////////////////////////////////////////////////

void EmptySample::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void EmptySample::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void EmptySample::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void EmptySample::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
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

void EmptySample::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void EmptySample::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void EmptySample::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void EmptySample::tearDown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void EmptySample::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
}
