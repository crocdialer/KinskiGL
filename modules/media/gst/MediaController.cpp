#include "core/file_functions.hpp"
#include "gl/Texture.hpp"

#include "MediaController.hpp"

// when we repeatedly seek, rather than play continuously
#define TRICKPLAY(speed) (speed < 0 || speed > 4 * DVD_PLAYSPEED_NORMAL)

namespace kinski{ namespace media
{
    struct MediaControllerImpl
    {
        std::string m_src_path;
        float m_rate = 1.f;
        float m_volume = 1.f;
        float m_fps = 0.f;
        bool m_loaded = false;
        bool m_has_video = false;
        bool m_has_audio = false;
        bool m_has_subtitle = false;
        bool m_pause = false;
        bool m_loop = false;
        bool m_playing = false;
        bool m_has_new_frame = false;
        MediaController::MediaCallback m_on_load_cb, m_movie_ended_cb;

        MediaController::RenderTarget m_render_target = MediaController::RenderTarget::TEXTURE;
        MediaController::AudioTarget m_audio_target = MediaController::AudioTarget::AUTO;

        MediaControllerImpl(){}

        ~MediaControllerImpl()
        {
            m_playing = false;
        };
    };

///////////////////////////////////////////////////////////////////////////////

    MediaControllerPtr MediaController::create()
    {
        return MediaControllerPtr(new MediaController());
    }

    MediaControllerPtr MediaController::create(const std::string &filePath, bool autoplay,
                                               bool loop, RenderTarget the_render_target,
                                               AudioTarget the_audio_target)
    {
        auto ptr = MediaControllerPtr(new MediaController());
        ptr->load(filePath, autoplay, loop, the_render_target, the_audio_target);
        return ptr;
    }

///////////////////////////////////////////////////////////////////////////////

    MediaController::MediaController():
            m_impl(new MediaControllerImpl)
    {

    }

    MediaController::~MediaController(){}

///////////////////////////////////////////////////////////////////////////////

    void MediaController::load(const std::string &filePath, bool autoplay, bool loop,
                               RenderTarget the_render_target, AudioTarget the_audio_target)
    {
        std::string found_path;
        if(fs::is_url(filePath)){ found_path = filePath; }
        else
        {
            try{ found_path = fs::search_file(filePath); }
            catch(fs::FileNotFoundException &e)
            {
                LOG_ERROR << e.what();
                return;
            }
        }
        MediaCallback on_load = m_impl ? m_impl->m_on_load_cb : MediaCallback();
        MediaCallback on_end = m_impl ? m_impl->m_movie_ended_cb : MediaCallback();
        m_impl.reset(new MediaControllerImpl());
        m_impl->m_src_path = found_path;
//            m_impl->m_movie_controller = shared_from_this();
        m_impl->m_on_load_cb = on_load;
        m_impl->m_movie_ended_cb = on_end;
        m_impl->m_render_target = the_render_target;
        m_impl->m_audio_target = the_audio_target;

//            m_impl->m_loaded = true;

        // fire onload callback
        if(m_impl->m_on_load_cb){ m_impl->m_on_load_cb(shared_from_this()); };

        // autoplay
        if(autoplay){ play(); }
    }

/////////////////////////////////////////////////////////////////

    void MediaController::play()
    {

    }

/////////////////////////////////////////////////////////////////

    bool MediaController::is_loaded() const
    {
        return m_impl && m_impl->m_loaded;
    }

/////////////////////////////////////////////////////////////////

    void MediaController::unload()
    {
        m_impl.reset();
    }

/////////////////////////////////////////////////////////////////

    bool MediaController::has_video() const
    {
        return m_impl && m_impl->m_has_video;
    }

/////////////////////////////////////////////////////////////////

    bool MediaController::has_audio() const
    {
        return m_impl && m_impl->m_has_audio;
    }

/////////////////////////////////////////////////////////////////

    void MediaController::pause()
    {
        if(m_impl && m_impl->m_playing){ m_impl->m_pause = true; }
    }

/////////////////////////////////////////////////////////////////

    bool MediaController::is_playing() const
    {
        return false;
    }

/////////////////////////////////////////////////////////////////

    void MediaController::restart()
    {

    }

/////////////////////////////////////////////////////////////////

    float MediaController::volume() const
    {
        return 0.f;
    }

/////////////////////////////////////////////////////////////////

    void MediaController::set_volume(float newVolume)
    {

    }

/////////////////////////////////////////////////////////////////

    bool MediaController::copy_frame(std::vector<uint8_t>& data, int *width, int *height)
    {
        return false;
    }

/////////////////////////////////////////////////////////////////

    bool MediaController::copy_frame_to_texture(gl::Texture &tex, bool as_texture2D)
    {
        return false;
    }

/////////////////////////////////////////////////////////////////

    bool MediaController::copy_frames_offline(gl::Texture &tex, bool compress)
    {
        return false;
    }

/////////////////////////////////////////////////////////////////

    double MediaController::duration() const
    {
        return 0.0;
    }

/////////////////////////////////////////////////////////////////

    double MediaController::current_time() const
    {
        return 0.0;
    }

/////////////////////////////////////////////////////////////////

    double MediaController::fps() const
    {
        double fps = 0.0;
        return fps;
    }

/////////////////////////////////////////////////////////////////

    void MediaController::seek_to_time(double value)
    {

    }

/////////////////////////////////////////////////////////////////

    void MediaController::seek_to_time(const std::string &the_time_str)
    {
        double secs = 0.0;
        auto splits = split(the_time_str, ':');

        switch(splits.size())
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
        seek_to_time(secs);
    }

/////////////////////////////////////////////////////////////////

    void MediaController::set_loop(bool b)
    {

    }

/////////////////////////////////////////////////////////////////

    bool MediaController::loop() const
    {
        return false;
    }

/////////////////////////////////////////////////////////////////

    float MediaController::rate() const
    {
        return is_loaded() ? m_impl->m_rate : 1.f;
    }

/////////////////////////////////////////////////////////////////

    void MediaController::set_rate(float r)
    {

    }

/////////////////////////////////////////////////////////////////

    const std::string& MediaController::path() const
    {
        static std::string ret;
        return is_loaded() ? m_impl->m_src_path : ret;
    }

/////////////////////////////////////////////////////////////////

    void MediaController::set_on_load_callback(MediaCallback c)
    {
        if(!m_impl){ return; }
        m_impl->m_on_load_cb = c;
    }

/////////////////////////////////////////////////////////////////

    void MediaController::set_media_ended_callback(MediaCallback c)
    {
        if(!m_impl){ return; }
        m_impl->m_movie_ended_cb = c;
    }

/////////////////////////////////////////////////////////////////

    MediaController::RenderTarget MediaController::render_target() const
    {
        return RenderTarget::TEXTURE;
    }

/////////////////////////////////////////////////////////////////

    MediaController::AudioTarget MediaController::audio_target() const
    {
        return AudioTarget::AUTO;
    }

/////////////////////////////////////////////////////////////////
}}// namespaces
