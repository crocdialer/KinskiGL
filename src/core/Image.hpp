// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Timer.h
//
//  Created by Croc Dialer on 11/09/16.

#pragma once

#include "core/core.hpp"

namespace kinski
{
    DEFINE_CLASS_PTR(Image);
    
    KINSKI_API ImagePtr create_image_from_file(const std::string &the_path, int num_channels = 0);
    KINSKI_API ImagePtr create_image_from_data(const std::vector<uint8_t> &the_data, int num_channels = 0);
    KINSKI_API void copy_image(const ImagePtr &src_img, ImagePtr &dst_img);
    KINSKI_API bool save_image_to_file(const ImagePtr &the_img, const std::string &the_path);
    KINSKI_API std::vector<uint8_t> encode_png(const ImagePtr &the_img);
    
    class Image
    {
    public:
        
        uint8_t* data = nullptr;
        uint32_t width = 0, height = 0;
        uint32_t bytes_per_pixel = 1;
        Area_<uint32_t> roi;
        bool do_not_dispose = false;
        
        static ImagePtr create(uint8_t* theData, uint32_t the_width, uint32_t the_height,
                               uint32_t theBytesPerPixel = 1, bool not_dispose = false)
        {
            return ImagePtr(new Image(theData, the_width, the_height, theBytesPerPixel, not_dispose));
        };
        
        static ImagePtr create(uint32_t the_width, uint32_t the_height, uint32_t theBytesPerPixel = 1)
        {
            return ImagePtr(new Image(the_width, the_height, theBytesPerPixel));
        };
        
        inline uint8_t* at(uint32_t x, uint32_t y) const
        { return data + (x + y * width) * bytes_per_pixel; };
        
        inline uint8_t* data_start_for_roi() const {return data + (roi.y0 * width + roi.x0) * bytes_per_pixel;}
        
        inline size_t num_bytes() const { return height * width * bytes_per_pixel; }
        
        ImagePtr resize(uint32_t the_width, uint32_t the_height);
        void convolve(const std::vector<float> &the_kernel);
        
        void flip();
        
        Image(const Image &the_other);
        Image(Image &&the_other);
        Image& operator=(Image the_other);
        ~Image();
        
    private:
        
        Image(uint8_t* theData,
              uint32_t the_width,
              uint32_t the_height,
              uint32_t theBytesPerPixel = 1,
              bool not_dispose = false);
        
        Image(uint32_t the_width,
              uint32_t the_height,
              uint32_t theBytesPerPixel = 1);
    };
    
    class ImageLoadException : public Exception
    {
    public:
        ImageLoadException()
        :Exception("Got trouble decoding image file"){};
    };
}
