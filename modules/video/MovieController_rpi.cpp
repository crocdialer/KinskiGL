// Raspian defines and includes
#define USE_VCHIQ_ARM
#define OMX_SKIP64BIT

#include "bcm_host.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "ilclient.h"
#undef countof

extern "C"
{
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  // #include <libavutil/avutil.h>
  #include <libavutil/mathematics.h>
}

#include <thread>
#include "gl/Texture.hpp"
#include "gl/Buffer.hpp"
#include "MovieController.h"

namespace kinski{ namespace video{


    struct MovieControllerImpl
    {
        std::string m_src_path;
        volatile bool m_playing;
        volatile bool m_has_new_frame;
        volatile bool m_loop;
        float m_rate;
        std::weak_ptr<MovieController> m_movie_controller;
        MovieController::MovieCallback m_on_load_cb, m_movie_ended_cb;


        OMX_BUFFERHEADERTYPE* m_egl_buffer;

        COMPONENT_T* m_egl_render = nullptr;
        COMPONENT_T* m_video_decode = nullptr;
        COMPONENT_T* m_video_scheduler = nullptr;
        COMPONENT_T* m_clock = nullptr;
        COMPONENT_T* m_comp_list[5];

        TUNNEL_T m_tunnels[5];
        ILCLIENT_T* m_il_client;

        OMX_VIDEO_PARAM_PORTFORMATTYPE m_port_format;
        OMX_TIME_CONFIG_CLOCKSTATETYPE m_clock_state;

        // EGL bridge -> gl::Texture
        void* m_egl_image;
        gl::Texture m_texture;

        // libav stuff
        AVFormatContext* m_av_format_context = nullptr;
        AVCodecContext* m_av_codec_ctx = nullptr;
        int m_av_video_stream_idx = -1;
        double m_duration = 0, m_current_time = 0;

        std::thread m_thread;

        MovieControllerImpl():
        m_src_path(""),
        m_playing(false),
        m_has_new_frame(false),
        m_loop(false),
        m_rate(1.f),
        m_egl_buffer(nullptr),
        m_egl_image(nullptr)
        {
            // ffmpeg init
            av_register_all();

            // init the IL-client library
            m_il_client = ilclient_init();

            LOG_ERROR_IF(OMX_Init() != OMX_ErrorNone)  << "OMX_Init failed.";

            memset(m_comp_list, 0, sizeof(m_comp_list));
            memset(m_tunnels, 0, sizeof(m_tunnels));

            memset(&m_clock_state, 0, sizeof(m_clock_state));
            m_clock_state.nSize = sizeof(m_clock_state);
            m_clock_state.nVersion.nVersion = OMX_VERSION;
            m_clock_state.eState = OMX_TIME_ClockStateWaitingForStartTime;
            m_clock_state.nWaitMask = 1;

            memset(&m_port_format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
            m_port_format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
            m_port_format.nVersion.nVersion = OMX_VERSION;
            m_port_format.nPortIndex = 130;
            m_port_format.eCompressionFormat = OMX_VIDEO_CodingAVC;
            // m_port_format.eCompressionFormat = OMX_VIDEO_CodingAutoDetect;

        }
        ~MovieControllerImpl()
        {
            m_playing = false;

            try{ if(m_thread.joinable()){ m_thread.join(); } }
            catch(std::exception &e){ LOG_ERROR << e.what(); }

            // ilclient_set_fill_buffer_done_callback(m_il_client, nullptr, nullptr);

            // LOG_DEBUG << "destroy tunnels";
            ilclient_disable_tunnel(m_tunnels);
            ilclient_disable_tunnel(m_tunnels + 1);
            ilclient_disable_tunnel(m_tunnels + 2);
            ilclient_disable_tunnel(m_tunnels + 3);
            ilclient_teardown_tunnels(m_tunnels);

            // LOG_DEBUG << "shutdown components (skipped)";
            ilclient_state_transition(m_comp_list, OMX_StateIdle);
            // ilclient_state_transition(m_comp_list, OMX_StateLoaded);

            // LOG_DEBUG << "cleanup components";
            ilclient_cleanup_components(m_comp_list);

            // LOG_DEBUG << "shutdown OMX";
            OMX_Deinit();
            ilclient_destroy(m_il_client);

            if(m_egl_image)
            {
                // LOG_DEBUG << "destroy egl_image";

                if(!eglDestroyImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY),
                                       (EGLImageKHR)m_egl_image))
                {
                    LOG_ERROR << "eglDestroyImageKHR failed.";
                }
            }

