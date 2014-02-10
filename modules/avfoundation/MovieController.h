// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2011, ART+COM AG Berlin, Germany <www.artcom.de>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef __kinskiGL__MovieController__
#define __kinskiGL__MovieController__

#include "kinskiGL/KinskiGL.h"

/*
* This class controls playback of Movies and manages their assets.
*/

namespace kinski
{
    class MovieController 
    {
    private:
        
        struct Impl;
        std::unique_ptr<Impl> m_impl;
        
    public:
        MovieController();
        virtual ~MovieController();
        
        void load(const std::string &filePath);
        void play();
        bool isPlaying() const;
        void stop();
        void pause();
        void seek_to_time(float value);
        double duration() const;
        double current_time() const;
        float volume() const;
        void set_volume(float volume);
        void set_loop(bool b);
        void set_rate(float r);
        bool loop() const;
        const std::string& get_path() const;
        
        /*!
         * upload the current frame to a gl::Texture object
         * return: true if a new frame could be successfully uploaded,
         * false otherwise
         */
        bool copy_frame_to_texture(gl::Texture &tex);
    };
    
    typedef std::shared_ptr<MovieController> MovieControllerPtr;
}

#endif
