#pragma once

#include "gl/gl.hpp"

/*
* This class provides capture Input from Decklink Black magic devices
*/

namespace kinski{ namespace decklink{
    
    class Input
    {
    public:
        
        Input();
        virtual ~Input();
        
        std::string name() const { return "pupu"; };
        
        std::vector<std::string> get_displaymode_names() const;
        
        void start_capture();
        void stop_capture();
        
        /*!
         * upload the current frame to the_texture with target GL_TEXTURE_2D
         * return: true if a new frame has been uploaded successfully,
         * false otherwise
         */
        bool copy_frame_to_texture(gl::Texture &the_texture);
        
        /*!
         * copy the current frame to a std::vector<uint8_t>
         * return: true if a new frame has been copied successfully,
         * false otherwise
         */
        bool copy_frame(std::vector<uint8_t>& data, int *width = nullptr, int *height = nullptr);
        
    private:
        
        class Impl;
        std::shared_ptr<Impl> m_impl;
    };
}}// namespaces