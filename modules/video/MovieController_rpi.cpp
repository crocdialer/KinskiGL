// Raspian defines and includes
#define USE_VCHIQ_ARM
#define OMX_SKIP64BIT

#include "bcm_host.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "ilclient.h"
#undef countof

#include <thread>
#include "gl/Texture.hpp"
#include "gl/Buffer.hpp"
#include "MovieController.h"

namespace kinski{ namespace video{

    struct MovieControllerImpl
    {
        std::string m_src_path;
        bool m_playing;
        bool m_has_new_frame;
        bool m_loop;
        float m_rate;

        MovieController::MovieCallback m_on_load_cb, m_movie_ended_cb;

        OMX_BUFFERHEADERTYPE* m_egl_buffer;

        COMPONENT_T* m_egl_render = nullptr;
        COMPONENT_T* m_video_decode = nullptr;
        COMPONENT_T* m_video_scheduler = nullptr;
        COMPONENT_T* m_clock = nullptr;
        COMPONENT_T* m_comp_list[5];

        TUNNEL_T m_tunnels[4];
        ILCLIENT_T* m_il_client;

        OMX_VIDEO_PARAM_PORTFORMATTYPE m_port_format;
        OMX_TIME_CONFIG_CLOCKSTATETYPE m_clock_state;

        void* m_egl_image;
        gl::Texture m_texture;

        std::thread m_thread;

        MovieControllerImpl():
        m_src_path(""),
        m_playing(false),
        m_has_new_frame(false),
        m_loop(false),
        m_rate(1.f),
        m_egl_buffer(nullptr),
        m_egl_image(nullptr),
        m_texture(1280, 706)
        {
            // init the client
            m_il_client = ilclient_init();

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

        }
        ~MovieControllerImpl()
        {
            m_playing = false;

            try{ if(m_thread.joinable()){ m_thread.join(); } }
            catch(std::exception &e){ LOG_ERROR<<e.what(); }

            if(m_egl_image)
            {
                if(!eglDestroyImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY),
                                       (EGLImageKHR)m_egl_image))
                {
                    LOG_WARNING << "eglDestroyImageKHR failed.";
                }
            }
            ilclient_disable_tunnel(m_tunnels);
            ilclient_disable_tunnel(m_tunnels + 1);
            ilclient_disable_tunnel(m_tunnels + 2);
            ilclient_teardown_tunnels(m_tunnels);

            ilclient_state_transition(m_comp_list, OMX_StateIdle);
            ilclient_state_transition(m_comp_list, OMX_StateLoaded);
            ilclient_cleanup_components(m_comp_list);
            OMX_Deinit();
            ilclient_destroy(m_il_client);
        };

        void thread_func()
        {
            LOG_INFO << "starting thread";

            size_t data_len = 0;

            FILE *file_handle = fopen(m_src_path.c_str(), "rb");

            if(!file_handle)
            { return; }

            OMX_BUFFERHEADERTYPE *buf = nullptr;
            int port_settings_changed = 0;
            int first_packet = 1;

            ilclient_change_component_state(m_video_decode, OMX_StateExecuting);

            while(m_playing)
            {
                buf = ilclient_get_input_buffer(m_video_decode, 130, 1);

                if(!buf){ m_playing = false; }

                // feed data and wait until we get port settings changed
                unsigned char *dest = buf->pBuffer;

                // loop if at end
                if(feof(file_handle))
                {
                   if(m_loop){ rewind(file_handle); }
                   else{ break; }
                }

                data_len += fread(dest, 1, buf->nAllocLen-data_len, file_handle);

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
                    break;

                buf->nFilledLen = data_len;
                data_len = 0;

                buf->nOffset = 0;

                if(first_packet)
                {
                    buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
                    first_packet = 0;
                }
                else
                    buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;

                if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(m_video_decode), buf) != OMX_ErrorNone)
                {
                    //status = -6;
                    break;
                }
            }// while(m_playing)

            if(file_handle){ fclose(file_handle); }

            if(buf)
            {
              buf->nFilledLen = 0;
              buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;

              if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(m_video_decode), buf) != OMX_ErrorNone)
              {
                //status = -20;
              }
            }

            // need to flush the renderer to allow m_video_decode to disable its input port
            ilclient_flush_tunnels(m_tunnels, 0);

            ilclient_disable_port_buffers(m_video_decode, 130, NULL, NULL, NULL);
        }
    };

///////////////////////////////////////////////////////////////////////////////

    void fill_buffer_done_cb(void* data, COMPONENT_T* comp)
    {
        MovieControllerImpl *impl = static_cast<MovieControllerImpl*>(data);

        if(!impl || OMX_FillThisBuffer(ilclient_get_handle(impl->m_egl_render),
                                       impl->m_egl_buffer) != OMX_ErrorNone)
        {
           LOG_ERROR << "OMX_FillThisBuffer failed in callback";
        }
        LOG_INFO << "fill_buffer_done_cb";
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

    MovieController::MovieController():
    m_impl(new MovieControllerImpl)
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
        m_impl.reset(new MovieControllerImpl);
        LOG_INFO << "loading: " << filePath;

        try{ m_impl->m_src_path = search_file(filePath); }
        catch(FileNotFoundException &e)
        {
           LOG_WARNING << e.what();
           return;
        }

        m_impl->m_egl_image = eglCreateImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY),
                                                eglGetCurrentContext(),
                                                EGL_GL_TEXTURE_2D_KHR,
                                                (EGLClientBuffer)m_impl->m_texture.getId(),
                                                0);
        if(m_impl->m_egl_image == EGL_NO_IMAGE_KHR){ LOG_WARNING << "eglImage is null"; }

        int status = 0;

        if(!m_impl->m_il_client || OMX_Init() != OMX_ErrorNone){ return; }

        // callback
        ilclient_set_fill_buffer_done_callback(m_impl->m_il_client, fill_buffer_done_cb, this);

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

    }

    void MovieController::play()
    {
        LOG_INFO << "starting movie playback";

        if(m_impl->m_playing){ return; }

        pause();
        m_impl->m_playing = true;
        m_impl->m_thread = std::thread(std::bind(&MovieControllerImpl::thread_func, m_impl.get()));

        //[m_impl->m_player setRate: m_impl->m_rate];
    }

    void MovieController::unload()
    {
        m_impl.reset(new MovieControllerImpl);
    }

    void MovieController::pause()
    {
        m_impl->m_playing = false;
        try{ if(m_impl->m_thread.joinable()){ m_impl->m_thread.join(); } }
        catch(std::exception &e){ LOG_ERROR<<e.what(); }
    }

    bool MovieController::isPlaying() const
    {
        return m_impl->m_playing;
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
        if(!m_impl->m_has_new_frame){ return false; }
        tex = m_impl->m_texture;
        return true;
    }

    bool MovieController::copy_frames_offline(gl::Texture &tex, bool compress)
    {
        LOG_WARNING << "copy_frames_offline not available on RPI";
        return false;
    }

    double MovieController::duration() const
    {
        return 0.f;
    }

    double MovieController::current_time() const
    {
        return 0.f;
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
        return m_impl->m_loop;
    }

    void MovieController::set_rate(float r)
    {
        m_impl->m_rate = r;
        //[m_impl->m_player setRate: r];
    }

    const std::string& MovieController::get_path() const
    {
        return m_impl->m_src_path;
    }

    void MovieController::set_on_load_callback(MovieCallback c)
    {
        m_impl->m_on_load_cb = c;
    }

    void MovieController::set_movie_ended_callback(MovieCallback c)
    {
        m_impl->m_movie_ended_cb = c;
    }
}}// namespaces
