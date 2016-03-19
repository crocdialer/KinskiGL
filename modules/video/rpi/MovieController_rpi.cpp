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
    std::shared_ptr<COMXCore> m_OMX;

    struct MovieControllerImpl
    {
        std::string m_src_path;
        gl::Texture m_texture;
        float m_rate = 1.f;
        float m_volume = 1.f;
        float m_fps = 0.f;
        bool m_loop = false;
        bool m_playing = false;
        bool m_has_new_frame = false;
        MovieController::MovieCallback m_on_load_cb, m_movie_ended_cb;
        std::weak_ptr<MovieController> m_movie_controller;
        std::thread m_thread;

        // bridge EGL -> gl::Texture
        void* m_egl_image = nullptr;

        // COMXCore m_OMX;
        OMXReader m_omx_reader;
        std::shared_ptr<OMXPlayerVideo> m_player_video;
        std::shared_ptr<OMXPlayerAudio> m_player_audio;
        std::shared_ptr<OMXClock> m_av_clock;
        OMXPacket* m_omx_pkt = nullptr;
        OMXAudioConfig m_config_audio;
        OMXVideoConfig m_config_video;
        bool m_has_video = false;
        bool m_has_audio = false;
        bool m_has_subtitle = false;
        bool m_pause = false;
        bool m_seek_flush = false;
        bool m_packet_after_seek = false;
        bool sentStarted = false;
        bool m_send_eos = false;
        bool m_chapter_seek = false;
        double startpts = 0;
        double m_incr = 0;
        double m_loop_from = 0;
        double last_seek_pos = 0;
        float m_threshold = 0.2f; // amount of audio/video required to come out of buffering
        float m_timeout = 10.0f; // amount of time file/network operation can stall for before timing out

        // int playspeed_current = 14;
        double m_last_check_time = 0.0;

        MovieControllerImpl()
        {
            if(!m_OMX)
            {
                m_OMX.reset(new COMXCore(), [](COMXCore *c){c->Deinitialize();});
                m_OMX->Initialize();
            }
            m_av_clock.reset(new OMXClock());
        }
        ~MovieControllerImpl()
        {
            m_playing = false;

            try{ if(m_thread.joinable()){ m_thread.join(); } }
            catch(std::exception &e){ LOG_ERROR << e.what(); }

            m_av_clock->OMXStop();
            m_av_clock->OMXStateIdle();

            // m_player_subtitles.Close();
            m_player_video->Close();
            m_player_audio->Close();

            if(m_omx_pkt)
            {
              m_omx_reader.FreePacket(m_omx_pkt);
              m_omx_pkt = nullptr;
            }
            m_omx_reader.Close();
            m_av_clock->OMXDeinitialize();

            if(m_egl_image)
            {
                if(!eglDestroyImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY),
                                       (EGLImageKHR)m_egl_image))
                {
                    LOG_ERROR << "eglDestroyImageKHR failed.";
                }
            }
            // m_OMX.Deinitialize();
        };

        void flush_stream(double pts)
        {
            m_av_clock->OMXStop();
            m_av_clock->OMXPause();
            if(m_has_video){ m_player_video->Flush(); }
            if(m_has_audio){ m_player_audio->Flush(); }
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
            LOG_DEBUG << "starting movie decode thread";
            m_playing = true;
            sentStarted = true;
            size_t num_frames = 0;

            while(m_playing)
            {
                double now = m_av_clock->GetAbsoluteClock();
                bool update = false;
                if (m_last_check_time == 0.0 || m_last_check_time + DVD_MSEC_TO_TIME(20) <= now)
                {
                    update = true;
                    m_last_check_time = now;
                }

                if(m_seek_flush || m_incr != 0)
                {
                    double seek_pos = 0;
                    double pts = 0;

                    // if(m_has_subtitle)
                    //     m_player_subtitles.Pause();
                    if(!m_chapter_seek)
                    {
                        pts = m_av_clock->OMXMediaTime();

                        seek_pos = (pts ? pts / DVD_TIME_BASE : last_seek_pos) + m_incr;
                        last_seek_pos = seek_pos;

                        seek_pos *= 1000.0;

                        if(m_omx_reader.SeekTime((int)seek_pos, m_incr < 0.0f, &startpts))
                        {
                            //   unsigned t = (unsigned)(startpts*1e-6);
                            //   auto dur = m_omx_reader.GetStreamLength() / 1000;
                            //   printf("Seek to: %02d:%02d:%02d\n", (t/3600), (t/60)%60, t%60);
                            flush_stream(startpts);
                        }
                    }

                    sentStarted = false;

                    if(m_omx_reader.IsEof()){ break; }

                    // Quick reset to reduce delay during loop & seek.
                    if(m_has_video && !m_player_video->Reset())
                    {
                        LOG_ERROR << "m_player_video->Reset() failed";
                        break;
                    }

                    kinski::log(Severity::DEBUG, "Seeked %.0f %.0f %.0f", DVD_MSEC_TO_TIME(seek_pos),
                                startpts, m_av_clock->OMXMediaTime());

                    m_av_clock->OMXPause();

                    // if(m_has_subtitle)
                    //     m_player_subtitles.Resume();

                    m_packet_after_seek = false;
                    m_seek_flush = false;
                    m_incr = 0;
                }
                else if(m_packet_after_seek && TRICKPLAY(m_av_clock->OMXPlaySpeed()))
                {
                    double seek_pos = 0;
                    double pts = 0;

                    pts = m_av_clock->OMXMediaTime();
                    seek_pos = (pts / DVD_TIME_BASE);

                    seek_pos *= 1000.0;

                    if(m_omx_reader.SeekTime((int)seek_pos, m_av_clock->OMXPlaySpeed() < 0, &startpts))
                    ; //FlushStreams(DVD_NOPTS_VALUE);

                    kinski::log(Severity::DEBUG, "Seeked %.0f %.0f %.0f", DVD_MSEC_TO_TIME(seek_pos),
                                startpts, m_av_clock->OMXMediaTime());
                    m_packet_after_seek = false;
                }

                /* player got in an error state */
                if(m_player_audio && m_player_audio->Error())
                {
                    LOG_ERROR << "audio player error. emergency exit!!!";
                    break;
                }

                if(update)
                {
                    /* when the video/audio fifos are low, we pause clock, when high we resume */
                    double stamp = m_av_clock->OMXMediaTime();
                    double audio_pts = m_player_audio ? m_player_audio->GetCurrentPTS() : DVD_NOPTS_VALUE;
                    double video_pts = m_player_video->GetCurrentPTS();

                    if (0 && m_av_clock->OMXIsPaused())
                    {
                        double old_stamp = stamp;
                        if (audio_pts != DVD_NOPTS_VALUE && (stamp == 0 || audio_pts < stamp))
                          stamp = audio_pts;
                        if (video_pts != DVD_NOPTS_VALUE && (stamp == 0 || video_pts < stamp))
                          stamp = video_pts;
                        if (old_stamp != stamp)
                        {
                          m_av_clock->OMXMediaTime(stamp);
                          stamp = m_av_clock->OMXMediaTime();
                        }
                    }

                    float audio_fifo = audio_pts == DVD_NOPTS_VALUE ? 0.0f : audio_pts / DVD_TIME_BASE - stamp * 1e-6;
                    float video_fifo = video_pts == DVD_NOPTS_VALUE ? 0.0f : video_pts / DVD_TIME_BASE - stamp * 1e-6;
                    float threshold = std::min(0.1f,
                                               m_player_audio ? (float)m_player_audio->GetCacheTotal() * 0.1f : 0.f);
                    bool audio_fifo_low = false, video_fifo_low = false, audio_fifo_high = false,
                        video_fifo_high = false;

                    if(audio_pts != DVD_NOPTS_VALUE)
                    {
                        audio_fifo_low = m_has_audio && audio_fifo < threshold;
                        audio_fifo_high = !m_has_audio || (audio_pts != DVD_NOPTS_VALUE && audio_fifo > m_threshold);
                    }
                    if (video_pts != DVD_NOPTS_VALUE)
                    {
                        video_fifo_low = m_has_video && video_fifo < threshold;
                        video_fifo_high = !m_has_video || (video_pts != DVD_NOPTS_VALUE && video_fifo > m_threshold);
                    }

                    if(!m_pause && (m_omx_reader.IsEof() || m_omx_pkt ||
                            TRICKPLAY(m_av_clock->OMXPlaySpeed()) || (audio_fifo_high && video_fifo_high)))
                    {
                        if(m_av_clock->OMXIsPaused())
                        {
                            kinski::log(Severity::DEBUG, "Resume %.2f,%.2f (%d,%d,%d,%d) EOF:%d PKT:%p",
                                        audio_fifo, video_fifo, audio_fifo_low, video_fifo_low,
                                        audio_fifo_high, video_fifo_high, m_omx_reader.IsEof(),
                                        m_omx_pkt);
                            m_av_clock->OMXResume();
                        }
                    }
                    else if(m_pause || audio_fifo_low || video_fifo_low)
                    {
                        if(!m_av_clock->OMXIsPaused())
                        {
                            if(!m_pause){ m_threshold = std::min(2.0f * m_threshold, 16.0f); }
                            kinski::log(Severity::DEBUG, "Pause %.2f,%.2f (%d,%d,%d,%d) %.2f",
                                        audio_fifo, video_fifo, audio_fifo_low, video_fifo_low,
                                        audio_fifo_high, video_fifo_high, m_threshold);
                            m_av_clock->OMXPause();
                        }
                    }
                }
                if(!sentStarted)
                {
                    kinski::log(Severity::DEBUG, "COMXPlayer::HandleMessages - player started RESET");
                    m_av_clock->OMXReset(m_has_video, m_has_audio);
                    sentStarted = true;
                }

                if(!m_omx_pkt){ m_omx_pkt = m_omx_reader.Read(); }
                if(m_omx_pkt){ m_send_eos = false; }

                if(m_omx_reader.IsEof() && !m_omx_pkt)
                {
                    // demuxer EOF, but may have not played out data yet
                    if( (m_has_video && m_player_video->GetCached()) ||
                       (m_has_audio && m_player_audio->GetCached()) )
                    {
                        OMXClock::OMXSleep(10);
                        continue;
                    }
                    if(!m_send_eos && m_has_video){ m_player_video->SubmitEOS(); }
                    if (!m_send_eos && m_has_audio){ m_player_audio->SubmitEOS(); }
                    m_send_eos = true;

                    if( (m_has_video && !m_player_video->IsEOS()) ||
                       (m_has_audio && !m_player_audio->IsEOS()) )
                    {
                        OMXClock::OMXSleep(10);
                        continue;
                    }

                    // fire movie ended callback
                    if(m_movie_ended_cb){ m_movie_ended_cb(m_movie_controller.lock()); }

                    if(m_loop)
                    {
                        m_incr = m_loop_from - (m_av_clock->OMXMediaTime() ? m_av_clock->OMXMediaTime() / DVD_TIME_BASE : last_seek_pos);
                        continue;
                    }
                    break;
                }

                if(m_has_video && m_omx_pkt && m_omx_reader.IsActive(OMXSTREAM_VIDEO, m_omx_pkt->stream_index))
                {
                    if(TRICKPLAY(m_av_clock->OMXPlaySpeed()))
                    {
                        m_packet_after_seek = true;
                    }
                    if(m_player_video->AddPacket(m_omx_pkt))
                    {
                         m_omx_pkt = nullptr;
                         num_frames++;
                         m_has_new_frame = true;
                    }
                    else{ OMXClock::OMXSleep(10); }
                }
                else if(m_has_audio && m_omx_pkt && !TRICKPLAY(m_av_clock->OMXPlaySpeed()) &&
                        m_omx_pkt->codec_type == AVMEDIA_TYPE_AUDIO)
                {
                    if(m_player_audio->AddPacket(m_omx_pkt)){ m_omx_pkt = nullptr; }
                    else{ OMXClock::OMXSleep(10); }
                }
                // else if(m_has_subtitle && m_omx_pkt && !TRICKPLAY(m_av_clock->OMXPlaySpeed()) &&
                //         m_omx_pkt->codec_type == AVMEDIA_TYPE_SUBTITLE)
                // {
                //     auto result = m_player_subtitles.AddPacket(m_omx_pkt,
                //                   m_omx_reader.GetRelativeIndex(m_omx_pkt->stream_index));
                //     if (result){ m_omx_pkt = nullptr; }
                //     else{ OMXClock::OMXSleep(10); }
                // }
                else
                {
                    if(m_omx_pkt)
                    {
                        m_omx_reader.FreePacket(m_omx_pkt);
                        m_omx_pkt = nullptr;
                    }
                    else{ OMXClock::OMXSleep(10); }
                }

                // std::this_thread::sleep_for(std::chrono::milliseconds(35));
            }
            m_playing = false;
            LOG_DEBUG << "movie decode thread ended: " << m_playing;
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
        MovieCallback on_load = m_impl ? m_impl->m_on_load_cb : MovieCallback();
        MovieCallback on_end = m_impl ? m_impl->m_movie_ended_cb : MovieCallback();
        m_impl.reset(new MovieControllerImpl());
        m_impl->m_movie_controller = shared_from_this();
        m_impl->m_on_load_cb = on_load;
        m_impl->m_movie_ended_cb = on_end;

        try{ m_impl->m_src_path = kinski::search_file(filePath); }
        catch(FileNotFoundException &e)
        {
            LOG_ERROR << e.what();
            return;
        }

        m_impl->m_player_audio.reset(new OMXPlayerAudio());
        m_impl->m_player_video.reset(new OMXPlayerVideo());

        if(!m_impl->m_omx_reader.Open(m_impl->m_src_path.c_str(), true/*m_dump_format*/))
        {
            m_impl.reset();
            LOG_WARNING << "could not open file: " << m_impl->m_src_path;
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
        // pass our egl_image for buffer creation
        m_impl->m_config_video.egl_image = m_impl->m_egl_image;

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

        if (m_impl->m_config_audio.device == "")
        {
        //   if(vc_tv_hdmi_audio_supported(EDID_AudioFormat_ePCM, 2, EDID_AudioSampleRate_e44KHz,
        //                                 EDID_AudioSampleSize_16bit ) == 0)
        //   { m_config_audio.device = "omx:hdmi"; }
        //   else
            { m_impl->m_config_audio.device = "omx:local"; }
        }

        if (m_impl->m_fps > 0.0f)
        {
            m_impl->m_config_video.hints.fpsrate = m_impl->m_fps * DVD_TIME_BASE,
            m_impl->m_config_video.hints.fpsscale = DVD_TIME_BASE;
        }

        if(m_impl->m_has_video && !m_impl->m_player_video->Open(m_impl->m_av_clock.get(),
                                                                m_impl->m_config_video))
        {
            m_impl.reset();
            LOG_WARNING << "could not open video player";
            return;
        }
        if(m_impl->m_has_audio && !m_impl->m_player_audio->Open(m_impl->m_av_clock.get(),
                                                                m_impl->m_config_audio,
                                                                &m_impl->m_omx_reader))
        {
            m_impl.reset();
            LOG_WARNING << "could not open audio player";
            return;
        }
        if(m_impl->m_has_audio)
        {
            m_impl->m_player_audio->SetVolume(m_impl->m_volume);
        //   if(m_Amplification)
        //     m_player_audio->SetDynamicRangeCompression(m_Amplification);
        }
        m_impl->m_av_clock->OMXReset(m_impl->m_has_video, m_impl->m_has_audio);
        m_impl->m_av_clock->OMXStateExecute();

        // fire on load callback, if any
        if(m_impl->m_on_load_cb){ m_impl->m_on_load_cb(shared_from_this()); }

        // autoplay
        if(autoplay){ play(); }
    }

    void MovieController::play()
    {
        if(!m_impl || m_impl->m_playing){ return; }
        LOG_DEBUG << "starting movie playback";

        m_impl->m_playing = true;
        set_rate(m_impl->m_rate);
        m_impl->m_thread = std::thread(std::bind(&MovieControllerImpl::thread_func, m_impl.get()));
    }

    bool MovieController::is_loaded() const
    {
        return m_impl.get();
    }

    void MovieController::unload()
    {
        m_impl.reset();
    }

    bool MovieController::has_video() const
    {
        return m_impl && m_impl->m_has_video;
    }

    bool MovieController::has_audio() const
    {
        return m_impl && m_impl->m_has_audio;
    }

    void MovieController::pause()
    {
        if(m_impl){ m_impl->m_pause = !m_impl->m_pause; }
    }

    bool MovieController::is_playing() const
    {
        return m_impl && m_impl->m_playing;
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
        if(!m_impl || !m_impl->m_has_audio){ return; }
        m_impl->m_volume = clamp(newVolume, 0.f, 1.f);
        m_impl->m_player_audio->SetVolume(m_impl->m_volume);
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
        return m_impl ? (m_impl->m_omx_reader.GetStreamLength() / 1000.0) : 0.0;
    }

    double MovieController::current_time() const
    {
        return m_impl ? m_impl->m_av_clock->OMXMediaTime() / (double)DVD_TIME_BASE : 0.0;
    }

    void MovieController::seek_to_time(float value)
    {
        if(!m_impl){ return; }

        if(m_impl->m_omx_reader.CanSeek())
        {
            m_impl->m_incr = -current_time() + value;
            m_impl->m_seek_flush = true;
        }
    }

    void MovieController::set_loop(bool b)
    {
        if(m_impl){ m_impl->m_loop = b; }
    }

    bool MovieController::loop() const
    {
        return m_impl && m_impl->m_loop;
    }

    float MovieController::rate() const
    {
        return m_impl ? m_impl->m_rate : 1.f;
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
