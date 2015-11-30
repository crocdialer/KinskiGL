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
    registerProperty(m_host_names);
    registerProperty(m_is_master);
    registerProperty(m_movie_path);
    registerProperty(m_movie_delay);
    observeProperties();
    create_tweakbar_from_component(shared_from_this());
    fonts()[FONT_LARGE].load("Courier New Bold.ttf", 64);

    // this will load settings or generate settings file, if not present
    if(!load_settings()){ save_settings(); }

    m_remote_control.add_command("start_movie");
    register_function("start_movie", [this](){ start_movie(*m_movie_delay); });

    if(*m_is_master)
    {
        start_movie(*m_movie_delay); 
    }
    else
    {
        net::async_send_tcp(io_service(), "start_movie", m_host_names->value().front(), 33333); 
    }
}

/////////////////////////////////////////////////////////////////

void OMXWrapper::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void OMXWrapper::draw()
{   
    vec2 offset(320, gl::windowDimension().y / 2.f);
    gl::drawText2D("zug ins nirgendwo", fonts()[FONT_LARGE], glm::vec4(1), offset); 
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

    if(theProperty == m_movie_path)
    {
        if(*m_is_master){ start_movie(*m_movie_delay); }
    }
}

/////////////////////////////////////////////////////////////////

void OMXWrapper::start_movie(float delay)
{
    stop_movie();

    m_timer = Timer(io_service(), [this]()
    {
        LOG_DEBUG << "start_movie";
        string win_sz_str = " --win '0 0 " + as_string((int)gl::windowDimension().x - 1) + " " +
        as_string((int)gl::windowDimension().y - 1) + "' "; 
        string cmd = "omxplayer --layer 0 --loop --pos " +
            as_string(0) + win_sz_str + m_movie_path->value() + " &";
        LOG_DEBUG << cmd;
        system(cmd.c_str());
        m_running = true; 
    });
    m_timer.expires_from_now(delay);

    if(*m_is_master)
    {
        // master needs to trigger playback on other devices
        std::list<string> dev_names(m_host_names->value().begin(), m_host_names->value().end());
        dev_names.pop_front();

        for(auto &d : dev_names)
        {
            net::async_send_tcp(io_service(), "start_movie", d, 33333);
        }
    }
}

/////////////////////////////////////////////////////////////////

void OMXWrapper::stop_movie()
{
    LOG_DEBUG << "stop_movie";
    string cmd = "killall 'omxplayer' && killall 'omxplayer.bin'";
    LOG_DEBUG << cmd;
    system(cmd.c_str());
    m_running = false;
}
