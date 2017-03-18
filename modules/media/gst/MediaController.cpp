#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>


#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>

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
    const int g_enable_async_state_change = false;
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
    std::atomic<bool> m_prerolled;
    std::atomic<bool> m_live;
    std::atomic<bool> m_buffering;
    std::atomic<bool> m_seeking;
    std::atomic<bool> m_seek_requested;
    std::atomic<bool> m_stream;
    std::atomic<bool> m_done;
    std::atomic<bool> m_pause;
    std::atomic<bool> m_loop;
    std::atomic<bool> m_playing;
    std::atomic<bool> m_has_new_frame;
    std::atomic<bool> m_video_has_changed;

    // memory map that holds the incoming frame.
    GstMapInfo m_memory_map_info;
    GstVideoInfo m_video_info;
    GstElement* m_pipeline = nullptr;
    GstElement* m_app_sink = nullptr;
    GstElement* m_video_bin = nullptr;
    GstGLContext* m_gl_context = nullptr;

    GstElement* m_gl_upload = nullptr;
    GstElement* m_gl_color_convert = nullptr;
    GstElement* m_raw_caps_filter = nullptr;

    std::atomic<GstState> m_current_state, m_target_state;

    std::shared_ptr<GstBuffer> m_current_buffer, m_new_buffer;

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

    MediaController::callback_t m_on_load_cb, m_movie_ended_cb;

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
    m_prerolled(false),
    m_live(false),
    m_buffering(false),
    m_seeking(false),
    m_seek_requested(false),
    m_stream(false),
    m_pause(false),
    m_loop(false),
    m_playing(false),
    m_has_new_frame(false),
    m_video_has_changed(true),
    m_current_state(GST_STATE_NULL),
    m_target_state(GST_STATE_NULL)
    {
        auto success = init_gstreamer();

        if(success)
        {
            m_g_main_loop = g_main_loop_new(nullptr, false);
            m_thread = std::thread(&g_main_loop_run, m_g_main_loop);
            m_gst_gl_display = g_gst_gl_display.lock();
            if(!m_gst_gl_display)
            {
                m_gst_gl_display =
                std::shared_ptr<GstGLDisplay>((GstGLDisplay*)gst_gl_display_x11_new_with_display(glfwGetX11Display()),
                                              &gst_object_unref);
                g_gst_gl_display = m_gst_gl_display;
            }
            m_gl_context = gst_gl_context_new_wrapped(m_gst_gl_display.get(),
                                                      (guintptr)::glfwGetGLXContext(glfwGetCurrentContext()),
                                                      GST_GL_PLATFORM_GLX, GST_GL_API_OPENGL);
        }
    }

    ~MediaControllerImpl()
    {
        m_playing = false;

        if(m_pipeline)
        {
            reset_pipeline();

            // Reset the bus since the associated pipeline got resetted.
            reset_bus();
        }

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
            guint major, minor, micro, nano;
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

        // Pipeline will unref and destroy its children..
        m_gl_upload = nullptr;
        m_gl_color_convert = nullptr;
    }

    void construct_pipeline()
    {
        if(m_pipeline) return;

        m_pipeline = gst_element_factory_make("playbin", "playbinsink");

        if(!m_pipeline)
        {
            LOG_ERROR << "failed to create playbin pipeline";
            return;
        }
        m_video_bin = gst_bin_new("kinski-vbin");
        if(!m_video_bin){ LOG_ERROR << "Failed to create video bin"; }

        m_app_sink = gst_element_factory_make("appsink", "videosink");

        if(!m_app_sink){ LOG_ERROR << "failed to create app sink element"; }
        else
        {
            gst_app_sink_set_max_buffers(GST_APP_SINK(m_app_sink), 1);
            gst_app_sink_set_drop(GST_APP_SINK(m_app_sink), true);
            gst_base_sink_set_qos_enabled(GST_BASE_SINK(m_app_sink), true);
            gst_base_sink_set_sync(GST_BASE_SINK(m_app_sink), true);
            gst_base_sink_set_max_lateness(GST_BASE_SINK(m_app_sink), 20 * GST_MSECOND);

            GstAppSinkCallbacks app_sink_callbacks;
            app_sink_callbacks.eos = on_gst_eos;
            app_sink_callbacks.new_preroll = on_gst_preroll;
            app_sink_callbacks.new_sample = on_gst_sample;

            std::string caps_descr = "video/x-raw(memory:GLMemory), format=RGBA";
            //!sUseGstGl
            if(false){ caps_descr = "video/x-raw, format=RGBA"; }

            gst_app_sink_set_callbacks(GST_APP_SINK(m_app_sink), &app_sink_callbacks, this, nullptr);
            GstCaps* caps = gst_caps_from_string(caps_descr.c_str());
            gst_app_sink_set_caps(GST_APP_SINK(m_app_sink), caps);
            gst_caps_unref(caps);
        }

        GstPad *pad = nullptr;

        //sUseGstGl
        if(true)
        {
            m_gl_upload = gst_element_factory_make("glupload", "upload");
            if(!m_gl_upload){ LOG_ERROR << "failed to create GL upload element"; };

            m_gl_color_convert = gst_element_factory_make("glcolorconvert", "convert");
            if(!m_gl_color_convert){ LOG_ERROR << "failed to create GL convert element"; }

            m_raw_caps_filter = gst_element_factory_make("capsfilter", "rawcapsfilter");

#if defined( CINDER_LINUX_EGL_ONLY ) && defined( CINDER_GST_HAS_GL )
        if( mGstData.rawCapsFilter ) g_object_set( G_OBJECT( mGstData.rawCapsFilter ), "caps", gst_caps_from_string( "video/x-raw(memory:GLMemory)" ), nullptr );
#else
            if(m_raw_caps_filter)
            {
                g_object_set(G_OBJECT(m_raw_caps_filter), "caps", gst_caps_from_string( "video/x-raw" ), nullptr );
            }
#endif
            else{ LOG_ERROR << "failed to create raw caps filter element"; }

            gst_bin_add_many(GST_BIN(m_video_bin), m_raw_caps_filter, m_gl_upload, m_gl_color_convert, m_app_sink,
                             nullptr);

            if(!gst_element_link_many(m_raw_caps_filter, m_gl_upload, m_gl_color_convert, m_app_sink, nullptr))
            {
                LOG_ERROR << "failed to link video elements";
            }
            pad = gst_element_get_static_pad(m_raw_caps_filter, "sink");
            gst_element_add_pad(m_video_bin, gst_ghost_pad_new("sink", pad));
        }
        else
        {
            gst_bin_add(GST_BIN(m_video_bin), m_app_sink);
            pad = gst_element_get_static_pad(m_app_sink, "sink");
            gst_element_add_pad(m_video_bin, gst_ghost_pad_new("sink", pad));
        }

        if(pad)
        {
            gst_object_unref(pad);
            pad = nullptr;
        }
        g_object_set(G_OBJECT(m_pipeline), "video-sink", m_video_bin, nullptr);
        add_bus_watch(m_pipeline);
    }

    void reset_bus()
    {
        if(g_source_remove(m_bus_id)){ m_bus_id = -1; }
        m_gst_bus = nullptr;
    }

    void update_state(GstState the_state)
    {
        m_current_state = the_state;

        switch(m_current_state)
        {
            case GST_STATE_NULL:
                break;

            case GST_STATE_READY:
//                prepareForNewVideo();
                break;

            case GST_STATE_PAUSED:
                m_prerolled = true;
                m_pause = true;
                m_loaded = true;
//                isPlayable = true;
                break;

            case GST_STATE_PLAYING:
                m_done = false;
                m_pause = false;
//                isPlayable      = true;
                break;

            default: break;
        }
    }

    bool set_pipeline_state(GstState the_target_state)
    {
        if(!m_pipeline){ return false; }

        char buf[128];
        GstState current, pending;
        gst_element_get_state(m_pipeline, &current, &pending, 0);
        m_target_state = the_target_state;

        // unnecessary state change
        if(current == the_target_state || pending == the_target_state)
            return true;

        GstStateChangeReturn state_change_result = gst_element_set_state( m_pipeline, m_target_state);
        LOG_DEBUG <<"pipeline state about to change from: " << gst_element_state_get_name(current) <<
                    " to " << gst_element_state_get_name(the_target_state);

        if(!g_enable_async_state_change && state_change_result == GST_STATE_CHANGE_ASYNC)
        {
            sprintf(buf, "blocking until pipeline state changes from: %s to %s",
                    gst_element_state_get_name(current),
                    gst_element_state_get_name(the_target_state));
            LOG_DEBUG << buf;
            state_change_result = gst_element_get_state(m_pipeline, &current, &pending, GST_CLOCK_TIME_NONE);
        }

        switch(state_change_result)
        {
            case GST_STATE_CHANGE_FAILURE:
                LOG_WARNING << "pipeline failed to change state";
                return false;

            case GST_STATE_CHANGE_SUCCESS:
                sprintf(buf, "pipeline state changed SUCCESSFULLY from: %s to %s",
                        gst_element_state_get_name(m_current_state),
                        gst_element_state_get_name(m_target_state));

                LOG_DEBUG << buf;
                // target state reached
                update_state(m_target_state);
                return true;

            case GST_STATE_CHANGE_ASYNC:
                sprintf(buf, "pipeline state change will happen ASYNC from: %s to %s",
                        gst_element_state_get_name(m_current_state),
                        gst_element_state_get_name(m_target_state));
                LOG_DEBUG << buf;
                return true;

            case GST_STATE_CHANGE_NO_PREROLL:
                m_live = true;
                return true;

            default:
                return false;
        }
    }

    void add_bus_watch(GstElement* the_pipeline)
    {
        m_gst_bus = gst_pipeline_get_bus(GST_PIPELINE(the_pipeline));
        m_bus_id = gst_bus_add_watch(m_gst_bus, check_bus_messages_async, this);
        gst_bus_set_sync_handler(m_gst_bus, check_bus_messages_sync, this, nullptr);
        gst_object_unref(m_gst_bus);
    }

    void process_sample(GstSample* sample)
    {
        m_prerolled = true;

        // pull the memory buffer from sample.
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            m_new_buffer = std::shared_ptr<GstBuffer>(gst_buffer_ref(gst_sample_get_buffer(sample)), &gst_buffer_unref);
        }

        if(m_video_has_changed)
        {
            // Grab video info.
            GstCaps* currentCaps = gst_sample_get_caps(sample);
            if(gst_video_info_from_caps(&m_video_info, currentCaps))
            {
                m_fps = (float)m_video_info.fps_n / (float)m_video_info.fps_d;

//                mGstData.width              = videoInfo.width;
//                mGstData.height             = videoInfo.height;
//                mGstData.videoFormat        = videoInfo.finfo->format;
//                mGstData.frameRate          = (float)videoInfo.fps_n / (float)videoInfo.fps_d;
//                mGstData.pixelAspectRatio   = (float)videoInfo.par_n / (float)videoInfo.par_d ;
//                mGstData.fpsNom             = videoInfo.fps_n;
//                mGstData.fpsDenom           = videoInfo.fps_d;
            }
            /// reset the new video flag
            m_video_has_changed = false;
        }

        gst_sample_unref(sample);
        m_has_new_frame = true;