            // close codec
            if(m_av_codec_ctx){ avcodec_close(m_av_codec_ctx); }
            if(m_av_format_context){ avformat_close_input(&m_av_format_context); }
            // LOG_DEBUG << "impl desctructor finished";
        };

        void thread_func()
        {
            LOG_DEBUG << "starting movie decode thread";
            m_playing = true;
            size_t data_len = 0;

            OMX_BUFFERHEADERTYPE *buf = nullptr;
            int port_settings_changed = 0;
            int first_packet = 1;

            ilclient_change_component_state(m_video_decode, OMX_StateExecuting);

            // allocate ffmpeg frame structure
            // AvFrame *frame = av_frame_alloc();
            AVPacket current_packet;
            current_packet.size = 0;
            current_packet.data = nullptr;
            current_packet.stream_index = -1;
            std::list<AVPacket> packet_queue;
            size_t packet_bytes_left = 0;

            while(m_playing)
            {
                buf = ilclient_get_input_buffer(m_video_decode, 130, 1);

                // no input-buffer -> quit
                if(!buf)
                {
                    LOG_ERROR << "could not get an input-buffer";
                    m_playing = false; break;
                }

                // feed data and wait until we get port settings changed
                uint8_t *buf_ptr = buf->pBuffer;
                bool buffer_filled = false;

                // fill up buffer
                while(!buffer_filled)
                {
                    // some packets left to feed into buffer?
                    if(packet_queue.empty())
                    {
                        // LOG_DEBUG << "av_read_frame";

                        int ret = av_read_frame(m_av_format_context, &current_packet);
                        if(ret < 0)
                        {
                            LOG_ERROR << "av_read_frame failed";

                            if(m_movie_ended_cb)
                            {
                                auto mc = m_movie_controller.lock();
                                if(mc){ m_movie_ended_cb(mc); }
                            }

                            if(m_loop)
                            {
                                AVRational timeBase =
                                    m_av_format_context->streams[m_av_video_stream_idx]->time_base;
                                int flags = (true) ? AVSEEK_FLAG_BACKWARD : 0;
                                int64_t seek_pos = (int64_t)(0.0 * AV_TIME_BASE);
                                int64_t seek_target = av_rescale_q(seek_pos, AV_TIME_BASE_Q, timeBase);
                                ret = av_seek_frame(m_av_format_context, m_av_video_stream_idx,
                                                    seek_target, flags);
                                avcodec_flush_buffers(m_av_codec_ctx);
                                first_packet = 1;
                                continue;
                            }
                            else{ m_playing = false; break; }
                        }
                        // else{ LOG_DEBUG << "av_read_frame succeeded";}

                        if(current_packet.size < 0){ av_free_packet(&current_packet); continue; }
                        packet_bytes_left = current_packet.size;
                    }
                    else
                    {
                        current_packet = packet_queue.front();
                        packet_queue.pop_front();
                    }

                    if(current_packet.stream_index == m_av_video_stream_idx)
                    {
                        // LOG_DEBUG << "video packet";

                        uint8_t *packet_data =
                        current_packet.buf ? current_packet.buf->data : current_packet.data;

                        packet_data += current_packet.size - packet_bytes_left;

                        size_t bytes_to_copy = std::min(packet_bytes_left, buf->nAllocLen - data_len);

                        data_len += bytes_to_copy;
                        memcpy(buf_ptr, packet_data, bytes_to_copy);
                        buf_ptr += bytes_to_copy;
                        packet_bytes_left -= bytes_to_copy;

                        if(data_len == buf->nAllocLen)
                        {
                            // LOG_DEBUG << "buffer filled";
                            buffer_filled = true;
                        }

                        if(packet_bytes_left)
                        {
                            packet_queue.push_back(current_packet);
                        }
                        else{ av_free_packet(&current_packet); }
                    }
                    else
                    {
                        // not a video packet -> release
                        // TODO: handle audio packets
                        av_free_packet(&current_packet);
                    }
                }

                if(port_settings_changed == 0 &&
                   ((data_len > 0 && ilclient_remove_event(m_video_decode,
                                                           OMX_EventPortSettingsChanged,
                                                           131, 0, 0, 1) == 0) ||
                   (data_len == 0 && ilclient_wait_for_event(m_video_decode,
                                                             OMX_EventPortSettingsChanged,
                                                             131, 0, 0, 1,
                                                             ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED,
                                                             10000) == 0)))
                {
                    port_settings_changed = 1;

                    if(ilclient_setup_tunnel(m_tunnels, 0, 0) != 0)
                    {
                       //status = -7;
                       LOG_ERROR << "setup tunnel failed";
                       break;
                    }

                    ilclient_change_component_state(m_video_scheduler, OMX_StateExecuting);

                    // now setup tunnel to egl_render
                    if(ilclient_setup_tunnel(m_tunnels + 1, 0, 1000) != 0)
                    {
                        //status = -12;
                        LOG_ERROR << "setup tunnel failed";
                        break;
                    }

                    // Set egl_render to idle
                    ilclient_change_component_state(m_egl_render, OMX_StateIdle);

                    // Enable the output port and tell egl_render to use the texture as a buffer
                    //ilclient_enable_port(egl_render, 221); THIS BLOCKS SO CANT BE USED
                    if(OMX_SendCommand(ILC_GET_HANDLE(m_egl_render), OMX_CommandPortEnable,
                                       221, NULL) != OMX_ErrorNone)
                    {
                        LOG_ERROR << "OMX_CommandPortEnable failed.";
                        m_playing = false;
                    }

                    if(OMX_UseEGLImage(ILC_GET_HANDLE(m_egl_render), &m_egl_buffer, 221, NULL, m_egl_image) != OMX_ErrorNone)
                    {
                        LOG_ERROR << "OMX_UseEGLImage failed.";
                        m_playing = false;
                    }

                    // Set egl_render to executing
                    ilclient_change_component_state(m_egl_render, OMX_StateExecuting);


                    // Request egl_render to write data to the texture buffer
                    if(OMX_FillThisBuffer(ILC_GET_HANDLE(m_egl_render), m_egl_buffer) != OMX_ErrorNone)
                    {
                        LOG_ERROR << "OMX_FillThisBuffer failed.";
                        m_playing = false;
                    }
                    m_has_new_frame = true;
                }
                if(!data_len)
                {
                    LOG_ERROR << "buffer underrun";
                    break;
                }
                buf->nFilledLen = data_len;
                data_len = 0;
                buf->nOffset = 0;

                if(first_packet)
                {
                    buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
                    first_packet = 0;
                }
                else{ buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN; }

                if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(m_video_decode), buf) != OMX_ErrorNone)
                {
                    LOG_ERROR << "OMX_EmptyThisBuffer failed.";
                    m_playing = false;
                }

