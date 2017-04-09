#include "GstUtil.h"
#include "gl/Texture.hpp"
#include "MediaController.hpp"

namespace kinski{ namespace media
{

namespace
{
    std::weak_ptr<GstGLDisplay> g_gst_gl_display;
    const int g_enable_async_state_change = true;
};

struct MediaControllerImpl
{
    std::string m_src_path;
    std::atomic<float> m_rate;
    std::atomic<float> m_volume;
    std::atomic<float> m_fps;
    std::atomic<uint32_t> m_num_video_channels;
    std::atomic<uint32_t> m_num_audio_channels;
    std::atomic<bool> m_has_subtitle;
    std::atomic<bool> m_prerolled;
    std::atomic<bool> m_live;
    std::atomic<bool> m_buffering;
    std::atomic<bool> m_seeking;
    std::atomic<bool> m_seek_requested;
    std::atomic<int64_t> m_current_time_nanos;
    std::atomic<int64_t> m_seek_requested_nanos;
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

    // network syncing
    std::shared_ptr<GstClock> m_gst_clock;

    // protect appsink callbacks
    std::mutex m_mutex;

    std::weak_ptr<MediaController> m_media_controller;
    MediaController::callback_t m_on_load_cb, m_media_ended_cb;

    MediaController::RenderTarget m_render_target = MediaController::RenderTarget::TEXTURE;
    MediaController::AudioTarget m_audio_target = MediaController::AudioTarget::AUTO;

    MediaControllerImpl():
    m_rate(1.f),
    m_volume(1.f),
    m_fps(0.f),
    m_num_video_channels(0),
    m_num_audio_channels(0),
    m_has_subtitle(false),
    m_prerolled(false),
    m_live(false),
    m_buffering(false),
    m_seeking(false),
    m_seek_requested(false),
    m_current_time_nanos(0),
    m_seek_requested_nanos(0),
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
                sprintf(buf, "GStreamer %i.%i.%i.%i", major, minor, micro, nano);
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

        // pipeline will unref and destroy its children..
        gst_object_unref(GST_OBJECT(m_pipeline));
        m_pipeline = nullptr;
        m_video_bin = nullptr;
        m_gl_upload = nullptr;
        m_gl_color_convert = nullptr;

        gst_object_unref(m_gl_context);
        m_gl_context = nullptr;
    }

