//
//  MovieTest.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "MovieTest.h"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void MovieTest::setup()
{
    ViewerApp::setup();
    set_window_title("movie tester");
    
    register_property(m_movie_speed);
    register_property(m_movie_path);
    observe_properties();
    create_tweakbar_from_component(shared_from_this());
    
    m_movie->set_on_load_callback(bind(&MovieTest::on_movie_load, this));
    
    load_settings();
    
    m_quad_warp.set_control_point(0, 0, vec2(.3f, 0.1f));
    m_quad_warp.set_control_point(1, 0, vec2(.8f, 0.3f));
}

/////////////////////////////////////////////////////////////////

void MovieTest::update(float timeDelta)
{
    if(m_camera_control && m_camera_control->is_capturing())
        m_camera_control->copy_frame_to_texture(m_textures[0]);
    else
        m_movie->copy_frame_to_texture(m_textures[0], true);
}

/////////////////////////////////////////////////////////////////

void MovieTest::draw()
{
    m_quad_warp.render_output(m_textures[0]);
    m_quad_warp.render_grid();
    m_quad_warp.render_control_points();
    
    if(displayTweakBar())
    {
        gl::drawText2D(m_movie->get_path() + " : " +
                       kinski::as_string(m_movie->current_time(), 2) + " / " +
                       kinski::as_string(m_movie->duration(), 2),
                       fonts()[0]);
    }
}

/////////////////////////////////////////////////////////////////

void MovieTest::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
    
    switch (e.getCode())
    {
        case Key::KEY_C:
            if(m_camera_control->is_capturing())
                m_camera_control->stop_capture();
            else
                m_camera_control->start_capture();
            break;
            
        case Key::KEY_P:
            m_movie->isPlaying() ? m_movie->pause() : m_movie->play();
            break;
            
        case Key::KEY_LEFT:
            m_movie->seek_to_time(m_movie->current_time() - 5);
            break;
        
        case Key::KEY_RIGHT:
            m_movie->seek_to_time(m_movie->current_time() + 5);
            break;
        case Key::KEY_UP:
            m_movie->set_volume(m_movie->volume() + .1f);
            break;
            
        case Key::KEY_DOWN:
            m_movie->set_volume(m_movie->volume() - .1f);
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

void MovieTest::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    *m_movie_path = files.back();
}

/////////////////////////////////////////////////////////////////

void MovieTest::tearDown()
{
    LOG_PRINT << "ciao movie sample";
}

/////////////////////////////////////////////////////////////////

void MovieTest::on_movie_load()
{
    m_movie->play();
}

/////////////////////////////////////////////////////////////////

void MovieTest::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
    
    if(theProperty == m_movie_path)
    {
        m_movie->load(*m_movie_path);
        m_movie->set_loop(true);
    }
    else if(theProperty == m_movie_speed)
    {
        m_movie->set_rate(*m_movie_speed);
    }
}