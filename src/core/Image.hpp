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
    KINSKI_API ImagePtr create_image_from_file(const std::string &the_path, int num_channels = 0);
    KINSKI_API ImagePtr create_image_from_data(const std::vector<uint8_t> &the_data, int num_channels = 0);
    KINSKI_API void copy_image(const ImagePtr &src_img, ImagePtr &dst_img);
    
    class Image
    {
    public:
        
        uint8_t* data = nullptr;
        uint32_t rows = 0, cols = 0;
        uint32_t bytes_per_pixel = 1;
        Area<uint32_t> roi;
        bool do_not_dispose = false;
        
        static ImagePtr create(uint8_t* theData, uint32_t theRows, uint32_t theCols, uint32_t theBytesPerPixel = 1,
                               bool not_dispose = false)
        {
            return ImagePtr(new Image(theData, theRows, theCols, theBytesPerPixel, not_dispose));
        };
        
        inline uint8_t* data_start_for_roi() const {return data + (roi.y1 * cols + roi.x1) * bytes_per_pixel;}
        
        inline size_t num_bytes() const { return rows * cols * bytes_per_pixel; }
        
        ~Image()
        {
            if(data && !do_not_dispose)
            {
                LOG_TRACE_2 << "disposing image";
                delete[](data);
            }
        };
        
    private:
        
        Image(uint8_t* theData, uint32_t theRows, uint32_t theCols, uint32_t theBytesPerPixel = 1,
              bool not_dispose = false):
        data(theData), rows(theRows), cols(theCols), bytes_per_pixel(theBytesPerPixel), do_not_dispose(not_dispose)
        {
            if(!do_not_dispose)
            {
                size_t num_bytes = rows * cols * bytes_per_pixel;
                data = new uint8_t[num_bytes];
                memcpy(data, theData, num_bytes);
            }
        };
    };
    
    class ImageLoadException : public Exception
    {
    public:
        ImageLoadException()
        :Exception("Got trouble decoding image file"){};
    };
}