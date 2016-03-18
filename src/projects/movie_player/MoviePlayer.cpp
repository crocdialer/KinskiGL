//
//  MoviePlayer.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "MoviePlayer.hpp"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void MoviePlayer::setup()
{
    ViewerApp::setup();
    
    fonts()[1].load(fonts()[0].path(), 28);
    register_property(m_movie_path);
    register_property(m_movie_speed);
    register_property(m_use_warping);
    observe_properties();
    add_tweakbar_for_component(shared_from_this());

    // warp component
    m_warp = std::make_shared<WarpComponent>();
    m_warp->observe_properties();

    remote_control().set_components({ shared_from_this(), m_warp });
    load_settings();
    
    // check for command line input
    if(args().size() > 1 && file_exists(args()[1])){ *m_movie_path = args()[1]; }
}

/////////////////////////////////////////////////////////////////

void MoviePlayer::update(float timeDelta)
{
    if(m_reload_movie)
    {
        m_movie->load(*m_movie_path, true, true);
        m_reload_movie = false;
    }
    
    if(m_camera_control && m_camera_control->is_capturing())
        m_camera_control->copy_frame_to_texture(textures()[TEXTURE_INPUT]);
    else
        m_movie->copy_frame_to_texture(textures()[TEXTURE_INPUT]);
}

/////////////////////////////////////////////////////////////////

void MoviePlayer::draw()
{
    if(*m_use_warping){ m_warp->render_output(textures()[TEXTURE_INPUT]); }
    else{ gl::draw_texture(textures()[TEXTURE_INPUT], gl::window_dimension()); }

    if(displayTweakBar())
    {
        gl::draw_text_2D(secs_to_time_str(m_movie->current_time()) + " / " +
                         secs_to_time_str(m_movie->duration()) + " - " +
                         get_filename_part(m_movie->get_path()),
                         fonts()[1], gl::COLOR_WHITE, gl::vec2(10));
        draw_textures(textures());
    }
}

/////////////////////////////////////////////////////////////////

void MoviePlayer::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);

    switch (e.getCode())
    {
        case Key::_C:
            if(m_camera_control->is_capturing())
                m_camera_control->stop_capture();
            else
                m_camera_control->start_capture();
            break;

        case Key::_P:
            m_movie->is_playing() ? m_movie->pause() : m_movie->play();
            break;

        case Key::_LEFT:
            m_movie->seek_to_time(m_movie->current_time() - 5);
            break;

        case Key::_RIGHT:
            m_movie->seek_to_time(m_movie->current_time() + 5);
            break;
        case Key::_UP:
            m_movie->set_volume(m_movie->volume() + .1f);
            break;

        case Key::_DOWN:
            m_movie->set_volume(m_movie->volume() - .1f);
            break;

        default:
            break;
    }
}

/////////////////////////////////////////////////////////////////

void MoviePlayer::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void MoviePlayer::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void MoviePlayer::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void MoviePlayer::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void MoviePlayer::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void MoviePlayer::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void MoviePlayer::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void MoviePlayer::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void MoviePlayer::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void MoviePlayer::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void MoviePlayer::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    *m_movie_path = files.back();
}

/////////////////////////////////////////////////////////////////

void MoviePlayer::tearDown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void MoviePlayer::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);

    if(theProperty == m_movie_path)
    {
        m_reload_movie = true;
    }
    else if(theProperty == m_movie_speed)
    {
        m_movie->set_rate(*m_movie_speed);
    }
    else if(theProperty == m_use_warping)
    {
        remove_tweakbar_for_component(m_warp);
        if(*m_use_warping){ add_tweakbar_for_component(m_warp); }
    }
}

/////////////////////////////////////////////////////////////////

bool MoviePlayer::save_settings(const std::string &path)
{
    bool ret = ViewerApp::save_settings(path);
    try
    {
        Serializer::saveComponentState(m_warp,
                                       join_paths(path ,"warp_config.json"),
                                       PropertyIO_GL());
    }
    catch(Exception &e){ LOG_ERROR << e.what(); return false; }
    return ret;
}

/////////////////////////////////////////////////////////////////

bool MoviePlayer::load_settings(const std::string &path)
{
    bool ret = ViewerApp::load_settings(path);
    try
    {
        Serializer::loadComponentState(m_warp,
                                       join_paths(path , "warp_config.json"),
                                       PropertyIO_GL());
    }
    catch(Exception &e){ LOG_ERROR << e.what(); return false; }
    return ret;
}

/////////////////////////////////////////////////////////////////

std::string MoviePlayer::secs_to_time_str(float the_secs) const
{
    return as_string((int)the_secs / 3600) + ":" + as_string(((int)the_secs / 60) % 60) + ":" +
               as_string(fmodf(the_secs, 60), 1);
}
