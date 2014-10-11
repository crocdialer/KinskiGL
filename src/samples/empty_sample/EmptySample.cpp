//
//  EmptySample.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "EmptySample.h"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void EmptySample::setup()
{
    ViewerApp::setup();
    m_font.load("Courier New Bold.ttf", 18);
    outstream_gl().set_color(gl::COLOR_WHITE);
    outstream_gl().set_font(m_font);
    observeProperties();
    create_tweakbar_from_component(shared_from_this());
    
    gl::createShader(gl::SHADER_UNLIT);
    gl::createShader(gl::SHADER_GOURAUD);
    gl::createShader(gl::SHADER_PHONG);
    gl::createShader(gl::SHADER_PHONG_NORMALMAP);
    gl::createShader(gl::SHADER_PHONG_SKIN);
    gl::createShader(gl::SHADER_LINES);
    gl::createShader(gl::SHADER_POINTS_COLOR);
    gl::createShader(gl::SHADER_POINTS_SPHERE);
    gl::createShader(gl::SHADER_POINTS_TEXTURE);
}

/////////////////////////////////////////////////////////////////

void EmptySample::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void EmptySample::draw()
{
    gl::setMatrices(camera());
    gl::drawGrid(50, 50);
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

void EmptySample::got_message(const std::vector<uint8_t> &the_message)
{
    LOG_INFO<<string(the_message.begin(), the_message.end());
}

/////////////////////////////////////////////////////////////////

void EmptySample::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void EmptySample::tearDown()
{
    LOG_PRINT<<"ciao empty sample";
}

/////////////////////////////////////////////////////////////////

void EmptySample::updateProperty(const Property::ConstPtr &theProperty)
{
    ViewerApp::updateProperty(theProperty);
}
