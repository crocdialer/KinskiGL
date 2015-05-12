#ifndef __gl__CameraController__
#define __gl__CameraController__

#include "gl/KinskiGL.h"

/*
* This class controls a camera capture
*/

namespace kinski
{
    class CameraController;
    typedef std::shared_ptr<CameraController> CameraControllerPtr;
    
    class CameraController
    {
        
    public:
        static CameraControllerPtr create(int device_id = 0);
        
        virtual ~CameraController();
        
        void start_capture();
        
        void stop_capture();
        
        bool is_capturing() const;
        
        int device_id() const;
        
        /*!
         * upload the current frame to a std::vector<uint8_t> object,
         * if provided the width and height are written to the according pointers.
         * return: true if a new frame could be successfully uploaded,
         * false otherwise.
         */
        bool copy_frame(std::vector<uint8_t>& data, int *width = nullptr, int *height = nullptr);
        
        /*!
         * upload the current frame to a gl::Texture object
         * return: true if a new frame could be successfully uploaded,
         * false otherwise
         */
        bool copy_frame_to_texture(gl::Texture &tex);
        
    private:
        
        CameraController(int device_id);
        struct Impl;
        std::shared_ptr<Impl> m_impl;
    };
}

#endif
