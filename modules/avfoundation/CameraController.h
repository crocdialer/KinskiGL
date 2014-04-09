#ifndef __kinskiGL__CameraController__
#define __kinskiGL__CameraController__

#include "kinskiGL/KinskiGL.h"

/*
* This class controls a camera capture
*/

namespace kinski
{
    class CameraController
    {
    private:
        
        struct Impl;
        std::unique_ptr<Impl> m_impl;
        
    public:
        CameraController();
        virtual ~CameraController();
        
        void start_capture();
        void stop_capture();
        bool is_capturing() const;
        
        /*!
         * upload the current frame to a gl::Texture object
         * return: true if a new frame could be successfully uploaded,
         * false otherwise
         */
        bool copy_frame_to_texture(gl::Texture &tex);
    };
    
    typedef std::shared_ptr<CameraController> CameraControllerPtr;
}

#endif
