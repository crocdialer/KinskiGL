// Raspian includes
#define USE_VCHIQ_ARM
#include "bcm_host.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "ilclient.h"
#undef countof

#include "gl/Texture.h"
#include "gl/Buffer.h"
#include "MovieController.h"

namespace kinski{ namespace video{
    
    void fill_buffer_done_cb(void* data, COMPONENT_T* comp)
    {
        //if (OMX_FillThisBuffer(ilclient_get_handle(egl_render), eglBuffer) != OMX_ErrorNone)
        //{
        //    printf("OMX_FillThisBuffer failed in callback\n");
        //}
    }

    struct MovieControllerImpl
    {
        std::string m_src_path;
        bool m_playing;
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
        GLuint m_texture;

        MovieControllerImpl():
        m_src_path(""),
        m_playing(false),
        m_loop(false),
        m_rate(1.f),
        m_egl_buffer(nullptr),
        m_egl_image(nullptr),
        m_texture(0)
        {
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
            if(m_egl_image)
            {
                if(!eglDestroyImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY),
                                       (EGLImageKHR)m_egl_image))
                {
                    LOG_WARNING << "eglDestroyImageKHR failed.";
                }
            }
            ilclient_state_transition(m_comp_list, OMX_StateIdle);
            ilclient_state_transition(m_comp_list, OMX_StateLoaded);
            ilclient_cleanup_components(m_comp_list);
            OMX_Deinit();
            ilclient_destroy(m_il_client);
        };

        void thread_func()
        {
            
        }
    };
    
    MovieControllerPtr MovieController::create()
    {
        return MovieControllerPtr(new MovieController());
    }
    
    MovieControllerPtr MovieController::create(const std::string &filePath, bool autoplay,
                                               bool loop)
    {
        return MovieControllerPtr(new MovieController(filePath, autoplay, loop));
    }
    
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

    void MovieController::load(const std::string &filePath, bool autoplay, bool loop)
    {

        m_impl->m_egl_image = eglCreateImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY),
                                                eglGetCurrentContext(),
                                                EGL_GL_TEXTURE_2D_KHR,
                                                (EGLClientBuffer)m_impl->m_texture,
                                                0);
        if(!m_impl->m_egl_image){ LOG_WARNING << "eglImage is null"; }

        FILE *in;
        int status = 0;
        unsigned int data_len = 0;

        auto path = search_file(filePath);

        if((in = fopen(path.c_str(), "rb")) == NULL)
          return;
        
        // init the client
        m_impl->m_il_client = ilclient_init();

        if(!m_impl->m_il_client)
        {
            fclose(in);
            return;
        }

        if(OMX_Init() != OMX_ErrorNone)
        {
            ilclient_destroy(m_impl->m_il_client);
            fclose(in);
            return;
        }

        // callback
        ilclient_set_fill_buffer_done_callback(m_impl->m_il_client, fill_buffer_done_cb, 0);

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

        
    }

    void MovieController::play()
    {
        LOG_TRACE << "starting movie playback";
        //[m_impl->m_player play];
        //[m_impl->m_player setRate: m_impl->m_rate];
        m_impl->m_playing = true;
    }
    
    void MovieController::unload()
    {
        m_impl.reset(new MovieControllerImpl);
        m_impl->m_playing = false;
    }
    
    void MovieController::pause()
    {
        //[m_impl->m_player pause];
        //[m_impl->m_player setRate: 0.f];
        m_impl->m_playing = false;
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
        //m_impl->m_player.volume = val;
    }
    
    bool MovieController::copy_frame(std::vector<uint8_t>& data, int *width, int *height)
    {
        return false;
    }
    
    bool MovieController::copy_frame_to_texture(gl::Texture &tex, bool as_texture2D)
    {
      return false;
    }
    
    bool MovieController::copy_frames_offline(gl::Texture &tex, bool compress)
    {
        LOG_WARNING << "implementation not available on RPI";
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
