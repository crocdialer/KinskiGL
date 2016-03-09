#pragma once

#include "gl/gl.hpp"

/*
* This class controls playback of Movies and manages their assets.
*/

namespace kinski{ namespace video{

    class MovieController;
    typedef std::shared_ptr<MovieController> MovieControllerPtr;

    class MovieController : public std::enable_shared_from_this<MovieController>
    {
    public:

        typedef std::function<void(MovieControllerPtr the_movie)> MovieCallback;

        static MovieControllerPtr create();
        static MovieControllerPtr create(const std::string &filePath, bool autoplay = false,
                                         bool loop = false);


        virtual ~MovieController();

        void load(const std::string &filePath, bool autoplay = false, bool loop = false);
        void unload();
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
        void set_rate(float r);
        const std::string& get_path() const;

        void set_on_load_callback(MovieCallback c);
        void set_movie_ended_callback(MovieCallback c);

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

        MovieController();
        MovieController(const std::string &filePath, bool autoplay = false, bool loop = false);
        std::unique_ptr<struct MovieControllerImpl> m_impl;
    };
}}// namespaces
