#pragma once

#include "gl/gl.hpp"

/*
* This class controls a camera capture
*/

namespace kinski{ namespace media{

    DEFINE_CLASS_PTR(CameraController);
    
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
         * copy the current frame to a kinski::ImagePtr
         * @return true if a new frame has been copied successfully,
         * false otherwise
         */
        bool copy_frame_to_image(ImagePtr& the_image);
        
        /*!
         * upload the current frame to a gl::Texture object
         * return: true if a new frame could be successfully uploaded,
         * false otherwise
         */
        bool copy_frame_to_texture(gl::Texture &tex);
        
    private:
        
        CameraController(int device_id);
        std::shared_ptr<struct CameraControllerImpl> m_impl;
    };
}} // namespaces
