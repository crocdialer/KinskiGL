#include "gl/Texture.hpp"
#include "gl/Buffer.hpp"
#include "GstUtil.h"
#include "CameraController.hpp"

namespace kinski{ namespace media{

struct CameraControllerImpl
{
    GstUtil m_gst_util;
    int m_device_id = -1;

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
    if(m_impl){ m_impl->m_gst_util.set_pipeline_state(GST_STATE_PLAYING); }
}

void CameraController::stop_capture()
{
    if(m_impl){ m_impl->m_gst_util.set_pipeline_state(GST_STATE_PAUSED); }
}

bool CameraController::copy_frame(std::vector<uint8_t>& data, int *width, int *height)
{
    if(m_impl)
    {
        GstMemory* mem = m_impl->m_gst_util.new_buffer();

        if(mem)
        {

        }
    }
    return false;
}

bool CameraController::copy_frame_to_texture(gl::Texture &tex)
{
    return false;
}

bool CameraController::is_capturing() const
{
    if(m_impl){ return m_impl->m_gst_util.is_playing(); }
    return false;
}

}} // namespaces
