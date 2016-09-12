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

#include "core/file_functions.hpp"
#include "Image.hpp"

namespace kinski
{
    KINSKI_API ImagePtr create_image_from_file(const std::string &the_path, int num_channels)
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
        free(data);
        return ret;
    }
    
    void copy_image(const ImagePtr &src_mat, ImagePtr &dst_mat)
    {
        uint32_t bytes_per_pixel = 1;
        
        assert(src_mat->roi.x2 <= src_mat->cols && src_mat->roi.y2 <= src_mat->rows);
        assert(dst_mat->roi.x2 <= dst_mat->cols && dst_mat->roi.y2 <= dst_mat->rows);
        assert(src_mat->roi.width() == src_mat->roi.width() && src_mat->roi.height() == src_mat->roi.height());
        
        uint32_t src_row_offset = src_mat->cols - src_mat->roi.width();
        uint32_t dst_row_offset = dst_mat->cols - dst_mat->roi.width();
        
        const uint8_t* src_area_start = src_mat->data + (src_mat->roi.y1 * src_mat->cols + src_mat->roi.x1) * bytes_per_pixel;
        uint8_t* dst_area_start = dst_mat->data + (dst_mat->roi.y1 * dst_mat->cols + dst_mat->roi.x1) * bytes_per_pixel;
        
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
}