// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2011, ART+COM AG Berlin, Germany <www.artcom.de>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

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
