#include "GstUtil.h"
#include "gl/Texture.hpp"
#include "gl/Buffer.hpp"
#include "MediaController.hpp"

#include <gst/net/gstnet.h>

namespace kinski{ namespace media
{

struct MediaControllerImpl
{
    std::string m_src_path;
    std::atomic<float> m_rate;
    std::atomic<float> m_volume;
    std::atomic<bool> m_seeking;
    std::atomic<bool> m_seek_requested;
    std::atomic<int64_t> m_current_time_nanos;
    std::atomic<int64_t> m_seek_requested_nanos;
    std::atomic<bool> m_stream;
    std::atomic<bool> m_loop;

    GstUtil m_gst_util;

    // memory map that holds the incoming frame.
    GstMapInfo m_memory_map_info;

    // handle for the pipeline's videosink element
    GstElement *m_video_sink;

    // provides our clock-time for remote synching
    std::shared_ptr<GstNetTimeProvider> m_net_time_provider;

    std::weak_ptr<MediaController> m_media_controller;
    MediaController::callback_t m_on_load_cb, m_on_end_cb;

    MediaController::RenderTarget m_render_target = MediaController::RenderTarget::TEXTURE;
    MediaController::AudioTarget m_audio_target = MediaController::AudioTarget::AUTO;

    MediaControllerImpl():
    m_rate(1.f),
    m_volume(1.f),
    m_seeking(false),
    m_seek_requested(false),
    m_current_time_nanos(0),
    m_seek_requested_nanos(0),
    m_stream(false),
    m_loop(false),
    m_gst_util(true),
    m_video_sink(nullptr)
    {
        init_callbacks();
    }

    ~MediaControllerImpl()
    {

    };

    void init_callbacks()
    {
        m_gst_util.set_on_load_cb([this]()
        {
            auto mc = m_media_controller.lock();
            if(mc && m_on_load_cb){ m_on_load_cb(mc); }
        });

        m_gst_util.set_on_end_cb([this]()
        {
            if(m_seeking){ return; }

            auto mc = m_media_controller.lock();
            if(mc && m_on_end_cb){ m_on_end_cb(mc); }

            if(m_loop)
            {
                // if playing back on reverse start the loop from the
                // end of the file
                if(m_rate < 0){ if(mc) mc->seek_to_time(mc->duration()); }
                else{ send_seek_event(0); }
            }
        });

        m_gst_util.set_on_aysnc_done_cb([this]()
        {
            if(m_seeking)
            {
                if(m_seek_requested)
                {
                    send_seek_event(m_seek_requested_nanos);
                    m_seek_requested = false;
                }
                else{ m_seeking = false; }
            }
        });
    }

    void send_seek_event(gint64 the_position_nanos, bool force_seek = false)
    {
        if(!m_gst_util.is_prerolled()){ return; }

        GstState current, pending;
        GstStateChangeReturn state_change = gst_element_get_state(m_gst_util.pipeline(), &current, &pending, 0);

        if(!force_seek && (state_change == GST_STATE_CHANGE_ASYNC || m_gst_util.is_buffering()))
        {
            m_seek_requested = true;
            m_seek_requested_nanos = the_position_nanos;
            return;
        }

        GstEvent* seek_event;
        GstSeekFlags seek_flags = GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE);
//        if(fabsf(m_rate) > 2){ seek_flags = GstSeekFlags(seek_flags | GST_SEEK_FLAG_TRICKMODE); }

        if(m_rate > 0.0)
        {
            seek_event = gst_event_new_seek(m_rate, GST_FORMAT_TIME, seek_flags, GST_SEEK_TYPE_SET, the_position_nanos,
                                            GST_SEEK_TYPE_NONE, 0);
        }
        else
        {
            seek_event = gst_event_new_seek(m_rate, GST_FORMAT_TIME, seek_flags, GST_SEEK_TYPE_SET, 0,
                                            GST_SEEK_TYPE_SET, the_position_nanos);
        }
        if(!gst_element_send_event(m_gst_util.pipeline(), seek_event)){ LOG_WARNING << "seek failed"; }
    }

    void send_step_event(uint32_t the_num_steps = 1)
    {
        if(!m_video_sink)
        {
            /* If we have not done so, obtain the sink through which we will send the step events */
            g_object_get(m_gst_util.pipeline(), "video-sink", &m_video_sink, NULL);
        }
        gst_element_send_event(m_video_sink,
                               gst_event_new_step(GST_FORMAT_BUFFERS, the_num_steps, std::fabs(m_rate), true, false));
    }

