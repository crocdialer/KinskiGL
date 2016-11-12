// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  MediaPlayer.hpp
//
//  Created by Fabian on 29/01/14.

#pragma once

#include "app/ViewerApp.hpp"
#include "gl/Texture.hpp"

#include "media/media.h"

namespace kinski
{
    class MediaPlayer : public ViewerApp
    {
    private:

        enum TextureEnum{TEXTURE_INPUT = 0, TEXTURE_OUTPUT = 1};

        media::MediaControllerPtr m_media = media::MediaController::create();
        media::CameraControllerPtr m_camera_control = media::CameraController::create();
        bool m_reload_media = false, m_needs_redraw = true;
        int m_is_syncing = 0;
        Timer m_broadcast_timer, m_sync_timer, m_sync_off_timer;
        
        net::udp_server m_udp_server;
        std::unordered_map<std::string, float> m_ip_timestamps;
        std::unordered_map<std::string, CircularBuffer<double>> m_ip_delays;
        
        // properties
        Property_<string>::Ptr m_media_path = Property_<string>::create("media path", "");
        Property_<bool>::Ptr
        m_loop = Property_<bool>::create("loop", false),
        m_auto_play = Property_<bool>::create("autoplay", true),
        m_force_audio_jack = Property_<bool>::create("force 3.5mm audio-jack", false),
        m_use_discovery_broadcast = Property_<bool>::create("use discovery broadcast", true),
        m_is_master = Property_<bool>::create("is master", false);
        
        Property_<float>::Ptr
        m_playback_speed = Property_<float>::create("playback speed", 1.f),
        m_volume = RangedProperty<float>::create("volume", 1.f, 0.f , 1.f),
        m_brightness = RangedProperty<float>::create("brightness", 1.f, 0.f , 2.f);
        
        Property_<uint32_t>::Ptr
        m_broadcast_port = Property_<uint32_t>::create("discovery broadcast port", 55555);
        
        std::string secs_to_time_str(float the_secs) const;
        void setup_rpc_interface();
        
        void reload_media();
        void sync_media_to_timestamp(double the_timestamp);
        
        std::vector<std::string> get_remote_adresses() const;
        void remove_dead_ip_adresses();
        void begin_network_sync();
        void send_sync_cmd();
        void send_network_cmd(const std::string &the_cmd);
        
        net::tcp_connection_ptr m_ping_connection;
        void ping_delay(const std::string &the_ip);
        
    public:

        MediaPlayer(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
        void setup() override;
        void update(float timeDelta) override;
        void draw() override;
        void resize(int w ,int h) override;
        void keyPress(const KeyEvent &e) override;
        void keyRelease(const KeyEvent &e) override;
        void mousePress(const MouseEvent &e) override;
        void mouseRelease(const MouseEvent &e) override;
        void mouseMove(const MouseEvent &e) override;
        void mouseDrag(const MouseEvent &e) override;
        void mouseWheel(const MouseEvent &e) override;
        void touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void fileDrop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void tearDown() override;
        void update_property(const Property::ConstPtr &theProperty) override;
        
        bool needs_redraw() const override;

    };
}// namespace kinski

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::MediaPlayer>(argc, argv);
    LOG_INFO << "local ip: " << kinski::net::local_ip();
    return theApp->run();
}
