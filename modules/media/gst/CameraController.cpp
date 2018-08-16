#include "gl/Texture.hpp"
#include "gl/Buffer.hpp"
#include "GstUtil.h"
#include "CameraController.hpp"

namespace kinski{ namespace media{

struct CameraControllerImpl
{
    GstUtil m_gst_util;

    // memory map that holds the incoming frame.
    GstMapInfo m_memory_map_info;

    int m_device_id = -1;

    gl::Buffer m_buffer_front, m_buffer_back;

    CameraControllerImpl(int device_id):
    m_gst_util(false),
    m_device_id(device_id)
    {

    }

    ~CameraControllerImpl()
    {

    };
};

CameraControllerPtr CameraController::create(int device_id)
{
    return CameraControllerPtr(new CameraController(device_id));
}

CameraController::CameraController(int device_id):
m_impl(new CameraControllerImpl(device_id))
{

}

CameraController::~CameraController()
{
    stop_capture();
}

int CameraController::device_id() const
{
    return m_impl->m_device_id;
}

void CameraController::start_capture()
{
    if(m_impl)
    {
        int w = 1280, h = 720;
//        int framerate = 60;
        const char* app_sink_name = "kinski_appsink";

        std::string pipeline_str =
                "v4l2src device=/dev/video%d ! "
//                "video/x-raw, format=RGB, width=%d, height=%d, framerate=%d/1 !"
                "video/x-raw, width=%d, height=%d !"
                "decodebin !"
                "videoconvert !"
                "appsink name=%s enable-last-sample=0 caps=\"video/x-raw,format=RGB\"";
        pipeline_str = format(pipeline_str, m_impl->m_device_id, w, h, app_sink_name);
        GError *error = nullptr;

        // construct a pipeline
        GstElement *pipeline = gst_parse_launch(pipeline_str.c_str(), &error);

        if(error)
        {
            LOG_ERROR << "could not construct pipeline: " << error->message;
            g_error_free(error);
        }
        else
        {
            GstElement* sink = gst_bin_get_by_name(GST_BIN(pipeline), app_sink_name);
            gst_base_sink_set_sync(GST_BASE_SINK(sink), true);

            m_impl->m_gst_util.use_pipeline(pipeline, sink);
            m_impl->m_gst_util.set_pipeline_state(GST_STATE_READY);

#if !defined(KINSKI_ARM)
            m_impl->m_buffer_front = gl::Buffer(GL_PIXEL_UNPACK_BUFFER, GL_STREAM_COPY);
            m_impl->m_buffer_back = gl::Buffer(GL_PIXEL_UNPACK_BUFFER, GL_STREAM_COPY);
#endif
            m_impl->m_gst_util.set_pipeline_state(GST_STATE_PLAYING);
        }
    }
}

void CameraController::stop_capture()
{
    if(m_impl){ m_impl->m_gst_util.set_pipeline_state(GST_STATE_READY); }
}

bool CameraController::copy_frame(std::vector<uint8_t>& out_data, int *width, int *height)
{
    GstBuffer* buf = m_impl->m_gst_util.new_buffer();

    if(buf)
    {
        *width = m_impl->m_gst_util.video_info().width;
        *height = m_impl->m_gst_util.video_info().height;

        // map the buffer for reading
        gst_buffer_map(buf, &m_impl->m_memory_map_info, GST_MAP_READ);
        uint8_t *buf_data = m_impl->m_memory_map_info.data;
        size_t num_bytes = m_impl->m_memory_map_info.size;
        out_data.assign(buf_data, buf_data + num_bytes);
        gst_buffer_unmap(buf, &m_impl->m_memory_map_info);
        return true;
    }
    return false;
}

/////////////////////////////////////////////////////////////////

bool CameraController::copy_frame_to_image(ImagePtr& the_image)
{
    GstBuffer* buf = m_impl->m_gst_util.new_buffer();

    if(buf)
    {
        constexpr uint8_t num_channels = 3;
        uint32_t w = m_impl->m_gst_util.video_info().width;
        uint32_t h = m_impl->m_gst_util.video_info().height;

        if(!the_image || the_image->width() != w || the_image->height() != h ||
           the_image->num_components() != num_channels)
        {
            auto img = Image_<uint8_t>::create(w, h, num_channels);
            img->m_type = Image::Type::RGB;
            the_image = img;
        }

        // map the buffer for reading
        gst_buffer_map(buf, &m_impl->m_memory_map_info, GST_MAP_READ);
        uint8_t *buf_data = m_impl->m_memory_map_info.data;
        size_t num_bytes = m_impl->m_memory_map_info.size;
        memcpy(the_image->data(), buf_data, num_bytes);
        gst_buffer_unmap(buf, &m_impl->m_memory_map_info);
        return true;
    }
    return false;
}

/////////////////////////////////////////////////////////////////

bool CameraController::copy_frame_to_texture(gl::Texture &tex)
{
#if !defined(KINSKI_ARM)

    if(m_impl)
    {
        GstBuffer* buf = m_impl->m_gst_util.new_buffer();

        if(buf)
        {
            int width = m_impl->m_gst_util.video_info().width;
            int height = m_impl->m_gst_util.video_info().height;

            // map the buffer for reading
            gst_buffer_map(buf, &m_impl->m_memory_map_info, GST_MAP_READ);
            uint8_t *buf_data = m_impl->m_memory_map_info.data;
            size_t num_bytes = m_impl->m_memory_map_info.size;

            if(m_impl->m_buffer_front.num_bytes() != num_bytes)
            {
                m_impl->m_buffer_front.set_data(nullptr, num_bytes);
            }

            uint8_t *ptr = m_impl->m_buffer_front.map();
            memcpy(ptr, buf_data, num_bytes);
            m_impl->m_buffer_front.unmap();
            gst_buffer_unmap(buf, &m_impl->m_memory_map_info);

            // bind pbo and schedule texture upload
            m_impl->m_buffer_front.bind();
            tex.update(nullptr, GL_UNSIGNED_BYTE, GL_RGB, width, height, true);
            m_impl->m_buffer_front.unbind();

            // ping pong our pbos
            std::swap(m_impl->m_buffer_front, m_impl->m_buffer_back);
            return true;
        }
    }
#endif
    return false;
}

bool CameraController::is_capturing() const
{
    if(m_impl){ return m_impl->m_gst_util.is_playing(); }
    return false;
}

}} // namespaces
