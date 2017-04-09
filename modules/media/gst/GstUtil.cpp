//
// Created by crocdialer on 4/9/17.
//

#include "GstUtil.h"

namespace kinski{ namespace media{

std::weak_ptr<GstGLDisplay> GstUtil::s_gst_gl_display = nullptr;
const int GstUtil::s_enable_async_state_change = true;

GstUtil::GstUtil()
{
    auto success = init_gstreamer();

    if(success)
    {
        m_g_main_loop = g_main_loop_new(nullptr, false);
        m_thread = std::thread(&g_main_loop_run, m_g_main_loop);
        m_gst_gl_display = s_gst_gl_display.lock();
    }
}

GstUtil::~GstUtil()
{
//    m_playing = false;

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
}

bool GstUtil::init_gstreamer()
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

void GstUtil::reset_pipeline()
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

void GstUtil::construct_pipeline()
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
            s_gst_gl_display = m_gst_gl_display;
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

void GstUtil::process_sample(GstSample* sample)
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

void GstUtil::update_state(GstState the_state)
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
                if(m_on_load_cb){ m_on_load_cb(); }
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

bool GstUtil::set_pipeline_state(GstState the_target_state)
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

    if(!s_enable_async_state_change && state_change_result == GST_STATE_CHANGE_ASYNC)
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

void GstUtil::add_bus_watch(GstElement* the_pipeline)
{
    m_gst_bus = gst_pipeline_get_bus(GST_PIPELINE(the_pipeline));
    m_bus_id = gst_bus_add_watch(m_gst_bus, check_bus_messages_async, this);
    gst_bus_set_sync_handler(m_gst_bus, check_bus_messages_sync, this, nullptr);
    gst_object_unref(m_gst_bus);
}

void GstUtil::reset_bus()
{
    if(g_source_remove(m_bus_id)){ m_bus_id = -1; }
    m_gst_bus = nullptr;
}

void GstUtil::on_gst_eos(GstAppSink *sink, gpointer userData)
{

}

GstFlowReturn GstUtil::on_gst_sample(GstAppSink *sink, gpointer userData)
{
    GstUtil *self = static_cast<GstUtil*>(userData);
    self->process_sample(gst_app_sink_pull_sample(sink));
    return GST_FLOW_OK;
}

GstFlowReturn GstUtil::on_gst_preroll(GstAppSink *sink, gpointer userData)
{
    GstUtil *self = static_cast<GstUtil*>(userData);
    self->process_sample(gst_app_sink_pull_preroll(sink));
    return GST_FLOW_OK;
}

const std::atomic<unsigned int> &GstUtil::num_video_channels() const
{
    return m_num_video_channels;
}

const std::atomic<unsigned int> &GstUtil::num_audio_channels() const
{
    return m_num_audio_channels;
}

const std::atomic<bool> &GstUtil::has_subtitle() const
{
    return m_has_subtitle;
}

const std::atomic<bool> &GstUtil::is_prerolled() const
{
    return m_prerolled;
}

const std::atomic<bool> &GstUtil::is_live() const
{
    return m_live;
}

const std::atomic<bool> &GstUtil::is_buffering() const
{
    return m_buffering;
}

const std::atomic<bool> &GstUtil::has_new_frame() const
{
    return m_has_new_frame;
}

const std::atomic<float> &GstUtil::fps() const
{
    return m_fps;
}

const std::atomic<bool> &GstUtil::is_done() const
{
    return m_done;
}

const std::atomic<bool> &GstUtil::is_paused() const
{
    return m_pause;
}

}}//namespaces
