//
//  MovieTest.cpp
//  kinskiGL
//
//  Created by Fabian on 29/01/14.
//
//

#include "MovieTest.h"
#include "kinskiGL/ArrayTexture.h"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void MovieTest::setup()
{
    ViewerApp::setup();
    
    m_font.load("Courier New Bold.ttf", 18);
    outstream_gl().set_color(gl::COLOR_WHITE);
    outstream_gl().set_font(m_font);
    
    registerProperty(m_movie_speed);
    registerProperty(m_movie_path);
    observeProperties();
    create_tweakbar_from_component(shared_from_this());
    
    m_movie.set_on_load_callback(bind(&MovieTest::on_movie_load, this));
    
//    m_camera.start_capture();
    
    load_settings();
}

/////////////////////////////////////////////////////////////////

void MovieTest::update(float timeDelta)
{
    m_movie.copy_frame_to_texture(m_textures[0]);
//    m_camera.copy_frame_to_texture(m_textures[0]);
}

/////////////////////////////////////////////////////////////////

void MovieTest::draw()
{
    if(m_textures[0])
        gl::drawTexture(m_textures[0], gl::windowDimension());
    
    if(displayTweakBar())
    {
        gl::drawText2D(m_movie.get_path() + " : " +
                       kinski::as_string(m_movie.current_time(), 2) + " / " +
                       kinski::as_string(m_movie.duration(), 2),
                       m_font);
    }
}

/////////////////////////////////////////////////////////////////

void MovieTest::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
    
    gl::ArrayTexture array_tex;
    
    switch (e.getCode())
    {
        case GLFW_KEY_P:
            m_movie.pause();
            break;
            
        case GLFW_KEY_B:
            m_movie.copy_frames_offline(array_tex);
            break;
            
        case GLFW_KEY_LEFT:
            m_movie.seek_to_time(m_movie.current_time() - 5);
            break;
        
        case GLFW_KEY_RIGHT:
            m_movie.seek_to_time(m_movie.current_time() + 5);
            break;
        case GLFW_KEY_UP:
            m_movie.set_volume(m_movie.volume() + .1f);
            break;
            
        case GLFW_KEY_DOWN:
            m_movie.set_volume(m_movie.volume() - .1f);
            break;
            
        default:
            break;
    }
}

/////////////////////////////////////////////////////////////////

void MovieTest::got_message(const std::vector<uint8_t> &the_data)
{
    LOG_INFO << string(the_data.begin(), the_data.end());
}

/////////////////////////////////////////////////////////////////

void MovieTest::tearDown()
{
    LOG_PRINT << "ciao movie sample";
}

/////////////////////////////////////////////////////////////////

void MovieTest::on_movie_load()
{
    m_movie.play();
}

/////////////////////////////////////////////////////////////////

void MovieTest::updateProperty(const Property::ConstPtr &theProperty)
{
    ViewerApp::updateProperty(theProperty);
    
    if(theProperty == m_movie_path)
    {
        m_movie.load(*m_movie_path);
        m_movie.set_loop(true);
    }
    else if(theProperty == m_movie_speed)
    {
        m_movie.set_rate(*m_movie_speed);
    }
}