                // playback time
                OMX_TIME_CONFIG_TIMESTAMPTYPE tstamp;
                tstamp.nPortIndex = OMX_ALL;
                if(OMX_GetConfig(ILC_GET_HANDLE(m_clock),
                                 OMX_IndexConfigTimeCurrentMediaTime,
                                 &tstamp) == OMX_ErrorNone)
                {
                    int64_t t = tstamp.nTimestamp.nHighPart;
                    t = (t << 32) | tstamp.nTimestamp.nLowPart;
                    m_current_time = t / (double)OMX_TICKS_PER_SECOND;
                }
            }// while(m_playing)

            if(buf)
            {
                buf->nFilledLen = 0;
                buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;

                if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(m_video_decode), buf) != OMX_ErrorNone)
                {
                    LOG_ERROR << "OMX_EmptyThisBuffer failed.";
                }
            }

            // need to flush the renderer to allow m_video_decode to disable its input port
            // LOG_DEBUG << "flush video_decode tunnel";
            ilclient_flush_tunnels(m_tunnels, 0);

            // LOG_DEBUG << "disable video_decode port buffers ... (skipped)";
            // ilclient_disable_port_buffers(m_video_decode, 130, NULL, NULL, NULL);
            // if(OMX_SendCommand(ILC_GET_HANDLE(m_video_decode),
            //                    OMX_CommandPortDisable, 130, NULL) != OMX_ErrorNone)
            // {
            //     LOG_ERROR << "disable video_decode port buffers failed";
            // }

            LOG_DEBUG << "movie decode thread ended: " << m_playing;
        }//thread func
    };

