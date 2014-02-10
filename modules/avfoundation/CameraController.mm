// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2011, ART+COM AG Berlin, Germany <www.artcom.de>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#import <AVFoundation/AVFoundation.h>

#include "kinskiGL/Texture.h"
#include "kinskiGL/Buffer.h"
#include "CameraController.h"

namespace kinski {
    
    struct CameraController::Impl
    {
        
        gl::Buffer m_pbo[2];
        uint8_t m_pbo_index;
        
        Impl(): m_pbo_index(0){}
        ~Impl()
        {

        };
    };
    
    CameraController::CameraController():
    m_impl(new Impl)
    {
        
    }
    
    CameraController::~CameraController()
    {

    }
    
    bool CameraController::copy_frame_to_texture(gl::Texture &tex)
    {
        return false;
    }
}
