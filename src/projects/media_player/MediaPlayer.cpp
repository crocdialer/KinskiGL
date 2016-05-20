//
//  MediaPlayer.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "MediaPlayer.hpp"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void MediaPlayer::setup()
{
    ViewerApp::setup();
    Logger::get()->set_use_log_file(true);
    
    fonts()[1].load(fonts()[0].path(), 28);
    register_property(m_movie_path);
    register_property(m_loop);
    register_property(m_auto_play);
    register_property(m_volume);
    register_property(m_playback_speed);
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
    
    // setup our components to receive rpc calls
    setup_rpc_interface();
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::update(float timeDelta)
{
    if(m_reload_movie)
    {
        m_movie->load(*m_movie_path, *m_auto_play, *m_loop);
        m_movie->set_rate(*m_playback_speed);
        m_movie->set_volume(*m_volume);
        m_reload_movie = false;
    }
    
    if(m_camera_control && m_camera_control->is_capturing())
        m_camera_control->copy_frame_to_texture(textures()[TEXTURE_INPUT]);
    else
        m_movie->copy_frame_to_texture(textures()[TEXTURE_INPUT]);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::draw()
{
    if(*m_use_warping){ m_warp->render_output(textures()[TEXTURE_INPUT]); }
    else{ gl::draw_texture(textures()[TEXTURE_INPUT], gl::window_dimension()); }

    if(displayTweakBar())
    {
        gl::draw_text_2D(secs_to_time_str(m_movie->current_time()) + " / " +
                         secs_to_time_str(m_movie->duration()) + " - " +
                         get_filename_part(m_movie->path()),
                         fonts()[1], gl::COLOR_WHITE, gl::vec2(10));
        draw_textures(textures());
    }
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::keyPress(const KeyEvent &e)
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

void MediaPlayer::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void MediaPlayer::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void MediaPlayer::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void MediaPlayer::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    *m_movie_path = files.back();
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::tearDown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);

    if(theProperty == m_movie_path)
    {
        m_reload_movie = true;
    }
    else if(theProperty == m_loop)
    {
        m_movie->set_loop(*m_loop);
    }
    else if(theProperty == m_volume)
    {
        m_movie->set_volume(*m_volume);
    }
    else if(theProperty == m_playback_speed)
    {
        m_movie->set_rate(*m_playback_speed);
    }
    else if(theProperty == m_use_warping)
    {
        remove_tweakbar_for_component(m_warp);
        if(*m_use_warping){ add_tweakbar_for_component(m_warp); }
    }
    else if(theProperty == m_use_discovery_broadcast)
    {
        if(*m_use_discovery_broadcast)
        {
            // setup a periodic udp-broadcast to enable discovery of this node
            m_broadcast_timer = Timer(main_queue().io_service(), [this]()
            {
                net::async_send_udp_broadcast(background_queue().io_service(), "ping", 55555);
            });
            m_broadcast_timer.set_periodic();
            m_broadcast_timer.expires_from_now(2.f);
        }
    }
}

/////////////////////////////////////////////////////////////////

bool MediaPlayer::save_settings(const std::string &path)
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

bool MediaPlayer::load_settings(const std::string &path)
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

std::string MediaPlayer::secs_to_time_str(float the_secs) const
{
    char buf[32];
    sprintf(buf, "%d:%02d:%.1f", (int)the_secs / 3600, ((int)the_secs / 60) % 60, fmodf(the_secs, 60));
    return buf;
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::setup_rpc_interface()
{
    remote_control().add_command("play");
    register_function("play", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty())
        {
            std::string p; for(const auto &arg : rpc_args){ p += arg + " "; }
            p = p.substr(0, p.size() - 1);
            *m_movie_path = p;
        }
        else{ m_movie->play(); }
    });
    remote_control().add_command("pause");
    register_function("pause", [this](const std::vector<std::string> &rpc_args)
    {
        m_movie->pause();
    });
    remote_control().add_command("restart", [this](net::tcp_connection_ptr con,
                                                   const std::vector<std::string> &rpc_args)
    {
        m_movie->restart();
    });
    remote_control().add_command("load");
    register_function("load", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty())
        {
            std::string p; for(const auto &arg : rpc_args){ p += arg + " "; }
            p = p.substr(0, p.size() - 1);
            *m_movie_path = p;
        }
    });
    remote_control().add_command("unload");
    register_function("unload", [this](const std::vector<std::string> &rpc_args)
    {
        m_movie->unload();
        textures()[TEXTURE_INPUT].reset();
    });
    remote_control().add_command("set_volume");
    register_function("set_volume", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_movie->set_volume(kinski::string_as<float>(rpc_args.front())); }
    });
    
    remote_control().add_command("volume", [this](net::tcp_connection_ptr con,
                                                  const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_movie->set_volume(kinski::string_as<float>(rpc_args.front())); }
        con->send(as_string(m_movie->volume()));
    });
    
    remote_control().add_command("set_rate");
    register_function("set_rate", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_movie->set_rate(kinski::string_as<float>(rpc_args.front())); }
    });
    
    remote_control().add_command("rate", [this](net::tcp_connection_ptr con,
                                                const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_movie->set_rate(kinski::string_as<float>(rpc_args.front())); }
        con->send(as_string(m_movie->rate()));
    });
    
    remote_control().add_command("seek_to_time");
    register_function("seek_to_time", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty())
        {
            float secs = 0.f;
            auto splits = split(rpc_args.front(), ':');
          
            switch (splits.size())
            {
                case 3:
                    secs = kinski::string_as<float>(splits[2]) +
                    60.f * kinski::string_as<float>(splits[1]) +
                    3600.f * kinski::string_as<float>(splits[0]) ;
                    break;
                  
                case 2:
                    secs = kinski::string_as<float>(splits[1]) +
                    60.f * kinski::string_as<float>(splits[0]);
                    break;
                  
                case 1:
                    secs = kinski::string_as<float>(splits[0]);
                    break;
                  
                default:
                    break;
            }
            m_movie->seek_to_time(secs);
        }
    });
    
    remote_control().add_command("current_time", [this](net::tcp_connection_ptr con,
                                                        const std::vector<std::string> &rpc_args)
    {
        con->send(as_string(m_movie->current_time(), 1));
    });
    
    remote_control().add_command("duration", [this](net::tcp_connection_ptr con,
                                                    const std::vector<std::string> &rpc_args)
    {
        con->send(as_string(m_movie->duration(), 1));
    });
    
    remote_control().add_command("set_loop");
    register_function("set_loop", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_movie->set_loop(kinski::string_as<bool>(rpc_args.front())); }
    });
    
    remote_control().add_command("loop", [this](net::tcp_connection_ptr con,
                                                const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_movie->set_loop(kinski::string_as<bool>(rpc_args.front())); }
        con->send(as_string(m_movie->loop()));
    });
    
    remote_control().add_command("is_playing", [this](net::tcp_connection_ptr con,
                                                      const std::vector<std::string> &rpc_args)
    {
        con->send(as_string(m_movie->is_playing()));
    });
}