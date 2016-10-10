// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Image
//
//  Created by Croc Dialer on 11/09/16.

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.inl"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.inl"

#include "core/file_functions.hpp"
#include "Image.hpp"

namespace kinski
{
    ImagePtr create_image_from_file(const std::string &the_path, int num_channels)
    {
        std::vector<uint8_t> dataVec;
        ImagePtr ret;
        
        try
        {
            dataVec = fs::read_binary_file(the_path);
            ret = create_image_from_data(dataVec, num_channels);
        }
        catch (Exception &e){ LOG_WARNING << e.what(); }
        return ret;
    }
    
    ImagePtr create_image_from_data(const std::vector<uint8_t> &the_data, int num_channels)
    {
        int width, height, num_components;
        unsigned char *data = stbi_load_from_memory(&the_data[0], the_data.size(),
                                                    &width, &height, &num_components, num_channels);
        
        if(!data) throw ImageLoadException();
        
        LOG_TRACE << "decoded image: " << width << " x " << height << " (" <<num_components<<" ch)";
        
        // ... process data if not NULL ...
        // ... x = width, y = height, n = # 8-bit components per pixel ...
        // ... replace '0' with '1'..'4' to force that many components per pixel
        // ... but 'n' will always be the number that it would have been if you said 0
        
        auto ret = Image::create(data, height, width, num_components);
        STBI_FREE(data);
        return ret;
    }
    
    void copy_image(const ImagePtr &src_mat, ImagePtr &dst_mat)
    {
        uint32_t bytes_per_pixel = 1;
        
        assert(src_mat->roi.x2 <= src_mat->width && src_mat->roi.y2 <= src_mat->height);
        assert(dst_mat->roi.x2 <= dst_mat->width && dst_mat->roi.y2 <= dst_mat->height);
        assert(src_mat->roi.width() == src_mat->roi.width() && src_mat->roi.height() == src_mat->roi.height());
        
        uint32_t src_row_offset = src_mat->width - src_mat->roi.width();
        uint32_t dst_row_offset = dst_mat->width - dst_mat->roi.width();
        
        const uint8_t* src_area_start = src_mat->data + (src_mat->roi.y1 * src_mat->width + src_mat->roi.x1) * bytes_per_pixel;
        uint8_t* dst_area_start = dst_mat->data + (dst_mat->roi.y1 * dst_mat->width + dst_mat->roi.x1) * bytes_per_pixel;
        
        for (uint32_t r = 0; r < src_mat->roi.height(); r++)
        {
            const uint8_t* src_row_start = src_area_start + r * (src_mat->roi.width() + src_row_offset) * bytes_per_pixel;
            uint8_t* dst_row_start = dst_area_start + r * (dst_mat->roi.width() + dst_row_offset) * bytes_per_pixel;
            for (uint32_t c = 0; c < src_mat->roi.width(); c++)
            {
                dst_row_start[c * bytes_per_pixel] = src_row_start[c * bytes_per_pixel];
            }
        }
    }
    
    bool save_image_to_file(const ImagePtr &the_img, const std::string &the_path)
    {
        return fs::write_file(the_path, encode_png(the_img));
    }
    
    std::vector<uint8_t> encode_png(const ImagePtr &the_img)
    {
        
        int num_bytes = 0;
        uint8_t* encoded_data = stbi_write_png_to_mem(the_img->data, 0, the_img->width, the_img->height,
                                                      the_img->bytes_per_pixel, &num_bytes);
        auto ret = std::vector<uint8_t>(encoded_data, encoded_data + num_bytes);
        STBI_FREE(encoded_data);
        return ret;
    }
    
    Image::Image(uint8_t* theData, uint32_t theheight, uint32_t thewidth, uint32_t theBytesPerPixel,
                 bool not_dispose):
    data(theData), height(theheight), width(thewidth), bytes_per_pixel(theBytesPerPixel), do_not_dispose(not_dispose)
    {
        if(!do_not_dispose)
        {
            size_t num_bytes = height * width * bytes_per_pixel;
            data = new uint8_t[num_bytes];
            memcpy(data, theData, num_bytes);
        }
    };
    
    Image::Image(uint32_t theheight, uint32_t thewidth, uint32_t theBytesPerPixel):
    data(new uint8_t[thewidth * theheight * theBytesPerPixel]()),
    height(theheight),
    width(thewidth),
    bytes_per_pixel(theBytesPerPixel),
    do_not_dispose(false)
    {
    
    }
    
    Image::~Image()
    {
        if(data && !do_not_dispose)
        {
            LOG_TRACE_2 << "disposing image";
            delete[](data);
        }
    };
    
    void Image::flip()
    {
        size_t line_offset = width * bytes_per_pixel;
        size_t total_bytes = width * height * bytes_per_pixel;
        
        // swap lines
        for(uint32_t i = 0; i < height / 2; i++)
        {
            std::swap_ranges(data + line_offset * i, data +  line_offset * (i + 1),
                             data + total_bytes - line_offset * (i + 1));
        }
    }
    
    ImagePtr Image::resize(uint32_t the_width, uint32_t the_height)
    {
        float scale_x = width / the_width, scale_y = height / the_height;
        ImagePtr ret = Image::create(the_width, the_height);
        
        // for all components in all pixels, calcalute new value
        for(uint32_t y = 0; y < the_height; ++y)
        {
            for(uint32_t x = 0; x < the_width; ++x)
            {
                float src_x = x * scale_x, src_y = y * scale_y;
                uint8_t* src_ptr = at(src_x, src_y);
                uint8_t* dst_ptr = ret->at(x, y);
                
                for(uint32_t c = 0; c < bytes_per_pixel; ++c){ dst_ptr[c] = src_ptr[c]; }
            }
        }
        return ret;
    }
    
    void Image::convolve(const std::vector<float> &the_kernel)
    {
//        for(uint32_t i = 0; i < height / 2; i++)
    }
}
