#pragma once

#include "gl/gl.hpp"

/*
* This class controls playback of media-files or streams and manages their assets.
*/

namespace kinski{ namespace media{

    class MediaController;
    typedef std::shared_ptr<MediaController> MediaControllerPtr;
    
    typedef MediaController MovieController;
    typedef MediaControllerPtr MovieControllerPtr;

    class MediaController : public std::enable_shared_from_this<MediaController>
    {
    public:

        enum class RenderTarget{SCREEN, TEXTURE};
        enum class AudioTarget{AUTO, HDMI, AUDIO_JACK};
        
        typedef std::function<void(MediaControllerPtr the_movie)> MediaCallback;

        static MediaControllerPtr create();
        static MediaControllerPtr create(const std::string &filePath, bool autoplay = false,
                                         bool loop = false,
                                         RenderTarget the_render_target = RenderTarget::TEXTURE,
                                         AudioTarget the_audio_target = AudioTarget::HDMI);


        virtual ~MediaController();

        void load(const std::string &the_path, bool autoplay = false, bool loop = false,
                  RenderTarget the_target = RenderTarget::TEXTURE,
                  AudioTarget the_audio_target = AudioTarget::HDMI);
        void unload();
        bool is_loaded() const;
        bool has_video() const;
        bool has_audio() const;
        void play();
        void restart();
        void pause();
        bool is_playing() const;
        void seek_to_time(float value);
        double duration() const;
        double current_time() const;
        float volume() const;
        void set_volume(float volume);
        bool loop() const;
        void set_loop(bool b);
        float rate() const;
        void set_rate(float r);
        const std::string& path() const;
        
        RenderTarget render_target() const;
        AudioTarget audio_target() const;

        void set_on_load_callback(MediaCallback c);
        void set_media_ended_callback(MediaCallback c);

        /*!
         * upload the current frame to the_texture with target GL_RECTANGLE as default
         * or GL_TEXTURE_2D if as_texture2D is true
         * return: true if a new frame has been uploaded successfully,
         * false otherwise
         */
        bool copy_frame_to_texture(gl::Texture &the_texture, bool as_texture2D = false);

        /*!
         * copy the current frame to a std::vector<uint8_t>
         * return: true if a new frame has been copied successfully,
         * false otherwise
         */
        bool copy_frame(std::vector<uint8_t>& data, int *width = nullptr, int *height = nullptr);

        /*!
         * upload all frames to a gl::Texture object with target GL_TEXTURE_2D_ARRAY
         * return: true if all frames have been uploaded successfully,
         * false otherwise
         */
        bool copy_frames_offline(gl::Texture &tex, bool compress = false);

    private:

        MediaController();
        std::unique_ptr<struct MediaControllerImpl> m_impl;
    };
}}// namespaces
