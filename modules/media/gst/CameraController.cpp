#include "gl/Texture.hpp"
#include "gl/Buffer.hpp"
#include "CameraController.hpp"

namespace kinski{ namespace media{

    struct CameraController::Impl
    {
        Impl(int device_id)
        {
        }

        ~Impl()
        {
            
        };
    };

    CameraControllerPtr CameraController::create(int device_id)
    {
        return CameraControllerPtr(new CameraController(device_id));
    }

    CameraController::CameraController(int device_id):
    m_impl(new Impl(device_id))
    {

    }

    int CameraController::device_id() const
    {
        return -1;
    }

    CameraController::~CameraController()
    {
        stop_capture();
    }

    void CameraController::start_capture()
    {

    }

    void CameraController::stop_capture()
    {

    }

    bool CameraController::copy_frame(std::vector<uint8_t>& data, int *width, int *height)
    {
        return false;
    }

    bool CameraController::copy_frame_to_texture(gl::Texture &tex)
    {
        return false;
    }

    bool CameraController::is_capturing() const
    {
        return false;
    }
}} // namespaces
