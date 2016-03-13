#include "gl/Texture.hpp"
#include "MovieController.h"

namespace kinski{ namespace video
{

    struct MovieControllerImpl
    {
        std::string m_src_path;
        gl::Texture m_texture;
        bool m_has_new_frame = false;
        MovieController::MovieCallback m_on_load_cb, m_movie_ended_cb;

        std::weak_ptr<MovieController> m_movie_controller;

        MovieControllerImpl()
        {

        }
        ~MovieControllerImpl()
        {

        };
    };

///////////////////////////////////////////////////////////////////////////////

    MovieControllerPtr MovieController::create()
    {
        return MovieControllerPtr(new MovieController());
    }

    MovieControllerPtr MovieController::create(const std::string &filePath, bool autoplay,
                                               bool loop)
    {
        return MovieControllerPtr(new MovieController(filePath, autoplay, loop));
    }

///////////////////////////////////////////////////////////////////////////////

    MovieController::MovieController()
    {

    }

    MovieController::MovieController(const std::string &filePath, bool autoplay, bool loop):
    m_impl(new MovieControllerImpl)
    {
        load(filePath, autoplay, loop);
    }

    MovieController::~MovieController()
    {

    }

///////////////////////////////////////////////////////////////////////////////

    void MovieController::load(const std::string &filePath, bool autoplay, bool loop)
    {
        LOG_DEBUG << "loading movie: " << filePath;
        string p;
        try{ p = search_file(filePath); }
        catch(FileNotFoundException &e)
        {
           LOG_WARNING << e.what();
           return;
        }
        m_impl.reset(new MovieControllerImpl);
        m_impl->m_src_path = p;
        m_impl->m_movie_controller = shared_from_this();
    }

    void MovieController::play()
    {
        LOG_DEBUG << "starting movie playback";
        if(!m_impl){ return; }
    }

    void MovieController::unload()
    {
        m_impl.reset(new MovieControllerImpl);
    }

    void MovieController::pause()
    {
        if(!m_impl){ return; }
    }

    bool MovieController::is_playing() const
    {
        return m_impl && false;
    }

    void MovieController::restart()
    {
        seek_to_time(0);
        play();
    }

    float MovieController::volume() const
    {
        return 0.f;//m_impl->m_player.volume;
    }

    void MovieController::set_volume(float newVolume)
    {
        float val = clamp(newVolume, 0.f, 1.f);
        LOG_WARNING << "set_volume NOT IMPLRMENTED: " << val;
    }

    bool MovieController::copy_frame(std::vector<uint8_t>& data, int *width, int *height)
    {
        return false;
    }

    bool MovieController::copy_frame_to_texture(gl::Texture &tex, bool as_texture2D)
    {
        if(!m_impl || !m_impl->m_has_new_frame){ return false; }
        tex = m_impl->m_texture;
        tex.setFlipped();
        m_impl->m_has_new_frame = false;
        return true;
    }

    bool MovieController::copy_frames_offline(gl::Texture &tex, bool compress)
    {
        LOG_WARNING << "copy_frames_offline not available on RPI";
        return false;
    }

    double MovieController::duration() const
    {
        return 0.0;
    }

    double MovieController::current_time() const
    {
        return 0.0;
    }

    void MovieController::seek_to_time(float value)
    {
        if(!m_impl){ return; }
    }

    void MovieController::set_loop(bool b)
    {

    }

    bool MovieController::loop() const
    {
        return m_impl && false;
    }

    void MovieController::set_rate(float r)
    {
        if(!m_impl){ return; }
    }

    const std::string& MovieController::get_path() const
    {
        static std::string ret;
        return m_impl ? m_impl->m_src_path : ret;
    }

    void MovieController::set_on_load_callback(MovieCallback c)
    {
        if(!m_impl){ return; }
        m_impl->m_on_load_cb = c;
    }

    void MovieController::set_movie_ended_callback(MovieCallback c)
    {
        if(!m_impl){ return; }
        m_impl->m_movie_ended_cb = c;
    }
}}// namespaces