    void construct_pipeline()
    {
        if(m_pipeline) return;

        m_pipeline = gst_element_factory_make("playbin", "playbinsink");
        m_gst_clock = std::shared_ptr<GstClock>(gst_pipeline_get_clock(GST_PIPELINE(m_pipeline)),
                                                &gst_object_unref);

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
            if(!m_gst_gl_display)
            {
                GstGLDisplay* gl_display = nullptr;
#if defined(KINSKI_EGL)
                auto platform_data_egl = std::dynamic_pointer_cast<gl::PlatformDataEGL>(gl::context()->platform_data());
                gl_display = (GstGLDisplay*) gst_gl_display_egl_new_with_egl_display(platform_data_egl->egl_display);
#elif defined(KINSKI_LINUX)
                gl_display = (GstGLDisplay*)gst_gl_display_x11_new_with_display(glfwGetX11Display());
#elif defined(KINSKI_MAC)
                gl_display = gst_gl_display_new();
#endif
                m_gst_gl_display = std::shared_ptr<GstGLDisplay>(gl_display, &gst_object_unref);
                g_gst_gl_display = m_gst_gl_display;
            }
#if defined(KINSKI_EGL)
            auto platform_data_egl = std::dynamic_pointer_cast<gl::PlatformDataEGL>(gl::context()->platform_data());
            m_gl_context = gst_gl_context_new_wrapped(m_gst_gl_display.get(), (guintptr)platform_data_egl->egl_context,
                                                      GST_GL_PLATFORM_EGL, GST_GL_API_GLES2);
#elif defined(KINSKI_LINUX)
            m_gl_context = gst_gl_context_new_wrapped(m_gst_gl_display.get(),
                                                      (guintptr)::glfwGetGLXContext(glfwGetCurrentContext()),
                                                      GST_GL_PLATFORM_GLX, GST_GL_API_OPENGL);
#elif defined(KINSKI_MAC)
            auto platform_data_mac = std::dynamic_pointer_cast<gl::PlatformDataCGL>(gl::context()->platform_data());
            m_gl_context = gst_gl_context_new_wrapped(m_gst_gl_display.get(),
                                                      (guintptr)platform_data_mac->cgl_context,
                                                      GST_GL_PLATFORM_CGL, GST_GL_API_OPENGL);
#endif

            m_gl_upload = gst_element_factory_make("glupload", "upload");
            if(!m_gl_upload){ LOG_ERROR << "failed to create GL upload element"; };

            m_gl_color_convert = gst_element_factory_make("glcolorconvert", "convert");
            if(!m_gl_color_convert){ LOG_ERROR << "failed to create GL convert element"; }

            m_raw_caps_filter = gst_element_factory_make("capsfilter", "rawcapsfilter");

#if defined(KINSKI_EGL)
            if(m_raw_caps_filter)
            {
                g_object_set(G_OBJECT(m_raw_caps_filter), "caps",
                             gst_caps_from_string("video/x-raw(memory:GLMemory)"), nullptr);
            }
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
                break;

            case GST_STATE_PAUSED:
            {
                // fire onload callback
                if(!m_prerolled)
                {
                    gint num_audio_channels = 0, num_video_channels = 0;
                    g_object_get(G_OBJECT(m_pipeline), "n-audio", &num_audio_channels, nullptr);
                    m_num_audio_channels = num_audio_channels;
                    g_object_get(G_OBJECT(m_pipeline), "n-video", &num_video_channels, nullptr);
                    m_num_video_channels = num_video_channels;
                    m_prerolled = true;
                    if(m_on_load_cb){ m_on_load_cb(m_media_controller.lock()); }
                };
                m_pause = true;
                break;
            }
            case GST_STATE_PLAYING:
                m_done = false;
                m_pause = false;
                break;

            default: break;
        }
    }