//        {
//            std::lock_guard<std::mutex> guard(m_mutex);
//            GstBuffer* new_buffer = gst_sample_get_buffer(sample);
//
//            // map the buffer for reading
//            gst_buffer_map(new_buffer, &m_memory_map_info, GST_MAP_READ);
//
//            // We have pre-rolled so query info and allocate buffers if we have a new video.
//            if(m_video_has_changed)
//            {
//                GstCaps* currentCaps = gst_sample_get_caps(sample);
//
//                if(gst_video_info_from_caps(&m_video_info, currentCaps))
//                {
//                    //
//                }
//                auto deleter = []( unsigned char* p ) {
//                    delete [] p;
//                };
//                if(!mFrontVBuffer)
//                {
//                    mFrontVBuffer = std::shared_ptr<unsigned char>(new unsigned char[m_memory_map_info.size], deleter);
//                }
//
//                if(!mBackVBuffer)
//                {
//                    mBackVBuffer = std::shared_ptr<unsigned char>(new unsigned char[m_memory_map_info.size], deleter);
//                }
//
//                /// reset the new video flag
//                m_video_has_changed = false;
//            }
//            memcpy( mBackVBuffer.get(), m_memory_map_info.data, m_memory_map_info.size);
//            gst_buffer_unmap(new_buffer, &m_memory_map_info);
//            mNewFrame = true;
//
//            // Finished working with the sample - unref-it.
//            gst_sample_unref(sample);
//            sample = nullptr;
//        }
    }

    static GstBusSyncReply check_bus_messages_sync(GstBus* bus, GstMessage* message, gpointer userData);
    static gboolean check_bus_messages_async(GstBus* bus, GstMessage* message, gpointer userData);
    static void on_gst_eos(GstAppSink* sink, gpointer userData);
    static GstFlowReturn on_gst_sample(GstAppSink* sink, gpointer userData);
    static GstFlowReturn on_gst_preroll(GstAppSink* sink, gpointer userData);
};