///////////////////////////////////////////////////////////////////////////////

    void fill_buffer_done_cb(void* data, COMPONENT_T* comp)
    {
        MovieControllerImpl *impl = static_cast<MovieControllerImpl*>(data);

        if(impl)
        {
            // OMX_BUFFERHEADERTYPE *write_buf = impl->m_playing ? impl->m_egl_buffer : nullptr;
            OMX_BUFFERHEADERTYPE *write_buf = impl->m_egl_buffer;

            if(OMX_FillThisBuffer(ilclient_get_handle(impl->m_egl_render),
                                  write_buf) != OMX_ErrorNone)
            {
              LOG_ERROR << "OMX_FillThisBuffer failed in callback";
            }
        }
    }

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

        ////////////////////////////////////////////////////////////////////////////

        // Open video file
        if(avformat_open_input(&m_impl->m_av_format_context, p.c_str(), nullptr, nullptr) != 0)
        {
            LOG_ERROR << "avformat_open_input failed";
            return;
        }

        // Retrieve stream information
        if(avformat_find_stream_info(m_impl->m_av_format_context, nullptr) < 0)
        {
            LOG_ERROR << "avformat_find_stream_info failed";
            return;
        }

        // Dump information about file onto standard error
        av_dump_format(m_impl->m_av_format_context, 0, p.c_str(), 0);

        // media duration
        m_impl->m_duration = m_impl->m_av_format_context->duration / (double) AV_TIME_BASE;

        // Find the first video stream
        m_impl->m_av_video_stream_idx = -1;

        for(uint32_t i = 0; i < m_impl->m_av_format_context->nb_streams; i++)
        {
            if(m_impl->m_av_format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                m_impl->m_av_video_stream_idx = i;
                break;
            }
        }

        if(m_impl->m_av_video_stream_idx == -1)
        {
            LOG_ERROR << "no video streams found";
            return;
        }

        // Get a pointer to the codec context for the video stream
        AVCodecContext* av_codec_ctx = nullptr;
        av_codec_ctx = m_impl->m_av_format_context->streams[m_impl->m_av_video_stream_idx]->codec;

        LOG_DEBUG << "movie: " << av_codec_ctx->width << " x " << av_codec_ctx->height;

        // allocate texture memory
        m_impl->m_texture = gl::Texture(av_codec_ctx->width, av_codec_ctx->height);

        AVCodec *av_codec = nullptr;

        // Find the decoder for the video stream
        av_codec = avcodec_find_decoder(av_codec_ctx->codec_id);

        if(!av_codec)
        {
            LOG_ERROR << "Unsupported codec!";
            return; // Codec not found
        }
        // m_impl->m_av_codec_ctx = av_codec_ctx;

        // Copy context
        m_impl->m_av_codec_ctx = avcodec_alloc_context3(av_codec);
        if(avcodec_copy_context(m_impl->m_av_codec_ctx, av_codec_ctx) != 0)
        {
            LOG_ERROR << "Couldn't copy codec context";
            return ; // Error copying codec context
        }
        // Open codec
        if(avcodec_open2(m_impl->m_av_codec_ctx, av_codec, nullptr) < 0)
        {
            LOG_ERROR << "Could not open codec";
            return; // Could not open codec
        }

        // avcodec_close(av_codec_ctx);

        ////////////////////////////////////////////////////////////////////////////

        m_impl->m_egl_image = eglCreateImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY),
                                                eglGetCurrentContext(),
                                                EGL_GL_TEXTURE_2D_KHR,
                                                (EGLClientBuffer)m_impl->m_texture.getId(),
                                                0);
        if(m_impl->m_egl_image == EGL_NO_IMAGE_KHR){ LOG_WARNING << "eglImage is null"; }

        int status = 0;

        if(!m_impl->m_il_client){ return; }

        // callback
        ilclient_set_fill_buffer_done_callback(m_impl->m_il_client, fill_buffer_done_cb, m_impl.get());

        // create video_decode
        if(ilclient_create_component(m_impl->m_il_client, &m_impl->m_video_decode, "video_decode",
                                     ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS) != 0)
        { status = -14; }
        m_impl->m_comp_list[0] = m_impl->m_video_decode;

        // create egl_render
        if(status == 0 &&
           ilclient_create_component(m_impl->m_il_client, &m_impl->m_egl_render, "egl_render",
                                     ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_OUTPUT_BUFFERS) != 0)
        { status = -14; }
        m_impl->m_comp_list[1] = m_impl->m_egl_render;

        // create clock
        if(status == 0 &&
           ilclient_create_component(m_impl->m_il_client, &m_impl->m_clock, "clock",
                                     ILCLIENT_DISABLE_ALL_PORTS) != 0)
        { status = -14; }
        m_impl->m_comp_list[2] = m_impl->m_clock;

        if(m_impl->m_clock &&
           OMX_SetParameter(ILC_GET_HANDLE(m_impl->m_clock), OMX_IndexConfigTimeClockState,
                            &m_impl->m_clock_state) != OMX_ErrorNone)
        { status = -13; }

        // create video_scheduler
        if(status == 0 &&
           ilclient_create_component(m_impl->m_il_client, &m_impl->m_video_scheduler,
                                     "video_scheduler", ILCLIENT_DISABLE_ALL_PORTS) != 0)
        { status = -14; }
        m_impl->m_comp_list[3] = m_impl->m_video_scheduler;

        set_tunnel(m_impl->m_tunnels, m_impl->m_video_decode, 131, m_impl->m_video_scheduler, 10);
        set_tunnel(m_impl->m_tunnels + 1, m_impl->m_video_scheduler, 11, m_impl->m_egl_render, 220);
        set_tunnel(m_impl->m_tunnels + 2, m_impl->m_clock, 80, m_impl->m_video_scheduler, 12);

        // setup clock tunnel first
        if(status == 0 && ilclient_setup_tunnel(m_impl->m_tunnels + 2, 0, 0) != 0)
        {
          status = -15;
        }
        else{ ilclient_change_component_state(m_impl->m_clock, OMX_StateExecuting); }

        if(status == 0){ ilclient_change_component_state(m_impl->m_video_decode, OMX_StateIdle); }

        if(status == 0 &&
           OMX_SetParameter(ILC_GET_HANDLE(m_impl->m_video_decode),
                            OMX_IndexParamVideoPortFormat,
                            &m_impl->m_port_format) == OMX_ErrorNone &&
           ilclient_enable_port_buffers(m_impl->m_video_decode, 130, NULL, NULL, NULL) == 0)
        {
            if(autoplay){ play(); }
            m_impl->m_loop = loop;
        }
        LOG_WARNING_IF(status) << "unsmooth movie init: " << status;
    }

    void MovieController::play()
    {
        LOG_DEBUG << "starting movie playback";

        if(!m_impl || m_impl->m_playing){ return; }

        pause();
        m_impl->m_playing = true;
        m_impl->m_thread = std::thread(std::bind(&MovieControllerImpl::thread_func, m_impl.get()));
        if(m_impl->m_on_load_cb){ m_impl->m_on_load_cb(shared_from_this()); }
        //[m_impl->m_player setRate: m_impl->m_rate];
    }

    void MovieController::unload()
    {
        m_impl.reset(new MovieControllerImpl);
    }

    void MovieController::pause()
    {
        if(!m_impl){ return; }
        m_impl->m_playing = false;
        try{ if(m_impl->m_thread.joinable()){ m_impl->m_thread.join(); } }
        catch(std::exception &e){ LOG_ERROR<<e.what(); }
    }

    bool MovieController::is_playing() const
    {
        return m_impl && m_impl->m_playing;
    }

    void MovieController::restart()
    {
        //[m_impl->m_player seekToTime:kCMTimeZero];
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
        return m_impl ? m_impl->m_duration : 0.0;
    }

    double MovieController::current_time() const
    {
        return m_impl ? m_impl->m_current_time : 0.0;
    }

    void MovieController::seek_to_time(float value)
    {
        //CMTime t = CMTimeMakeWithSeconds(value, NSEC_PER_SEC);
        //[m_impl->m_player seekToTime:t];
        //[m_impl->m_player setRate: m_impl->m_rate];
    }

    void MovieController::set_loop(bool b)
    {

    }

    bool MovieController::loop() const
    {
        return m_impl && m_impl->m_loop;
    }

    void MovieController::set_rate(float r)
    {
        if(!m_impl){ return; }

        m_impl->m_rate = r;
        //[m_impl->m_player setRate: r];
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