    bool set_pipeline_state(GstState the_target_state)
    {
        if(!m_pipeline){ return false; }

        GstState current, pending;
        gst_element_get_state(m_pipeline, &current, &pending, 0);
        m_target_state = the_target_state;

        // unnecessary state change
        if(current == the_target_state || pending == the_target_state)
            return true;

        GstStateChangeReturn state_change_result = gst_element_set_state(m_pipeline, m_target_state);
        LOG_TRACE_2 << "pipeline state about to change from: " << gst_element_state_get_name(current) <<
                    " to " << gst_element_state_get_name(the_target_state);

        if(!g_enable_async_state_change && state_change_result == GST_STATE_CHANGE_ASYNC)
        {
            LOG_TRACE_2 << "blocking until pipeline state changes from: "
                        << gst_element_state_get_name(current) << " to: "
                        << gst_element_state_get_name(the_target_state);
            state_change_result = gst_element_get_state(m_pipeline, &current, &pending, GST_CLOCK_TIME_NONE);
        }

        switch(state_change_result)
        {
            case GST_STATE_CHANGE_FAILURE:
                LOG_WARNING << "pipeline failed to change state";
                return false;

            case GST_STATE_CHANGE_SUCCESS:
                LOG_TRACE_2 << "pipeline state changed SUCCESSFULLY from: "
                            << gst_element_state_get_name(m_current_state) << " to: "
                            << gst_element_state_get_name(m_target_state);

                // target state reached
                update_state(m_target_state);
                return true;

            case GST_STATE_CHANGE_ASYNC:
                LOG_TRACE_2 << "pipeline state change will happen ASYNC from: "
                            << gst_element_state_get_name(m_current_state) << " to: "
                            << gst_element_state_get_name(m_target_state);
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

    void send_seek_event(gint64 the_position_nanos, bool force_seek = false)
    {
        if(!m_prerolled){ return; }

        GstState current, pending;
        GstStateChangeReturn state_change = gst_element_get_state(m_pipeline, &current, &pending, 0);

        if(!force_seek && (state_change == GST_STATE_CHANGE_ASYNC || m_buffering))
        {
            m_seek_requested = true;
            m_seek_requested_nanos = the_position_nanos;
            return;
        }

        GstEvent* seek_event;
        GstSeekFlags seek_flags = GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE);
        if(fabsf(m_rate) > 2){ seek_flags = GstSeekFlags(seek_flags | GST_SEEK_FLAG_TRICKMODE); }

        if(m_rate > 0.0)
        {
            seek_event = gst_event_new_seek(m_rate, GST_FORMAT_TIME, seek_flags, GST_SEEK_TYPE_SET, the_position_nanos,
                                            GST_SEEK_TYPE_SET, GST_CLOCK_TIME_NONE);
        }
        else
        {
            seek_event = gst_event_new_seek(m_rate, GST_FORMAT_TIME, seek_flags, GST_SEEK_TYPE_SET, 0,
                                            GST_SEEK_TYPE_SET, the_position_nanos);
        }
        if(!gst_element_send_event(m_pipeline, seek_event)){ LOG_WARNING << "seek failed"; }
    }

    gint64 current_time_nanos()
    {
        gint64 position = m_current_time_nanos;

        if(m_prerolled)
        {
            if(!gst_element_query_position(m_pipeline, GST_FORMAT_TIME, &position))
            {
                if(m_seek_requested)
                {
                    position = m_seek_requested_nanos;
                }
            }
            m_current_time_nanos = position;
        }
        return position;
    }

    void process_sample(GstSample* sample)
    {
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

            LOG_TRACE_2 << "need context " << context_type << " from element " << GST_ELEMENT_NAME(GST_MESSAGE_SRC(message));

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
            LOG_TRACE_2 << "have context " << context_type << " from element " << GST_ELEMENT_NAME(GST_MESSAGE_SRC(message));
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
            LOG_DEBUG << "buffering " << std::setfill('0') << std::setw(3) << percent << " %";

            if(percent == 100)
            {
                self->m_buffering = false;
                LOG_TRACE_2 << "buffering complete!";

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
                    LOG_TRACE_2 << "buffering in process ...";
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
                    LOG_TRACE_2 << "pipeline state changed from: "
                                << gst_element_state_get_name(old) << " to: "
                                << gst_element_state_get_name(current) << " with pending: "
                                << gst_element_state_get_name(pending);
                }
                self->update_state(current);

                if(self->m_target_state != self->m_current_state && pending == GST_STATE_VOID_PENDING)
                {
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
                            self->send_seek_event(self->m_seek_requested_nanos);
                            self->m_seek_requested = false;
                        }
                        else{ self->m_seeking = false; }
                    }
                    break;
                }

                default:
                    break;
            }
            break;
        }

        case GST_MESSAGE_EOS:
        {
            auto mc = self->m_media_controller.lock();

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
                    // if playing back on reverse start the loop from the
                    // end of the file
                    if(self->m_rate < 0)
                    {
                        if(mc) mc->seek_to_time(mc->duration());
                    }
                    else
                    {
                        self->send_seek_event(0);
                    }
                }
            }
            self->m_done = true;

            // fire media ended callback, if any
            if(mc && self->m_media_ended_cb){ self->m_media_ended_cb(mc); }
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
    callback_t on_end = m_impl ? m_impl->m_media_ended_cb : callback_t();
    m_impl.reset(new MediaControllerImpl());
    m_impl->m_src_path = found_path;
    m_impl->m_loop = loop;
    m_impl->m_media_controller = shared_from_this();
    m_impl->m_on_load_cb = on_load;
    m_impl->m_media_ended_cb = on_end;
    m_impl->m_render_target = the_render_target;
    m_impl->m_audio_target = the_audio_target;

    // construct a pipeline
    m_impl->construct_pipeline();
    m_impl->set_pipeline_state(GST_STATE_READY);

    std::string uri_path = filePath;

    if(fs::is_uri(filePath)){ m_impl->m_stream = true; }
    else
    {
        GError* err = nullptr;
        gchar* uri = gst_filename_to_uri(filePath.c_str(), &err);
        uri_path = std::string(static_cast<const char*>(uri));
        g_free(uri);
        if(err){ g_free(err); }
    }

    // set the new path
    g_object_set(G_OBJECT(m_impl->m_pipeline), "uri", uri_path.c_str(), nullptr);

    // preroll
    m_impl->set_pipeline_state(GST_STATE_PAUSED);

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
    return m_impl && m_impl->m_prerolled;
}

