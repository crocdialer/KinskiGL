//
//  CoreVideoTest.cpp
//  kinskiGL
//
//  Created by Fabian on 29/01/14.
//
//

#include "MovieTest.h"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void CoreVideoTest::setup()
{
    ViewerApp::setup();
    
    m_font.load("Courier New Bold.ttf", 18);
    outstream_gl().set_color(gl::COLOR_WHITE);
    outstream_gl().set_font(m_font);
    
    registerProperty(m_movie_speed);
    registerProperty(m_movie_path);
    observeProperties();
    create_tweakbar_from_component(shared_from_this());
    load_settings();
}

/////////////////////////////////////////////////////////////////

void CoreVideoTest::update(float timeDelta)
{
    m_movie.copy_frame_to_texture(m_textures[0]);
}

/////////////////////////////////////////////////////////////////

void CoreVideoTest::draw()
{
    if(m_textures[0])
        gl::drawTexture(m_textures[0], gl::windowDimension());
    
    gl::drawText2D(m_movie.get_path() + " : " +
                   kinski::as_string(m_movie.current_time()),
                   m_font);
}

/////////////////////////////////////////////////////////////////

void CoreVideoTest::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
    
    switch (e.getCode())
    {
        case GLFW_KEY_P:
            m_movie.pause();
            break;
            
        case GLFW_KEY_LEFT:
            m_movie.seek_to_time(m_movie.current_time() - 5);
            break;
        
        case GLFW_KEY_RIGHT:
            m_movie.seek_to_time(m_movie.current_time() + 5);
            break;
            
        default:
            break;
    }
}

/////////////////////////////////////////////////////////////////

void CoreVideoTest::got_message(const std::string &the_message)
{
    LOG_INFO<<the_message;
}

/////////////////////////////////////////////////////////////////

void CoreVideoTest::tearDown()
{
    LOG_PRINT<<"ciao movie sample";
}

/////////////////////////////////////////////////////////////////

void CoreVideoTest::updateProperty(const Property::ConstPtr &theProperty)
{
    ViewerApp::updateProperty(theProperty);
    
    if(theProperty == m_movie_path)
    {
        m_movie.load(*m_movie_path);
        m_movie.set_loop(true);
        m_movie.play();
    }
    else if(theProperty == m_movie_speed)
    {
        m_movie.set_rate(*m_movie_speed);
    }
}
