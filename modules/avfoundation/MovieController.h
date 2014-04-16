#ifndef __kinskiGL__MovieController__
#define __kinskiGL__MovieController__

#include "kinskiGL/KinskiGL.h"

/*
* This class controls playback of Movies and manages their assets.
*/

namespace kinski
{
    class MovieController 
    {
    private:
        
        struct Impl;
        std::shared_ptr<Impl> m_impl;
        
    public:
        
        typedef std::function<void()> Callback;
        typedef std::function<void(MovieController &the_movie)> MovieCallback;
        
        MovieController();
        virtual ~MovieController();
        
        void load(const std::string &filePath, bool autoplay = false);
        void play();
        bool isPlaying() const;
        void stop();
        void pause();
        void seek_to_time(float value);
        double duration() const;
        double current_time() const;
        float volume() const;
        void set_volume(float volume);
        void set_loop(bool b);
        void set_rate(float r);
        bool loop() const;
        const std::string& get_path() const;
        
        void set_on_load_callback(MovieCallback c);
        
        /*!
         * upload the current frame to a gl::Texture object
         * return: true if a new frame has been uploaded successfully,
         * false otherwise
         */
        bool copy_frame_to_texture(gl::Texture &tex);
        
        /*!
         * copy the current frame to a std::vector<uint8_t>
         * return: true if a new frame has been copied successfully,
         * false otherwise
         */
        bool copy_frame(std::vector<uint8_t>& data);
        
        bool copy_frames_offline(gl::ArrayTexture &tex);
    };
}

#endif
