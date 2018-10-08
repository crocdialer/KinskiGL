//
// Created by crocdialer on 4/9/17.
//

#ifndef KINSKIGL_GSTUTIL_H
#define KINSKIGL_GSTUTIL_H

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "core/file_functions.hpp"
#include "gl/gl.hpp"

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>
#include <gst/gl/gstglconfig.h>

#if defined(KINSKI_ARM)
#undef GST_GL_HAVE_OPENGL
//#undef GST_GL_HAVE_GLES2
#undef GST_GL_HAVE_PLATFORM_GLX
#else // Desktop
#undef GST_GL_HAVE_GLES2
#undef GST_GL_HAVE_PLATFORM_EGL
#undef GST_GL_HAVE_GLEGLIMAGEOES
#endif

//#define GST_USE_UNSTABLE_API
#include <gst/gl/gstglcontext.h>
#include <gst/gl/gstgldisplay.h>
#include <gst/gl/gstglmemory.h>

#if defined(KINSKI_ARM)
#include <gst/gl/egl/gstgldisplay_egl.h>
#elif defined(KINSKI_LINUX)
#include <gst/gl/x11/gstgldisplay_x11.h>
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#include "GLFW/glfw3native.h"
#elif defined(KINSKI_MAC)
#include <gst/gl/cocoa/gstgldisplay_cocoa.h>
#endif

namespace kinski{ namespace media{

class GstUtil
{
public:
    GstUtil(bool use_gl);
    ~GstUtil();

    bool set_pipeline_state(GstState the_target_state);

    void reset_pipeline();

    void use_pipeline(GstElement *the_pipeline, GstElement *the_appsink = nullptr);

    GstElement* pipeline(){ return m_pipeline; }

    GstClock* clock(){ return m_gst_clock.get(); }

    std::shared_ptr<GstBuffer> new_buffer();

    std::shared_ptr<GstBuffer> wait_for_buffer();

    const GstVideoInfo &video_info() const;

    bool is_playing() const;
    const uint32_t num_video_channels() const;
    const uint32_t num_audio_channels() const;
    const bool has_subtitle() const;
    const bool is_prerolled() const;
    const bool is_live() const;
    const bool is_buffering() const;
    const bool has_new_frame() const;
    const float fps() const;
    const bool is_eos() const;
    const bool is_paused() const;

    void set_on_load_cb(const std::function<void()> &the_cb);
    void set_on_end_cb(const std::function<void()> &the_cb);
    void set_on_new_frame_cb(const std::function<void(std::shared_ptr<GstBuffer>)> &the_cb);
    void set_on_aysnc_done_cb(const std::function<void()> &the_cb);

private:
    static std::weak_ptr<GMainLoop> s_g_main_loop;
    static std::weak_ptr<std::thread> s_thread;
    static std::weak_ptr<GstGLDisplay> s_gst_gl_display;
    static const int s_enable_async_state_change;

    bool m_use_gl;
    std::atomic<uint32_t> m_num_video_channels;
    std::atomic<uint32_t> m_num_audio_channels;
    std::atomic<bool> m_has_subtitle;
    std::atomic<bool> m_prerolled;
    std::atomic<bool> m_live;
    std::atomic<bool> m_buffering;
    std::atomic<bool> m_has_new_frame;
    std::atomic<bool> m_video_has_changed;
    std::atomic<float> m_fps;
    std::atomic<bool> m_end_of_stream;
    std::atomic<bool> m_pause;

    std::function<void()> m_on_load_cb, m_on_end_cb, m_on_async_done_cb;
    std::function<void(std::shared_ptr<GstBuffer>)> m_on_new_frame_cb;

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
    std::shared_ptr<GMainLoop> m_g_main_loop;

    // delivers the messages
    GstBus* m_gst_bus = nullptr;

    // Save the id of the bus for releasing when not needed.
    int m_bus_id;

    std::shared_ptr<GstGLDisplay> m_gst_gl_display;

    // runs GMainLoop.
    std::shared_ptr<std::thread> m_thread;

    // network syncing
    std::shared_ptr<GstClock> m_gst_clock;

    // protect appsink callbacks
    std::mutex m_mutex;

    // blocking stuff
    std::condition_variable m_condition_new_frame;

    bool init_gstreamer();

    static GstBusSyncReply check_bus_messages_sync(GstBus* bus, GstMessage* message, gpointer userData);
    static gboolean check_bus_messages_async(GstBus* bus, GstMessage* message, gpointer userData);
    static void on_gst_eos(GstAppSink* sink, gpointer userData);
    static GstFlowReturn on_gst_sample(GstAppSink* sink, gpointer userData);
    static GstFlowReturn on_gst_preroll(GstAppSink* sink, gpointer userData);

    void set_eos();

    void process_sample(GstSample *sample);

    void reset_bus();

    void add_bus_watch(GstElement *the_pipeline);

    void update_state(GstState the_state);
};
}}// namspaces

#endif //KINSKIGL_GSTUTIL_H
