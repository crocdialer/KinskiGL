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
    register_property(m_media_path);
    register_property(m_loop);
    register_property(m_auto_play);
    register_property(m_volume);
    register_property(m_brightness);
    register_property(m_playback_speed);
    register_property(m_use_warping);
    register_property(m_force_audio_jack);
    register_property(m_use_discovery_broadcast);
    register_property(m_broadcast_port);
    observe_properties();
    add_tweakbar_for_component(shared_from_this());

    // warp component
    m_warp = std::make_shared<WarpComponent>();
    m_warp->observe_properties();

    remote_control().set_components({ shared_from_this(), m_warp });
//    set_default_config_path("~/");
    load_settings();
    
    // check for command line input
    if(args().size() > 1 && fs::exists(args()[1])){ *m_media_path = args()[1]; }
    
    // setup our components to receive rpc calls
    setup_rpc_interface();
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::update(float timeDelta)
{
    if(m_reload_media){ reload_media(); }
    
    if(m_camera_control && m_camera_control->is_capturing())
        m_camera_control->copy_frame_to_texture(textures()[TEXTURE_INPUT]);
    else
        m_media->copy_frame_to_texture(textures()[TEXTURE_INPUT]);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::draw()
{
    if(*m_use_warping){ m_warp->render_output(textures()[TEXTURE_INPUT], *m_brightness); }
    else{ gl::draw_texture(textures()[TEXTURE_INPUT], gl::window_dimension(), gl::vec2(0),
                           *m_brightness); }

    if(displayTweakBar())
    {
        gl::draw_text_2D(secs_to_time_str(m_media->current_time()) + " / " +
                         secs_to_time_str(m_media->duration()) + " - " +
                         fs::get_filename_part(m_media->path()),
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
            m_media->is_playing() ? m_media->pause() : m_media->play();
            break;

        case Key::_LEFT:
            m_media->seek_to_time(m_media->current_time() - 5);
            break;

        case Key::_RIGHT:
            m_media->seek_to_time(m_media->current_time() + 5);
            break;
        case Key::_UP:
            m_media->set_volume(m_media->volume() + .1f);
            break;

        case Key::_DOWN:
            m_media->set_volume(m_media->volume() - .1f);
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
    *m_media_path = files.back();
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

    if(theProperty == m_media_path)
    {
        m_reload_media = true;
    }
    else if(theProperty == m_loop)
    {
        m_media->set_loop(*m_loop);
    }
    else if(theProperty == m_volume)
    {
        m_media->set_volume(*m_volume);
    }
    else if(theProperty == m_playback_speed)
    {
        m_media->set_rate(*m_playback_speed);
    }
    else if(theProperty == m_use_warping)
    {
        remove_tweakbar_for_component(m_warp);
        if(*m_use_warping){ add_tweakbar_for_component(m_warp); }
    }
    else if(theProperty == m_use_discovery_broadcast || theProperty == m_broadcast_port)
    {
        if(*m_use_discovery_broadcast)
        {
            // setup a periodic udp-broadcast to enable discovery of this node
            m_broadcast_timer = Timer(main_queue().io_service(), [this]()
            {
                LOG_TRACE_2 << "sending discovery_broadcast on udp-port: " << m_broadcast_port->value();
                net::async_send_udp_broadcast(background_queue().io_service(), name(),
                                              *m_broadcast_port);
            });
            m_broadcast_timer.set_periodic();
            m_broadcast_timer.expires_from_now(2.f);
        }else{ m_broadcast_timer.cancel(); }
    }
}

/////////////////////////////////////////////////////////////////

bool MediaPlayer::save_settings(const std::string &the_path)
{
    bool ret = ViewerApp::save_settings(the_path);
    std::string path_prefix = the_path.empty() ? m_default_config_path : the_path;
    path_prefix = fs::get_directory_part(path_prefix);
    try
    {
        Serializer::saveComponentState(m_warp,
                                       fs::join_paths(path_prefix ,"warp_config.json"),
                                       PropertyIO_GL());
    }
    catch(Exception &e){ LOG_ERROR << e.what(); return false; }
    return ret;
}

/////////////////////////////////////////////////////////////////

bool MediaPlayer::load_settings(const std::string &the_path)
{
    bool ret = ViewerApp::load_settings(the_path);
    std::string path_prefix = the_path.empty() ? m_default_config_path : the_path;
    path_prefix = fs::get_directory_part(path_prefix);
    try
    {
        Serializer::loadComponentState(m_warp,
                                       fs::join_paths(path_prefix, "warp_config.json"),
                                       PropertyIO_GL());
    }
    catch(Exception &e){ LOG_ERROR << e.what(); return false; }
    return ret;
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::reload_media()
{
    App::Task t(this);
    
    auto media_type = fs::get_file_type(*m_media_path);
    
    LOG_DEBUG << "loading: " << m_media_path->value();
    
    if(media_type == fs::FileType::AUDIO || media_type == fs::FileType::MOVIE)
    {
        auto render_target = *m_use_warping ? media::MediaController::RenderTarget::TEXTURE :
        media::MediaController::RenderTarget::SCREEN;
        
        auto audio_target = *m_force_audio_jack ? media::MediaController::AudioTarget::AUDIO_JACK :
        media::MediaController::AudioTarget::AUTO;
        
        if(render_target == media::MediaController::RenderTarget::SCREEN)
        { set_clear_color(gl::Color(clear_color().rgb(), 0.f)); }
        
        m_media->load(*m_media_path, *m_auto_play, *m_loop, render_target, audio_target);
        m_media->set_rate(*m_playback_speed);
        m_media->set_volume(*m_volume);
        m_media->set_media_ended_callback([this](media::MediaControllerPtr mc)
        {
            LOG_DEBUG << "media ended";
        });
    }
    else if(media_type == fs::FileType::IMAGE)
    {
        m_media->unload();
        textures()[TEXTURE_INPUT] = gl::create_texture_from_file(*m_media_path);
    }
    m_reload_media = false;
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
            *m_media_path = p;
        }
        else{ m_media->play(); }
    });
    remote_control().add_command("pause");
    register_function("pause", [this](const std::vector<std::string> &rpc_args)
    {
        m_media->pause();
    });
    remote_control().add_command("restart", [this](net::tcp_connection_ptr con,
                                                   const std::vector<std::string> &rpc_args)
    {
        m_media->restart();
    });
    remote_control().add_command("load");
    register_function("load", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty())
        {
            std::string p; for(const auto &arg : rpc_args){ p += arg + " "; }
            p = p.substr(0, p.size() - 1);
            *m_media_path = p;
        }
    });
    remote_control().add_command("unload");
    register_function("unload", [this](const std::vector<std::string> &rpc_args)
    {
        m_media->unload();
        textures()[TEXTURE_INPUT].reset();
    });
    remote_control().add_command("set_volume");
    register_function("set_volume", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_media->set_volume(kinski::string_to<float>(rpc_args.front())); }
    });
    
    remote_control().add_command("volume", [this](net::tcp_connection_ptr con,
                                                  const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_media->set_volume(kinski::string_to<float>(rpc_args.front())); }
        else{ con->send(to_string(m_media->volume())); }
    });
    
    remote_control().add_command("brightness", [this](net::tcp_connection_ptr con,
                                                      const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ *m_brightness = kinski::string_to<float>(rpc_args.front()); }
        else{ con->send(to_string(m_brightness->value())); }
    });
    
    remote_control().add_command("set_brightness", [this](net::tcp_connection_ptr con,
                                                          const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ *m_brightness = kinski::string_to<float>(rpc_args.front()); }
    });
    
    remote_control().add_command("set_rate");
    register_function("set_rate", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_media->set_rate(kinski::string_to<float>(rpc_args.front())); }
    });
    
    remote_control().add_command("rate", [this](net::tcp_connection_ptr con,
                                                const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_media->set_rate(kinski::string_to<float>(rpc_args.front())); }
        con->send(to_string(m_media->rate()));
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
                    secs = kinski::string_to<float>(splits[2]) +
                    60.f * kinski::string_to<float>(splits[1]) +
                    3600.f * kinski::string_to<float>(splits[0]) ;
                    break;
                  
                case 2:
                    secs = kinski::string_to<float>(splits[1]) +
                    60.f * kinski::string_to<float>(splits[0]);
                    break;
                  
                case 1:
                    secs = kinski::string_to<float>(splits[0]);
                    break;
                  
                default:
                    break;
            }
            m_media->seek_to_time(secs);
        }
    });
    
    remote_control().add_command("current_time", [this](net::tcp_connection_ptr con,
                                                        const std::vector<std::string> &rpc_args)
    {
        con->send(to_string(m_media->current_time(), 1));
    });
    
    remote_control().add_command("duration", [this](net::tcp_connection_ptr con,
                                                    const std::vector<std::string> &rpc_args)
    {
        con->send(to_string(m_media->duration(), 1));
    });
    
    remote_control().add_command("set_loop");
    register_function("set_loop", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_media->set_loop(kinski::string_to<bool>(rpc_args.front())); }
    });
    
    remote_control().add_command("loop", [this](net::tcp_connection_ptr con,
                                                const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_media->set_loop(kinski::string_to<bool>(rpc_args.front())); }
        con->send(to_string(m_media->loop()));
    });
    
    remote_control().add_command("is_playing", [this](net::tcp_connection_ptr con,
                                                      const std::vector<std::string> &rpc_args)
    {
        con->send(to_string(m_media->is_playing()));
    });
}