/////////////////////////////////////////////////////////////////

void MediaController::unload()
{
    m_impl.reset();
}

/////////////////////////////////////////////////////////////////

bool MediaController::has_video() const
{
    return m_impl && m_impl->m_num_video_channels;
}

/////////////////////////////////////////////////////////////////

bool MediaController::has_audio() const
{
    return m_impl && m_impl->m_num_audio_channels;
}

/////////////////////////////////////////////////////////////////

void MediaController::pause()
{
    if(m_impl){ m_impl->set_pipeline_state(GST_STATE_PAUSED); }
}

/////////////////////////////////////////////////////////////////

bool MediaController::is_playing() const
{
    if(m_impl && m_impl->m_pipeline)
    {
        GstState current, pending;
        gst_element_get_state(m_impl->m_pipeline, &current, &pending, 0);
        return current == GST_STATE_PLAYING;
    }
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
    if(m_impl){ return m_impl->m_volume; }
    return 0.f;
}

/////////////////////////////////////////////////////////////////

void MediaController::set_volume(float newVolume)
{
    if(m_impl && m_impl->m_pipeline)
    {
        newVolume = clamp(newVolume, 0.f, 1.f);
        m_impl->m_volume = newVolume;
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
        std::lock_guard<std::mutex> guard(m_impl->m_mutex);

        GstMemory *mem = gst_buffer_peek_memory(m_impl->m_new_buffer.get(), 0);

        if(gst_is_gl_memory(mem))
        {
            GstGLMemory *gl_mem = (GstGLMemory*)(mem);

            GLint id = gl_mem->tex_id;
            GLint target = GL_TEXTURE_2D;
#if !defined(KINSKI_GLES)
            if(gl_mem->tex_target == GST_GL_TEXTURE_TARGET_RECTANGLE){ target = GL_TEXTURE_RECTANGLE; }
#endif
            tex = gl::Texture(target, id, m_impl->m_video_info.width, m_impl->m_video_info.height, true);
            tex.set_flipped(true);
        }
        std::swap(m_impl->m_new_buffer, m_impl->m_current_buffer);
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
        return duration / (double)GST_SECOND;
    }
    return 0.0;
}

/////////////////////////////////////////////////////////////////

double MediaController::current_time() const
{
    if(m_impl)
    {
        return m_impl->current_time_nanos() / (double)GST_SECOND;
    }
    return 0.0;
}

/////////////////////////////////////////////////////////////////

double MediaController::fps() const
{
    if(m_impl){ return m_impl->m_fps; }
    return 0.0;
}

/////////////////////////////////////////////////////////////////

void MediaController::seek_to_time(double value)
{
    if(m_impl)
    {
        m_impl->m_seeking = true;
        m_impl->m_done = false;
        m_impl->send_seek_event(value * GST_SECOND);
    }
}

/////////////////////////////////////////////////////////////////

void MediaController::set_loop(bool b)
{
    if(m_impl){ m_impl->m_loop = b; }
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
    if(!is_loaded() || (r < 0.0f && m_impl->m_stream)) return;

    // rate equal to 0 is not valid and has to be handled by pausing the pipeline.
    if(r == 0.0f)
    {
        pause();
        return;
    }
    m_impl->m_rate = r;
    gint64 current_time = m_impl->current_time_nanos();
    m_impl->send_seek_event(current_time);
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
    m_impl->m_media_ended_cb = c;
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
