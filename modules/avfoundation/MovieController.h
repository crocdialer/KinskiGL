// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2011, ART+COM AG Berlin, Germany <www.artcom.de>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef _included_mobile_ios_MovieController_
#define _included_mobile_ios_MovieController_

#include "kinskiGL/KinskiGL.h"
#import <CoreVideo/CoreVideo.h>

/*
* This class controls playback of Movie-widgets and manages their assets.
* There is 1:1 correlation between spark::Movie object and MovieControllers,
* each controller beeing responsible for exactly one widget.
* Objective-C classes are wrapped in a forward declared C-struct (AVStruct) for compatibility reasons.
*
* Due to planned experiments this class is still a little messy :(
*/

namespace kinski
{
    
    class MovieController 
    {
    private:
        
        std::shared_ptr<struct MovieControllerImpl> m_impl;
        
    public:
        MovieController();
        virtual ~MovieController();
        
        void load(const std::string &filePath);
        void play();
        bool isPlaying() const;
        void stop();
        void pause();
        void seek_to_time(float value);
        double duration();
        double current_time();
        float getVolume() const;
        void setVolume(const float newVolume);
        void set_loop(bool b);
        void set_rate(float r);
        bool loop();
        const std::string& get_path();
        
        bool copy_frame_to_texture(gl::Texture &tex);
    };
    
    typedef std::shared_ptr<MovieController> MovieControllerPtr;
}

#endif