    gint64 current_time_nanos()
    {
        gint64 position = m_current_time_nanos;

        if(m_gst_util.is_prerolled())
        {
            if(!gst_element_query_position(m_gst_util.pipeline(), GST_FORMAT_TIME, &position))
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
};

///////////////////////////////////////////////////////////////////////////////

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
    callback_t on_end = m_impl ? m_impl->m_on_end_cb : callback_t();
    m_impl.reset(new MediaControllerImpl());
    m_impl->m_src_path = found_path;
    m_impl->m_loop = loop;
    m_impl->m_media_controller = shared_from_this();
    m_impl->m_on_load_cb = on_load;
    m_impl->m_on_end_cb = on_end;
    m_impl->m_render_target = the_render_target;
    m_impl->m_audio_target = the_audio_target;

    // construct a pipeline
    m_impl->m_gst_util.use_pipeline(gst_element_factory_make("playbin", "playbinsink"));
    m_impl->m_gst_util.set_pipeline_state(GST_STATE_READY);

    // net time provider
    auto ntp = gst_net_time_provider_new(m_impl->m_gst_util.clock(), nullptr, 0);
    m_impl->m_net_time_provider = std::shared_ptr<GstNetTimeProvider>(ntp, &gst_object_unref);

    std::string uri_path = filePath;

    if(fs::is_uri(filePath)){ m_impl->m_stream = true; }
    else
    {
        GError* err = nullptr;
        gchar* uri = gst_filename_to_uri(found_path.c_str(), &err);
        uri_path = std::string(static_cast<const char*>(uri));
        g_free(uri);
        if(err){ g_free(err); }
    }

    // set the new path
    g_object_set(G_OBJECT(m_impl->m_gst_util.pipeline()), "uri", uri_path.c_str(), nullptr);

    // preroll
    m_impl->m_gst_util.set_pipeline_state(GST_STATE_PAUSED);

    // autoplay
    if(autoplay){ play(); }
}

/////////////////////////////////////////////////////////////////

void MediaController::play()
{
    if(m_impl){ m_impl->m_gst_util.set_pipeline_state(GST_STATE_PLAYING); }
}

/////////////////////////////////////////////////////////////////

bool MediaController::is_loaded() const
{
    return m_impl && m_impl->m_gst_util.is_prerolled();
}

/////////////////////////////////////////////////////////////////

void MediaController::unload()
{
    m_impl.reset(new MediaControllerImpl);
//    m_impl->m_gst_util.reset_pipeline();
}

/////////////////////////////////////////////////////////////////

bool MediaController::has_video() const
{
    return m_impl && m_impl->m_gst_util.num_video_channels();
}

/////////////////////////////////////////////////////////////////

bool MediaController::has_audio() const
{
    return m_impl && m_impl->m_gst_util.num_audio_channels();
}

/////////////////////////////////////////////////////////////////

void MediaController::pause()
{
    if(m_impl){ m_impl->m_gst_util.set_pipeline_state(GST_STATE_PAUSED); }
}

/////////////////////////////////////////////////////////////////

bool MediaController::is_playing() const
{
    if(m_impl){ return m_impl->m_gst_util.is_playing(); }
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
    if(m_impl && m_impl->m_gst_util.pipeline())
    {
        newVolume = clamp(newVolume, 0.f, 1.f);
        m_impl->m_volume = newVolume;
        g_object_set(G_OBJECT(m_impl->m_gst_util.pipeline()), "volume", (gdouble)newVolume, nullptr);
    }
}

/////////////////////////////////////////////////////////////////

bool MediaController::copy_frame(std::vector<uint8_t>& data, int *width, int *height)
{
    auto buf = m_impl->m_gst_util.new_buffer();

    if(buf)
    {
        *width = m_impl->m_gst_util.video_info().width;
        *height = m_impl->m_gst_util.video_info().height;

        // map the buffer for reading
        gst_buffer_map(buf.get(), &m_impl->m_memory_map_info, GST_MAP_READ);
        uint8_t *buf_data = m_impl->m_memory_map_info.data;
        size_t num_bytes = m_impl->m_memory_map_info.size;
        data.assign(buf_data, buf_data + num_bytes);
        gst_buffer_unmap(buf.get(), &m_impl->m_memory_map_info);
        return true;
    }
    return false;
}

/////////////////////////////////////////////////////////////////

bool MediaController::copy_frame_to_image(ImagePtr& the_image)
{
    auto buf = m_impl->m_gst_util.new_buffer();

    if(buf)
    {
        constexpr uint8_t num_channels = 4;
        uint32_t w = m_impl->m_gst_util.video_info().width;
        uint32_t h = m_impl->m_gst_util.video_info().height;

        if(!the_image || the_image->width() != w || the_image->height() != h ||
           the_image->num_components() != num_channels)
        {
            auto img = Image_<uint8_t >::create(w, h, num_channels);
            img->m_type = Image::Type::RGBA;
        }

        // map the buffer for reading
        gst_buffer_map(buf.get(), &m_impl->m_memory_map_info, GST_MAP_READ);
        uint8_t *buf_data = m_impl->m_memory_map_info.data;
        size_t num_bytes = m_impl->m_memory_map_info.size;
        memcpy(the_image->data(), buf_data, num_bytes);
        gst_buffer_unmap(buf.get(), &m_impl->m_memory_map_info);
        return true;
    }
    return false;
}

/////////////////////////////////////////////////////////////////

bool MediaController::copy_frame_to_texture(gl::Texture &tex, bool as_texture2D)
{
    auto buf = m_impl->m_gst_util.new_buffer();

    if(buf)
    {
        GstMemory *mem = gst_buffer_peek_memory(buf.get(), 0);

        if(mem && gst_is_gl_memory(mem))
        {
            GstGLMemory *gl_mem = (GstGLMemory*)(mem);

            GLint id = gl_mem->tex_id;
            GLint target = GL_TEXTURE_2D;
#if !defined(KINSKI_GLES)
            if(gl_mem->tex_target == GST_GL_TEXTURE_TARGET_RECTANGLE){ target = GL_TEXTURE_RECTANGLE; }
#endif
            tex = gl::Texture(target, id, m_impl->m_gst_util.video_info().width,
                              m_impl->m_gst_util.video_info().height, true);
            tex.set_flipped(true);
            return true;
        }
    }
    return false;
}

/////////////////////////////////////////////////////////////////

bool MediaController::copy_frames_offline(gl::Texture &tex, bool compress)
{
    // rewind + play
    restart();

    // pause
    pause();

    std::deque<std::shared_ptr<GstBuffer>> samples;

    // keep calling wait_for_new_buffer()
    while(auto buf = m_impl->m_gst_util.wait_for_buffer())
    {
        samples.push_back(buf);

        // step to next frame
        m_impl->send_step_event();
    }

    uint32_t num_frames = samples.size();
    uint32_t width, height, i = 0;

    if(num_frames)
    {
        width = (uint32_t)m_impl->m_gst_util.video_info().width;
        height = (uint32_t)m_impl->m_gst_util.video_info().height;

        // aquire gpu-memory for our frames
        gl::Texture::Format fmt;
        fmt.target = GL_TEXTURE_2D_ARRAY;
        fmt.internal_format = compress ? GL_COMPRESSED_RGBA_S3TC_DXT5_EXT : GL_RGBA;
        tex = gl::Texture(width, height, num_frames, fmt);
        tex.set_flipped();
        KINSKI_CHECK_GL_ERRORS();
    }
    else
    {
        LOG_ERROR << "no samples";
        return false;
    }

    while(!samples.empty())
    {
        auto s = samples.front();
        samples.pop_front();

        // map the buffer for reading
        gst_buffer_map(s.get(), &m_impl->m_memory_map_info, GST_MAP_READ);
        uint8_t *buf_data = m_impl->m_memory_map_info.data;

        // upload to texture
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, buf_data);

        // unmap
        gst_buffer_unmap(s.get(), &m_impl->m_memory_map_info);

        // advance index
        i++;
    }
    return true;
}

/////////////////////////////////////////////////////////////////

double MediaController::duration() const
{
    if(m_impl && m_impl->m_gst_util.is_prerolled())
    {
        gint64 duration = 0;
        gst_element_query_duration(m_impl->m_gst_util.pipeline(), GST_FORMAT_TIME, &duration);
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
    if(m_impl){ return m_impl->m_gst_util.fps(); }
    return 0.0;
}

/////////////////////////////////////////////////////////////////

void MediaController::seek_to_time(double value)
{
    if(m_impl)
    {
        m_impl->m_seeking = true;
        m_impl->send_seek_event(value * GST_SECOND);
    }
}

/////////////////////////////////////////////////////////////////

void MediaController::step_frame(int the_num_frames)
{
    // adjust rate to allow reverse stepping
    if(kinski::sgn<float>(m_impl->m_rate) != kinski::sgn(the_num_frames)){ set_rate(-rate()); }
    m_impl->send_step_event(std::abs(the_num_frames));
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
    m_impl->m_on_end_cb = c;
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
