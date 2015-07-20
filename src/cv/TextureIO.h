// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef _TEXTUREIO_IS_INCLUDED_
#define _TEXTUREIO_IS_INCLUDED_

#include "Texture.h"
#include "opencv2/opencv.hpp"

namespace kinski{ namespace gl{

    class TextureIO 
    {
    private:
        TextureIO();
        
    public:
        
        static const gl::Texture loadTexture(const std::string &imgPath);
        static bool saveTexture(const std::string &imgPath, const gl::Texture &texture,
                                bool flip = false);
        static bool encode_jpg(const gl::Texture &texture, std::vector<uint8_t> &out_bytes);
        
        static void updateTexture(gl::Texture &theTexture,
                                  const cv::Mat &theImage,
                                  bool compress = false);
    };
}}
#endif //_TEXTUREIO_IS_INCLUDED_