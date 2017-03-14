//
//  MediaPlayer.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "MediaPlayer.hpp"
#include <mutex>

using namespace std;
using namespace kinski;
using namespace glm;


namespace
{
    //! enforce mutual exlusion on state
    std::mutex g_ip_table_mutex;

    //! interval to send sync cmd (secs)
    const double g_sync_interval = 0.05;

    //! keep_alive timeout after which a remote node is considered dead (secs)
    const double g_dead_thresh = 10.0;

    //! interval for keep_alive broadcasts (secs)
    const double g_broadcast_interval = 2.0;

    //! minimum difference to remote media-clock for fine-tuning (secs)
    double g_sync_thresh = 0.02;
    
    //! minimum difference to remote media-clock for scrubbing (secs)
    const double g_scrub_thresh = 1.0;
    
    //! force reset of playback speed (secs)
    const double g_sync_duration = 1.0;
}

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
    register_property(m_force_audio_jack);
    register_property(m_is_master);
    register_property(m_use_discovery_broadcast);
    register_property(m_broadcast_port);
    observe_properties();
    add_tweakbar_for_component(shared_from_this());

    remote_control().set_components({ shared_from_this(), m_warp_component });
//    set_default_config_path("~/");

    // setup our components to receive rpc calls
    setup_rpc_interface();

    load_settings();
    
    // check for command line input
    if(args().size() > 1 && fs::exists(args()[1])){ *m_media_path = args()[1]; }
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::update(float timeDelta)
{
    if(m_reload_media){ reload_media(); }

    if(m_camera_control && m_camera_control->is_capturing())
        m_needs_redraw = m_camera_control->copy_frame_to_texture(textures()[TEXTURE_INPUT]) || m_needs_redraw;
    else if(m_media)
        m_needs_redraw = m_media->copy_frame_to_texture(textures()[TEXTURE_INPUT]) || m_needs_redraw;
    else
        m_needs_redraw = true;
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::draw()
{
    if(*m_use_warping)
    {
        for(uint32_t i = 0; i < m_warp_component->num_warps(); i++)
        {
            if(m_warp_component->enabled(i))
            {
                m_warp_component->render_output(i, textures()[TEXTURE_INPUT], *m_brightness);
            }
        }
    }
    else{ gl::draw_texture(textures()[TEXTURE_INPUT], gl::window_dimension(), gl::vec2(0),
                           *m_brightness); }
    
    if(!*m_is_master && m_is_syncing && Logger::get()->severity() >= Severity::DEBUG)
    {
        gl::draw_text_2D(to_string(m_is_syncing) + " ms", fonts()[1], gl::COLOR_WHITE, vec2(50));
    }
    if(displayTweakBar())
    {
        gl::draw_text_2D(m_media->is_loaded() ? fs::get_filename_part(m_media->path()) : *m_media_path,
                         fonts()[1], gl::COLOR_WHITE, gl::vec2(10));
        gl::draw_text_2D(secs_to_time_str(m_media->current_time()) + " / " +
                         secs_to_time_str(m_media->duration()),
                         fonts()[1], gl::COLOR_WHITE, gl::vec2(10, 40));
        draw_textures(textures());
    }
    m_needs_redraw = false;
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);
    
    if(!e.isAltDown())
    {
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
                if(*m_is_master){ send_network_cmd(m_media->is_playing() ? "play" : "pause"); }
                break;
                
            case Key::_LEFT:
                m_media->seek_to_time(m_media->current_time() - (e.isShiftDown() ? 30 : 5));
                m_needs_redraw = true;
                break;
                
            case Key::_RIGHT:
                m_media->seek_to_time(m_media->current_time() + (e.isShiftDown() ? 30 : 5));
                m_needs_redraw = true;
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
}

/////////////////////////////////////////////////////////////////

bool MediaPlayer::needs_redraw() const
{
    return (m_media && (!m_media->is_playing() || !m_media->has_video())) || !*m_use_warping
        || m_needs_redraw || m_is_syncing;
};

/////////////////////////////////////////////////////////////////

void MediaPlayer::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::keyRelease(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
//    if(m_media && m_media->is_loaded())
//    {
//        if(m_media->is_playing()){ m_media->pause(); }
//        else{ m_media->play(); }
//    }
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
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
        if(*m_is_master){ send_network_cmd("set_rate " + to_string(m_playback_speed->value(), 2)); }
    }
    else if(theProperty == m_use_discovery_broadcast || theProperty == m_broadcast_port)
    {
        if(*m_use_discovery_broadcast && !*m_is_master)
        {
            // setup a periodic udp-broadcast to enable discovery of this node
            m_broadcast_timer = Timer(main_queue().io_service(), [this]()
            {
                LOG_TRACE_2 << "sending discovery_broadcast on udp-port: " << m_broadcast_port->value();
                net::async_send_udp_broadcast(background_queue().io_service(), name(),
                                              *m_broadcast_port);
            });
            m_broadcast_timer.set_periodic();
            m_broadcast_timer.expires_from_now(g_broadcast_interval);
        }else{ m_broadcast_timer.cancel(); }
    }
    else if(theProperty == m_is_master)
    {
        if(*m_is_master)
        {
            *m_use_discovery_broadcast = false;
            m_ip_timestamps.clear();

            // discovery udp-server to receive pings from existing nodes in the network
            m_udp_server = net::udp_server(background_queue().io_service());
            m_udp_server.start_listen(*m_broadcast_port);
            m_udp_server.set_receive_function([this](const std::vector<uint8_t> &data,
                                                     const std::string &remote_ip, uint16_t remote_port)
            {
                string str(data.begin(), data.end());
                LOG_TRACE_1 << str << " " << remote_ip << " (" << remote_port << ")";

                if(str == name())
                {
                    std::unique_lock<std::mutex> lock(g_ip_table_mutex);
                    m_ip_timestamps[remote_ip] = get_application_time();
                    
                    ping_delay(remote_ip);
                }
            });
            begin_network_sync();
        }
        else
        {
            m_sync_timer.cancel();
            m_use_discovery_broadcast->notify_observers();
            
            m_sync_off_timer = Timer(background_queue().io_service(), [this]()
            {
                m_media->set_rate(*m_playback_speed);
                m_is_syncing = 0;
            });
        }
    }
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::reload_media()
{
    App::Task t(this);

    textures()[TEXTURE_INPUT].reset();
    m_sync_off_timer.cancel();

    std::string abs_path;
    try{ abs_path = fs::search_file(*m_media_path); }
    catch (fs::FileNotFoundException &e){ LOG_DEBUG << e.what(); m_reload_media = false; return; }

    auto media_type = fs::get_file_type(abs_path);

    LOG_DEBUG << "loading: " << m_media_path->value();

    if(fs::is_url(*m_media_path) ||
       media_type == fs::FileType::AUDIO ||
       media_type == fs::FileType::MOVIE)
    {
        auto render_target = *m_use_warping ? media::MediaController::RenderTarget::TEXTURE :
        media::MediaController::RenderTarget::SCREEN;

        auto audio_target = *m_force_audio_jack ? media::MediaController::AudioTarget::AUDIO_JACK :
        media::MediaController::AudioTarget::AUTO;

        if(render_target == media::MediaController::RenderTarget::SCREEN)
        { set_clear_color(gl::Color(clear_color().rgb(), 0.f)); }

        m_media->set_media_ended_callback([this](media::MediaControllerPtr mc)
        {
            LOG_DEBUG << "media ended";
            
            if(*m_is_master && *m_loop)
            {
                send_network_cmd("restart");
                begin_network_sync();
            }
        });
        
        m_media->set_on_load_callback([this](media::MediaControllerPtr mc)
        {
            if(m_media->has_video() && m_media->fps() > 0)
            {
                g_sync_thresh = 1.0 / m_media->fps() / 2.0;
                LOG_DEBUG << "media fps: " << to_string(m_media->fps(), 2);
            }
            m_media->set_rate(*m_playback_speed);
            m_media->set_volume(*m_volume);
        });
        m_media->load(abs_path, *m_auto_play, *m_loop, render_target, audio_target);
    }
    else if(media_type == fs::FileType::IMAGE)
    {
        m_media->unload();
        textures()[TEXTURE_INPUT] = gl::create_texture_from_file(abs_path);
    }
    else if(media_type == fs::FileType::FONT)
    {
        m_media->unload();
        gl::Font font;
        font.load(abs_path, 44);
        textures()[TEXTURE_INPUT] = font.create_texture("The quick brown fox \njumps over the lazy dog ... \n0123456789");
    }
    m_reload_media = false;

    // network sync
    if(*m_is_master)
    {
        send_network_cmd("load " + fs::get_filename_part(*m_media_path));
        begin_network_sync();
    }
}

/////////////////////////////////////////////////////////////////

std::string MediaPlayer::secs_to_time_str(float the_secs) const
{
    char buf[32];
    sprintf(buf, "%d:%02d:%.1f", (int)the_secs / 3600, ((int)the_secs / 60) % 60, fmodf(the_secs, 60));
    return buf;
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::sync_media_to_timestamp(double the_timestamp)
{
    auto diff = the_timestamp - m_media->current_time();
    
    if(m_media->is_playing())
    {
        m_is_syncing = diff * 1000.0;
        
        // adapt to playback rate
        auto scrub_thresh = g_scrub_thresh / *m_playback_speed;
        auto sync_thresh = g_sync_thresh / *m_playback_speed;
        
        if((abs(diff) > scrub_thresh))
        {
            m_media->seek_to_time(the_timestamp);
        }
        else if(abs(diff) > sync_thresh)
        {
            auto rate = *m_playback_speed * (1.0 + sgn(diff) * 0.05 + 0.75 * diff / scrub_thresh);
            m_media->set_rate(rate);
            m_sync_off_timer.expires_from_now(g_sync_duration);
        }
        else
        {
            m_media->set_rate(*m_playback_speed);
            m_is_syncing = 0;
        }
    }
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::begin_network_sync()
{
    m_sync_timer = Timer(background_queue().io_service(), [this]()
    {
        if(m_media && m_media->is_playing()){ send_sync_cmd(); }
    });
    m_sync_timer.set_periodic();
    m_sync_timer.expires_from_now(g_sync_interval);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::send_sync_cmd()
{
    remove_dead_ip_adresses();
    
    std::unique_lock<std::mutex> lock(g_ip_table_mutex);
    bool use_udp = true;
    
    for(auto &pair : m_ip_roundtrip)
    {
        double sync_delay = median(pair.second) * (use_udp ? 0.5 : 1.5);
        string cmd = "seek_to_time " + to_string(m_media->current_time() + sync_delay, 3);
        
        if(use_udp)
        {
            net::async_send_udp(background_queue().io_service(), cmd, pair.first,
                                remote_control().udp_port());
        }
        else
        {
            net::async_send_tcp(background_queue().io_service(), cmd, pair.first,
                                remote_control().tcp_port());
        }
    }
}

void MediaPlayer::remove_dead_ip_adresses()
{
    std::unique_lock<std::mutex> lock(g_ip_table_mutex);
    
    auto now = get_application_time();
    std::list<std::unordered_map<std::string, float>::iterator> dead_iterators;
    
    auto it = m_ip_timestamps.begin();
    
    for(; it != m_ip_timestamps.end(); ++it)
    {
        if(now - it->second >= g_dead_thresh){ dead_iterators.push_back(it); }
    }
    
    // remove dead iterators
    for(auto &dead_it : dead_iterators)
    {
        m_ip_roundtrip.erase(dead_it->first);
        m_ip_timestamps.erase(dead_it);
    }
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::send_network_cmd(const std::string &the_cmd)
{
    remove_dead_ip_adresses();
    
    std::unique_lock<std::mutex> lock(g_ip_table_mutex);

    auto it = m_ip_timestamps.begin();

    for(; it != m_ip_timestamps.end(); ++it)
    {
        net::async_send_tcp(background_queue().io_service(), the_cmd, it->first,
                            remote_control().tcp_port());
    }
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::ping_delay(const std::string &the_ip)
{
    Stopwatch timer;
    timer.start();
    
    auto con = net::tcp_connection::create(background_queue().io_service(), the_ip,
                                           remote_control().tcp_port());
    auto receive_func = [this, timer, con](net::tcp_connection_ptr ptr,
                                           const std::vector<uint8_t> &data)
    {
        std::unique_lock<std::mutex> lock(g_ip_table_mutex);
        
        // we measured 2 roundtrips
        auto delay = timer.time_elapsed() * 0.5;
        
        auto it = m_ip_roundtrip.find(ptr->remote_ip());
        if(it == m_ip_roundtrip.end())
        {
            m_ip_roundtrip[ptr->remote_ip()] = CircularBuffer<double>(5);
            m_ip_roundtrip[ptr->remote_ip()].push_back(delay);
        }
        else{ it->second.push_back(delay); }
        
        LOG_TRACE << ptr->remote_ip() << " (roundtrip, last 10s): "
            << (int)(1000.0 * mean(m_ip_roundtrip[ptr->remote_ip()])) << " ms";
        
//        ptr->close();
        con->set_tcp_receive_cb();
    };
    con->set_connect_cb([this](UARTPtr the_con){ the_con->write("echo ping"); });
    con->set_tcp_receive_cb(receive_func);
}

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
        else
        {
            m_media->play();
            m_sync_off_timer.cancel();
            if(*m_is_master){ send_network_cmd("play"); }
        }
    });
    remote_control().add_command("pause");
    register_function("pause", [this](const std::vector<std::string> &rpc_args)
    {
        m_media->pause();
        m_sync_off_timer.cancel();
        if(*m_is_master){ send_network_cmd("pause"); }
    });
    remote_control().add_command("restart", [this](net::tcp_connection_ptr con,
                                                   const std::vector<std::string> &rpc_args)
    {
        m_media->restart();
        m_sync_off_timer.cancel();
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
        if(!rpc_args.empty()){ *m_volume = kinski::string_to<float>(rpc_args.front()); }
    });

    remote_control().add_command("volume", [this](net::tcp_connection_ptr con,
                                                  const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ *m_volume = kinski::string_to<float>(rpc_args.front()); }
        else{ con->write(to_string(m_media->volume())); }
    });

    remote_control().add_command("brightness", [this](net::tcp_connection_ptr con,
                                                      const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ *m_brightness = kinski::string_to<float>(rpc_args.front()); }
        else{ con->write(to_string(m_brightness->value())); }
    });

    remote_control().add_command("set_brightness", [this](net::tcp_connection_ptr con,
                                                          const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ *m_brightness = kinski::string_to<float>(rpc_args.front()); }
    });

    remote_control().add_command("set_rate");
    register_function("set_rate", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ *m_playback_speed = kinski::string_to<float>(rpc_args.front()); }
    });

    remote_control().add_command("rate", [this](net::tcp_connection_ptr con,
                                                const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ *m_playback_speed = kinski::string_to<float>(rpc_args.front()); }
        con->write(to_string(m_media->rate()));
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
            sync_media_to_timestamp(secs);
        }
    });

    remote_control().add_command("current_time", [this](net::tcp_connection_ptr con,
                                                        const std::vector<std::string> &rpc_args)
    {
        con->write(to_string(m_media->current_time(), 1));
    });

    remote_control().add_command("duration", [this](net::tcp_connection_ptr con,
                                                    const std::vector<std::string> &rpc_args)
    {
        con->write(to_string(m_media->duration(), 1));
    });

    remote_control().add_command("set_loop");
    register_function("set_loop", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ *m_loop = kinski::string_to<bool>(rpc_args.front()); }
    });

    remote_control().add_command("loop", [this](net::tcp_connection_ptr con,
                                                const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ *m_loop = kinski::string_to<bool>(rpc_args.front()); }
        con->write(to_string(m_media->loop()));
    });

    remote_control().add_command("is_playing", [this](net::tcp_connection_ptr con,
                                                      const std::vector<std::string> &rpc_args)
    {
        con->write(to_string(m_media->is_playing()));
    });
}
