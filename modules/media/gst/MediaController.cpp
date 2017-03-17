#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>


//#if GST_CHECK_VERSION(1, 4, 5)
#include <gst/gl/gstglconfig.h>

#if defined(KINSKI_GLES)
#undef GST_GL_HAVE_OPENGL
//#undef GST_GL_HAVE_GLES2
#undef GST_GL_HAVE_PLATFORM_GLX
#else // Desktop
#undef GST_GL_HAVE_GLES2
#undef GST_GL_HAVE_PLATFORM_EGL
#undef GST_GL_HAVE_GLEGLIMAGEOES
#endif

#define GST_USE_UNSTABLE_API
#include <gst/gl/gstglcontext.h>
#include <gst/gl/gstgldisplay.h>
#include <gst/gl/x11/gstgldisplay_x11.h>
//#include <gst/gl/egl/gstgldisplay_egl.h>

#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#include "GLFW/glfw3native.h"

#include "core/file_functions.hpp"
#include "gl/Texture.hpp"
#include "MediaController.hpp"


namespace kinski{ namespace media
{

namespace
{
    std::weak_ptr<GstGLDisplay> g_gst_gl_display;
};

struct MediaControllerImpl
{
    std::string m_src_path;
    std::atomic<float> m_rate;
    std::atomic<float> m_volume;
    std::atomic<float> m_fps;
    std::atomic<bool> m_loaded;
    std::atomic<bool> m_has_video;
    std::atomic<bool> m_has_audio;
    std::atomic<bool> m_has_subtitle;
    std::atomic<bool> m_pause;
    std::atomic<bool> m_loop;
    std::atomic<bool> m_playing;
    std::atomic<bool> m_has_new_frame;

    // Memory map that holds the incoming frame.
    GstMapInfo m_memory_map_info;
    GstVideoInfo m_video_info;
    GstElement* m_pipeline = nullptr;
    GstElement* m_app_sink = nullptr;
    GstElement* m_video_bin = nullptr;
    GstGLContext* m_gl_context = nullptr;

    GstElement* m_gl_upload = nullptr;
    GstElement* m_gl_color_convert = nullptr;
    GstElement* m_raw_caps_filter = nullptr;

    // needed for message activation since we are not using signals.
    GMainLoop* m_g_main_loop = nullptr;

    // delivers the messages
    GstBus* m_gst_bus = nullptr;

    // Save the id of the bus for releasing when not needed.
    int m_bus_id;

    std::shared_ptr<GstGLDisplay> m_gst_gl_display;

    // runs GMainLoop.
    std::thread m_thread;

    // protect appsink callbacks
    std::mutex m_mutex;

    MediaController::MediaCallback m_on_load_cb, m_movie_ended_cb;

    MediaController::RenderTarget m_render_target = MediaController::RenderTarget::TEXTURE;
    MediaController::AudioTarget m_audio_target = MediaController::AudioTarget::AUTO;

    MediaControllerImpl():
    m_rate(1.f),
    m_volume(1.f),
    m_fps(0.f),
    m_loaded(false),
    m_has_video(false),
    m_has_audio(false),
    m_has_subtitle(false),
    m_pause(false),
    m_loop(false),
    m_playing(false),
    m_has_new_frame(false)
    {
        auto success = init_gstreamer();

        if(success)
        {
            m_g_main_loop = g_main_loop_new(nullptr, false);
            m_thread = std::thread([this](){ g_main_loop_run(m_g_main_loop); });

            auto display = g_gst_gl_display.lock();

            if(display){ m_gst_gl_display = display; }
            else
            {
                m_gst_gl_display =
                std::shared_ptr<GstGLDisplay>((GstGLDisplay*)gst_gl_display_x11_new_with_display(glfwGetX11Display()),
                                              &gst_object_unref);
                g_gst_gl_display = m_gst_gl_display;
            }
        }
    }

    ~MediaControllerImpl()
    {
        m_playing = false;

//        if(mGstData.pipeline != nullptr)
//        {
//
//            resetPipeline();
//            // Reset the bus since the associated pipeline got resetted.
//            resetBus();
//        }

//        resetVideoBuffers();

        if(g_main_loop_is_running(m_g_main_loop))
        {
            g_main_loop_quit(m_g_main_loop);
        }
        g_main_loop_unref(m_g_main_loop);

        if(m_thread.joinable())
        {
            try{ m_thread.join(); }
            catch(std::exception &e){ LOG_ERROR << e.what(); }
        }
    };

    bool init_gstreamer()
    {
        if(!gst_is_initialized())
        {
            guint major;
            guint minor;
            guint micro;
            guint nano;
            gst_version(&major, &minor, &micro, &nano);
            GError* err;

            if(!gst_init_check(nullptr, nullptr, &err))
            {
                if(err->message){ LOG_ERROR << "FAILED to initialize GStreamer: " << err->message; }
                else{ LOG_ERROR << "FAILED to initialize GStreamer due to unknown error ..."; }
                return false;
            }
            else
            {
                char buf[128];
                sprintf(buf, "initialized GStreamer version %i.%i.%i.%i", major, minor, micro, nano);
                LOG_INFO << buf;
                return true;
            }
        }
        return true;
    }

    void reset_pipeline()
    {
        if(!m_pipeline ){ return; }

        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        gst_element_get_state(m_pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
        gst_object_unref(GST_OBJECT(m_pipeline));
        m_pipeline = nullptr;
        m_video_bin = nullptr;
        gst_object_unref(m_gl_context);
        m_gl_context = nullptr;

//        gst_object_unref(sGstGLDisplay);

        // Pipeline will unref and destroy its children..
        m_gl_upload = nullptr;
        m_gl_color_convert = nullptr;
    }

    static void on_gst_eos(GstAppSink* sink, gpointer userData);
    static GstFlowReturn on_gst_sample(GstAppSink* sink, gpointer userData);
    static GstFlowReturn on_gst_preroll(GstAppSink* sink, gpointer userData);
};

///////////////////////////////////////////////////////////////////////////////

void MediaControllerImpl::on_gst_eos(GstAppSink *sink, gpointer userData)
{

}

GstFlowReturn MediaControllerImpl::on_gst_sample(GstAppSink *sink, gpointer userData)
{
    return GST_FLOW_CUSTOM_ERROR;
}

GstFlowReturn MediaControllerImpl::on_gst_preroll(GstAppSink *sink, gpointer userData)
{
    return GST_FLOW_CUSTOM_ERROR;
}

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
    m_impl->m_on_load_cb = on_load;
    m_impl->m_movie_ended_cb = on_end;
    m_impl->m_render_target = the_render_target;
    m_impl->m_audio_target = the_audio_target;

    m_impl->m_loaded = true;

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
    if(is_loaded()){ return m_impl->m_rate; }
    else{ return 1.f; }
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
