//
//  OMXWrapper.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "OMXWrapper.h"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void OMXWrapper::setup()
{
    ViewerApp::setup();
    registerProperty(m_is_master);
    registerProperty(m_movie_path);
    registerProperty(m_movie_delay);
    observeProperties();
    create_tweakbar_from_component(shared_from_this());
    fonts()[FONT_LARGE].load("Courier New Bold.ttf", 256);

    // this will load settings or generate settings file, if not present
    if(!load_settings()){ save_settings(); }

    //m_video_thread = std::thread(&OMXWrapper::thread_func, this);
}

/////////////////////////////////////////////////////////////////

void OMXWrapper::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void OMXWrapper::draw()
{   
    vec2 offset(20, gl::windowDimension().y / 2.f);
    gl::drawText2D("poooop", fonts()[FONT_LARGE], glm::vec4(1), offset); 
    
    gl::drawQuad(gl::COLOR_ORANGE, vec2(80), vec2(120), true);
}

/////////////////////////////////////////////////////////////////

void OMXWrapper::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void OMXWrapper::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
}

/////////////////////////////////////////////////////////////////

void OMXWrapper::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void OMXWrapper::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void OMXWrapper::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void OMXWrapper::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void OMXWrapper::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void OMXWrapper::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void OMXWrapper::got_message(const std::vector<uint8_t> &the_message)
{
    LOG_INFO<<string(the_message.begin(), the_message.end());
}

/////////////////////////////////////////////////////////////////

void OMXWrapper::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void OMXWrapper::tearDown()
{
    LOG_PRINT<<"ciao empty sample";
}

/////////////////////////////////////////////////////////////////

void OMXWrapper::updateProperty(const Property::ConstPtr &theProperty)
{
    ViewerApp::updateProperty(theProperty);
}

/////////////////////////////////////////////////////////////////

void OMXWrapper::thread_func()
{
    LOG_DEBUG << "thread started";
    int delay_secs = *m_movie_delay;
    string cmd = "omxplayer --loop -b --pos " + as_string(delay_secs) +" "+ m_movie_path->value();
    system(cmd.c_str());

    LOG_DEBUG << "thread terminated";
}