///////////////////////////////////////////////////////////////////////////////

GstBusSyncReply MediaControllerImpl::check_bus_messages_sync(GstBus* bus, GstMessage* message, gpointer userData)
{
    if(!userData ){ return GST_BUS_DROP; }

    MediaControllerImpl* self = static_cast<MediaControllerImpl*>(userData);

    switch(GST_MESSAGE_TYPE(message))
    {
        case GST_MESSAGE_NEED_CONTEXT:
        {
            const gchar *context_type = nullptr;
            GstContext* context = nullptr;
            gst_message_parse_context_type(message, &context_type);

            LOG_DEBUG << "need context " << context_type << " from element " << GST_ELEMENT_NAME(GST_MESSAGE_SRC(message));

            if(g_strcmp0(context_type, GST_GL_DISPLAY_CONTEXT_TYPE) == 0)
            {
                context = gst_context_new(GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
                gst_context_set_gl_display(context, self->m_gst_gl_display.get());
                gst_element_set_context(GST_ELEMENT(message->src), context);
            }
            else if(g_strcmp0(context_type, "gst.gl.app_context") == 0)
            {
                context = gst_context_new("gst.gl.app_context", TRUE);
                GstStructure *s = gst_context_writable_structure(context);
                gst_structure_set(s, "context", GST_GL_TYPE_CONTEXT, self->m_gl_context, nullptr);
                gst_element_set_context(GST_ELEMENT(message->src), context);
            }

            if(context){ gst_context_unref(context); }
            break;
        }
        default: break;
    }
    return GST_BUS_PASS;
}

gboolean MediaControllerImpl::check_bus_messages_async(GstBus* bus, GstMessage* message, gpointer userData)
{
    if(!userData){ return true; }

    MediaControllerImpl* self = static_cast<MediaControllerImpl*>(userData);

    switch(GST_MESSAGE_TYPE(message))
    {
        case GST_MESSAGE_ERROR:
        {
            GError *err = nullptr;
            gchar *dbg;
            gst_message_parse_error(message, &err, &dbg);
            gst_object_default_error(message->src, err, dbg);
            g_error_free(err);
            g_free(dbg);

            GstStateChangeReturn state = gst_element_set_state(self->m_pipeline, GST_STATE_NULL);

            if(state == GST_STATE_CHANGE_FAILURE)
            {
                LOG_ERROR << "failed to set the pipeline to nullptr state after ERROR occured";
            }
            break;
        }

        case GST_MESSAGE_HAVE_CONTEXT:
        {
            GstContext *context = nullptr;
            const gchar *context_type = nullptr;
            gchar *context_str = nullptr;

            gst_message_parse_have_context(message, &context);
            context_type = gst_context_get_context_type(context);
            context_str = gst_structure_to_string(gst_context_get_structure(context));
            LOG_DEBUG << "have context " << context_type << " from element " << GST_ELEMENT_NAME(GST_MESSAGE_SRC(message));
            g_free(context_str);

            if(context){ gst_context_unref(context); }
            break;
        }

        case GST_MESSAGE_BUFFERING:
        {
            // no buffering for live sources.
            if(self->m_live) break;
            gint percent = 0;
            gst_message_parse_buffering(message, &percent);
//                LOG_DEBUG << "buffering " << std::setfill('0') << std::setw(3) << percent << " %";

            if(percent == 100)
            {
                self->m_buffering = false;
                LOG_DEBUG << "buffering complete!";

                if(self->m_target_state == GST_STATE_PLAYING)
                {
                    gst_element_set_state(self->m_pipeline, GST_STATE_PLAYING);
                }
            }
            else
            {
                if(!self->m_buffering && self->m_target_state == GST_STATE_PLAYING)
                {
                    gst_element_set_state(self->m_pipeline, GST_STATE_PAUSED);
                    LOG_DEBUG << "buffering in process ...";
                }
                self->m_buffering = true;
            }
            break;
        }

            // possibly due to network connection error while streaming
        case GST_MESSAGE_CLOCK_LOST:
        {
            // Get a new clock
            gst_element_set_state(self->m_pipeline, GST_STATE_PAUSED);
            gst_element_set_state(self->m_pipeline, GST_STATE_PLAYING);
            break;
        }

        case GST_MESSAGE_DURATION_CHANGED:
            break;

        case GST_MESSAGE_STATE_CHANGED:
        {
            if(GST_MESSAGE_SRC(message) == GST_OBJECT(self->m_pipeline))
            {
                GstState old, current, pending;
                gst_message_parse_state_changed(message, &old, &current, &pending);

                if(old != current)
                {
                    char buf[256];
                    sprintf(buf, "pipeline state changed from: %s to %s with pending %s",
                            gst_element_state_get_name(old), gst_element_state_get_name(current),
                            gst_element_state_get_name(pending));
                    LOG_DEBUG << buf;
                }

                self->update_state(current);

                if(self->m_target_state != self->m_current_state && pending == GST_STATE_VOID_PENDING)
                {
                    //TODO: missing behaviour
                    if(self->m_target_state == GST_STATE_PAUSED){ self->set_pipeline_state(GST_STATE_PAUSED); }
                    else if(self->m_target_state == GST_STATE_PLAYING){ self->set_pipeline_state(GST_STATE_PLAYING); }
                }
            }
            break;
        }
        case GST_MESSAGE_ASYNC_DONE:
        {
            switch(self->m_current_state)
            {
                case GST_STATE_PAUSED:
                {
                    if(self->m_seeking)
                    {
                        if(self->m_seek_requested)
                        {
                            //TODO: missing behaviour
//                                data.position = data.requestedSeekTime * GST_SECOND;
//                                if( data.player ) data.player->seekToTime( data.requestedSeekTime, true );
                            self->m_seek_requested = false;
                        }
                        else{ self->m_seeking = false; }
                    }
                }
                default: break;
            }
            break;
        }

        case GST_MESSAGE_EOS:
        {
            if(self->m_loop)
            {
                if( /*data.palindrome &&*/false && !self->m_stream && !self->m_live)
                {
                    // Toggle the direction we are playing
                    //TODO: missing behaviour
//                        if( data.player ) data.player->setRate( -data.player->getRate() );
                }
                else
                {
                    // If playing back on reverse start the loop from the
                    // end of the file
                    if(self->m_rate < 0)
                    {
                        //TODO: missing behaviour
//                            if( data.player ) data.player->seekToTime( data.player->getDurationSeconds() );
                    }
                    else
                    {
                        //TODO: missing behaviour
                        // otherwise restart from beginning.
//                            if( data.player ) data.player->seekToTime( 0 );
                    }
                }
            }
//                data.videoHasChanged = false;
            self->m_done = true;
            break;
        }

        default: break;
    }

    return true;
}

void MediaControllerImpl::on_gst_eos(GstAppSink *sink, gpointer userData)
{

}

GstFlowReturn MediaControllerImpl::on_gst_sample(GstAppSink *sink, gpointer userData)
{
    MediaControllerImpl *self = static_cast<MediaControllerImpl*>(userData);
    self->process_sample(gst_app_sink_pull_sample(sink));
    return GST_FLOW_OK;
}

GstFlowReturn MediaControllerImpl::on_gst_preroll(GstAppSink *sink, gpointer userData)
{
    MediaControllerImpl *self = static_cast<MediaControllerImpl*>(userData);
    self->process_sample(gst_app_sink_pull_preroll(sink));
    return GST_FLOW_OK;
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

MediaController::MediaController()
{

}

MediaController::~MediaController(){}

///////////////////////////////////////////////////////////////////////////////

void MediaController::load(const std::string &filePath, bool autoplay, bool loop,
                           RenderTarget the_render_target, AudioTarget the_audio_target)
{
    std::string found_path;
    if(fs::is_uri(filePath)){ found_path = filePath; }
    else
    {
        try{ found_path = fs::search_file(filePath); }
        catch(fs::FileNotFoundException &e)
        {
            LOG_ERROR << e.what();
            return;
        }
    }
    callback_t on_load = m_impl ? m_impl->m_on_load_cb : callback_t();
    callback_t on_end = m_impl ? m_impl->m_movie_ended_cb : callback_t();
    m_impl.reset(new MediaControllerImpl());
    m_impl->m_src_path = found_path;
    m_impl->m_on_load_cb = on_load;
    m_impl->m_movie_ended_cb = on_end;
    m_impl->m_render_target = the_render_target;
    m_impl->m_audio_target = the_audio_target;

    // construct a pipeline
    m_impl->construct_pipeline();
    m_impl->set_pipeline_state(GST_STATE_READY);

    std::string uri_path = filePath;

    if(fs::is_uri(filePath)){ m_impl->m_stream = true; }
    else{ uri_path = fs::path_as_uri(found_path); }

    // set the new path
    g_object_set(G_OBJECT(m_impl->m_pipeline), "uri", uri_path.c_str(), nullptr);

    // preroll
    m_impl->set_pipeline_state(GST_STATE_PAUSED);

    m_impl->m_loaded = true;

    // fire onload callback
    if(m_impl->m_on_load_cb){ m_impl->m_on_load_cb(shared_from_this()); };

    // autoplay
    if(autoplay){ play(); }
}

/////////////////////////////////////////////////////////////////

void MediaController::play()
{
    m_impl->set_pipeline_state(GST_STATE_PLAYING);
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
    if(m_impl){ m_impl->set_pipeline_state(GST_STATE_PAUSED); }
}

/////////////////////////////////////////////////////////////////

bool MediaController::is_playing() const
{
    return false;
}

/////////////////////////////////////////////////////////////////

void MediaController::restart()
{
    play();
    seek_to_time(0.0);
}

/////////////////////////////////////////////////////////////////

float MediaController::volume() const
{
    if(m_impl && m_impl->m_pipeline)
    {
        float ret = 0.f;
        g_object_get(G_OBJECT(m_impl->m_pipeline), "volume", &ret, nullptr);
        return ret;
    }
    return 0.f;
}

/////////////////////////////////////////////////////////////////

void MediaController::set_volume(float newVolume)
{
    if(m_impl && m_impl->m_pipeline)
    {
        g_object_set(G_OBJECT(m_impl->m_pipeline), "volume", (gdouble)newVolume, nullptr);
    }
}

/////////////////////////////////////////////////////////////////

bool MediaController::copy_frame(std::vector<uint8_t>& data, int *width, int *height)
{
    return false;
}

/////////////////////////////////////////////////////////////////

bool MediaController::copy_frame_to_texture(gl::Texture &tex, bool as_texture2D)
{
    if(m_impl && m_impl->m_has_new_frame)
    {
        GstMemory *mem = gst_buffer_peek_memory(m_impl->m_new_buffer.get(), 0);

        if(gst_is_gl_memory(mem))
        {
            std::lock_guard<std::mutex> guard(m_impl->m_mutex);
            GstGLMemory *gl_mem = (GstGLMemory*)(mem);

            GLint id = gl_mem->tex_id;
            GLint target = GL_TEXTURE_2D;
#if !defined(KINSKI_GLES)
            if(gl_mem->tex_target == GST_GL_TEXTURE_TARGET_RECTANGLE){ target = GL_TEXTURE_RECTANGLE; }
#endif
            tex = gl::Texture(target, id, m_impl->m_video_info.width, m_impl->m_video_info.height, true);
            tex.set_flipped(true);
            std::swap(m_impl->m_new_buffer, m_impl->m_current_buffer);
        }
        m_impl->m_has_new_frame = false;
        return true;
    }
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
    if(m_impl && m_impl->m_prerolled)
    {
        gint64 duration = 0;
        gst_element_query_duration(m_impl->m_pipeline, GST_FORMAT_TIME, &duration);
        return duration / 1000000000.0;
    }
    return 0.0;
}

/////////////////////////////////////////////////////////////////

double MediaController::current_time() const
{
    if(m_impl && m_impl->m_prerolled)
    {
        gint64 position = 0;
        gst_element_query_position(m_impl->m_pipeline, GST_FORMAT_TIME, &position);
        return position / 1000000000.0;
    }
    return 0.0;
}

/////////////////////////////////////////////////////////////////

double MediaController::fps() const
{
    if(m_impl && m_impl->m_prerolled){ return m_impl->m_fps; }
    return 0.0;
}

/////////////////////////////////////////////////////////////////

void MediaController::seek_to_time(double value)
{
    GstEvent* seek_event;
    GstSeekFlags seek_flags = GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE);
    m_impl->m_seeking = true;
    m_impl->m_done = false;
    gint64 seek_time = value * GST_SECOND;

    if(m_impl->m_rate > 0.0)
    {
        seek_event = gst_event_new_seek(m_impl->m_rate, GST_FORMAT_TIME, seek_flags, GST_SEEK_TYPE_SET, seek_time,
                                        GST_SEEK_TYPE_SET, GST_CLOCK_TIME_NONE);
    }
    else
    {
        seek_event = gst_event_new_seek(m_impl->m_rate, GST_FORMAT_TIME, seek_flags, GST_SEEK_TYPE_SET, 0,
                                        GST_SEEK_TYPE_SET, seek_time);
    }
    if(!gst_element_send_event(m_impl->m_pipeline, seek_event)){ LOG_WARNING << "seek failed"; }
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
    return m_impl && m_impl->m_loop;
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

void MediaController::set_on_load_callback(callback_t c)
{
    if(!m_impl){ return; }
    m_impl->m_on_load_cb = c;
}

/////////////////////////////////////////////////////////////////

void MediaController::set_media_ended_callback(callback_t c)
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
