#include <thread>
#include "EGL/egl.h"
#include "EGL/eglext.h"
#undef countof

#include "OMXClock.h"
#include "OMXAudio.h"
#include "OMXReader.h"
#include "OMXPlayerVideo.h"
#include "OMXPlayerAudio.h"

#include "gl/Texture.hpp"
#include "MovieController.h"

// when we repeatedly seek, rather than play continuously
#define TRICKPLAY(speed) (speed < 0 || speed > 4 * DVD_PLAYSPEED_NORMAL)

namespace kinski{ namespace video
{
    struct MovieControllerImpl
    {
        std::string m_src_path;
        gl::Texture m_texture;
        float m_rate = 1.f;
        float m_volume = 1.f;
        bool m_loop = false;
        bool m_playing = false;
        bool m_has_new_frame = false;
        MovieController::MovieCallback m_on_load_cb, m_movie_ended_cb;
        std::weak_ptr<MovieController> m_movie_controller;
        std::thread m_thread;

        // bridge EGL -> gl::Texture
        OMX_BUFFERHEADERTYPE* m_egl_buffer = nullptr;
        void* m_egl_image = nullptr;

        COMXCore m_OMX;
        OMXReader m_omx_reader;
        OMXPlayerVideo m_player_video;
        OMXPlayerAudio m_player_audio;
        OMXClock* m_av_clock = new OMXClock();
        OMXPacket* m_omx_pkt = nullptr;
        OMXAudioConfig m_config_audio;
        OMXVideoConfig m_config_video;
        bool m_has_video = false;
        bool m_has_audio = false;
        bool m_has_subtitle = false;

        MovieControllerImpl()
        {
            m_OMX.Initialize();
        }
        ~MovieControllerImpl()
        {
            m_playing = false;

            try{ if(m_thread.joinable()){ m_thread.join(); } }
            catch(std::exception &e){ LOG_ERROR << e.what(); }

            m_av_clock->OMXStop();
            m_av_clock->OMXStateIdle();

            // m_player_subtitles.Close();
            m_player_video.Close();
            m_player_audio.Close();

            if(m_omx_pkt)
            {
              m_omx_reader.FreePacket(m_omx_pkt);
              m_omx_pkt = nullptr;
            }
            m_omx_reader.Close();
            m_av_clock->OMXDeinitialize();
            if(m_av_clock){ delete m_av_clock; }

            if(m_egl_image)
            {
                if(!eglDestroyImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY),
                                       (EGLImageKHR)m_egl_image))
                {
                    LOG_ERROR << "eglDestroyImageKHR failed.";
                }
            }
            m_OMX.Deinitialize();
        };

        void flush_stream(double pts)
        {
            m_av_clock->OMXStop();
            m_av_clock->OMXPause();
            if(m_has_video){ m_player_video.Flush(); }
            if(m_has_audio){ m_player_audio.Flush(); }
            if(pts != DVD_NOPTS_VALUE){ m_av_clock->OMXMediaTime(pts); }
            // if(m_has_subtitle){ m_player_subtitles.Flush(); }
            if(m_omx_pkt)
            {
                m_omx_reader.FreePacket(m_omx_pkt);
                m_omx_pkt = nullptr;
            }
        }

        void thread_func()
        {
            while(m_playing)
            {

            }
        }
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

        if(!m_impl->m_omx_reader.Open(p.c_str(), true/*m_dump_format*/))
        {
            m_impl.reset();
            LOG_WARNING << "could not open file: " << p;
            return;
        }

        m_impl->m_has_video = m_impl->m_omx_reader.VideoStreamCount();
        m_impl->m_has_audio = m_impl->m_omx_reader.AudioStreamCount();
        m_impl->m_loop = loop && m_impl->m_omx_reader.CanSeek();

        // create texture
        m_impl->m_texture = gl::Texture(m_impl->m_omx_reader.GetWidth(),
                                        m_impl->m_omx_reader.GetHeight());

        // create EGL image
        m_impl->m_egl_image = eglCreateImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY),
                                                eglGetCurrentContext(),
                                                EGL_GL_TEXTURE_2D_KHR,
                                                (EGLClientBuffer)m_impl->m_texture.getId(),
                                                0);

        if(!m_impl->m_av_clock->OMXInitialize())
        {
            m_impl.reset();
            LOG_WARNING << "could not init clock";
            return;
        }
        m_impl->m_av_clock->OMXStateIdle();
        m_impl->m_av_clock->OMXStop();
        m_impl->m_av_clock->OMXPause();
        m_impl->m_omx_reader.GetHints(OMXSTREAM_AUDIO, m_impl->m_config_audio.hints);
        m_impl->m_omx_reader.GetHints(OMXSTREAM_VIDEO, m_impl->m_config_video.hints);

        // if (m_fps > 0.0f)
        //   m_config_video.hints.fpsrate = m_fps * DVD_TIME_BASE, m_config_video.hints.fpsscale = DVD_TIME_BASE;

        if(m_impl->m_has_video && !m_impl->m_player_video.Open(m_impl->m_av_clock,
                                                               m_impl->m_config_video))
        {
            m_impl.reset();
            LOG_WARNING << "could not open video player";
            return;
        }
        if(m_impl->m_has_audio && !m_impl->m_player_audio.Open(m_impl->m_av_clock,
                                                               m_impl->m_config_audio,
                                                               &m_impl->m_omx_reader))
        {
            m_impl.reset();
            LOG_WARNING << "could not open audio player";
            return;
        }
        if(m_impl->m_has_audio)
        {
            m_impl->m_player_audio.SetVolume(m_impl->m_volume);
        //   if(m_Amplification)
        //     m_player_audio.SetDynamicRangeCompression(m_Amplification);
        }
        m_impl->m_av_clock->OMXReset(m_impl->m_has_video, m_impl->m_has_audio);
        m_impl->m_av_clock->OMXStateExecute();
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
        return m_impl ? m_impl->m_volume : 0.f;
    }

    void MovieController::set_volume(float newVolume)
    {
        if(!m_impl){ return; }
        float val = clamp(newVolume, 0.f, 1.f);
        m_impl->m_volume = val;
        m_impl->m_player_audio.SetVolume(m_impl->m_volume);
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
        if(!m_impl || !m_impl->m_av_clock){ return; }

        int iSpeed = DVD_PLAYSPEED_NORMAL * r;
        m_impl->m_omx_reader.SetSpeed(iSpeed);

        // flush when in trickplay mode
        if(TRICKPLAY(iSpeed) || TRICKPLAY(m_impl->m_av_clock->OMXPlaySpeed()))
        { m_impl->flush_stream(DVD_NOPTS_VALUE); }

        m_impl->m_av_clock->OMXSetSpeed(iSpeed);
        m_impl->m_av_clock->OMXSetSpeed(iSpeed, true, true);
        m_impl->m_rate = r;
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
