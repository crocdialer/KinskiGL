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
        
        auto ret = Image::create(data, width, height, num_components);
        STBI_FREE(data);
        return ret;
    }
    
    void copy_image(const ImagePtr &src_mat, ImagePtr &dst_mat)
    {
        uint32_t bytes_per_pixel = 1;
        
        assert(src_mat->roi.x1 <= src_mat->width && src_mat->roi.y1 <= src_mat->height);
        assert(dst_mat->roi.x1 <= dst_mat->width && dst_mat->roi.y1 <= dst_mat->height);
        assert(src_mat->roi.width() == src_mat->roi.width() && src_mat->roi.height() == src_mat->roi.height());
        
        uint32_t src_row_offset = src_mat->width - src_mat->roi.width();
        uint32_t dst_row_offset = dst_mat->width - dst_mat->roi.width();
        
        const uint8_t* src_area_start = src_mat->data + (src_mat->roi.y0 * src_mat->width + src_mat->roi.x0) * bytes_per_pixel;
        uint8_t* dst_area_start = dst_mat->data + (dst_mat->roi.y0 * dst_mat->width + dst_mat->roi.x0) * bytes_per_pixel;
        
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
    
    Image::Image(uint8_t* the_data, uint32_t the_width, uint32_t the_height,
                 uint32_t the_bytes_per_pixel, bool not_dispose):
    data(the_data),
    width(the_width),
    height(the_height),
    bytes_per_pixel(the_bytes_per_pixel),
    roi(Area_<uint32_t>(0, 0, the_width - 1, the_height - 1)),
    do_not_dispose(not_dispose)
    {
        if(!do_not_dispose)
        {
            size_t num_bytes = height * width * bytes_per_pixel;
            data = new uint8_t[num_bytes];
            memcpy(data, the_data, num_bytes);
        }
    };
    
    Image::Image(uint32_t the_width, uint32_t the_height, uint32_t the_bytes_per_pixel):
    data(new uint8_t[the_width * the_height * the_bytes_per_pixel]()),
    width(the_width),
    height(the_height),
    bytes_per_pixel(the_bytes_per_pixel),
    roi(Area_<uint32_t>(0, 0, the_width - 1, the_height - 1)),
    do_not_dispose(false)
    {
    
    }
    
    Image::Image(const Image &the_other):
    data(new uint8_t[the_other.width * the_other.height * the_other.bytes_per_pixel]),
    width(the_other.width),
    height(the_other.height),
    bytes_per_pixel(the_other.bytes_per_pixel),
    roi(the_other.roi),
    do_not_dispose(the_other.do_not_dispose)
    {
        memcpy(data, the_other.data, width * height * bytes_per_pixel);
    }
    
    Image::Image(Image &&the_other):
    data(the_other.data),
    width(the_other.width),
    height(the_other.height),
    bytes_per_pixel(the_other.bytes_per_pixel),
    roi(the_other.roi),
    do_not_dispose(the_other.do_not_dispose)
    {
        the_other.data = nullptr;
    }
    
    Image& Image::operator=(Image the_other)
    {
        std::swap(data, the_other.data);
        std::swap(height, the_other.height);
        std::swap(width, the_other.width);
        std::swap(bytes_per_pixel, the_other.bytes_per_pixel);
        std::swap(roi, the_other.roi);
        std::swap(do_not_dispose, the_other.do_not_dispose);
        return *this;
    }
    
    Image::~Image()
    {
        if(!do_not_dispose){ delete[](data); }
    }
    
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
    
    ImagePtr Image::blur()
    {
        // gaussian blur
        const std::vector<float> gaussian =
        {
            1, 4, 7, 4, 1,
            4, 16, 26, 16, 4,
            7, 26, 41, 26, 7,
            4, 16, 26, 16, 4,
            1, 4, 7, 4, 1,
        };
//        const std::vector<float> identity =
//        {
//            0, 0, 0,
//            0, 1, 0,
//            0, 0, 0
//        };
        return convolve(gaussian);
    }
    
    ImagePtr Image::resize(uint32_t the_width, uint32_t the_height)
    {
        ImagePtr ret = Image::create(the_width, the_height, bytes_per_pixel);
        float scale_x = the_width / (float)width, scale_y = the_height / (float)height;
        
        // blur
        ImagePtr blur_img = blur();
        
        // for all components in all pixels, calculate new value
        for(uint32_t y = 0; y < the_height; ++y)
        {
            for(uint32_t x = 0; x < the_width; ++x)
            {
                float src_x = x / scale_x, src_y = y / scale_y;
                uint8_t* dst_ptr = ret->at(x, y);
                
                // nearest neighbour
                uint8_t* src_ptr = blur_img->at(roundf(src_x), roundf(src_y));
                for(uint32_t c = 0; c < bytes_per_pixel; ++c){ dst_ptr[c] = src_ptr[c]; }
            }
        }
        return ret;
    }
    
    ImagePtr Image::convolve(const std::vector<float> &the_kernel)
    {
        ImagePtr ret;
        auto norm_kernel = the_kernel;
        float kernel_sum = sum(the_kernel);
        for(auto &e : norm_kernel){ e /= kernel_sum; }
        
        int kernel_dim = sqrt(the_kernel.size());
        int kernel_dim_2 = kernel_dim / 2;
        
        if(kernel_dim * kernel_dim != the_kernel.size())
        {
            LOG_WARNING << "only quadratic kernels accepted";
            return ret;
        }
        ret = Image::create(width, height, bytes_per_pixel);
        
        for(uint32_t y = 0; y < height; ++y)
        {
            for(uint32_t x = 0; x < width; ++x)
            {
                uint8_t* dst_ptr = ret->at(x, y);
                
                for(uint32_t c = 0; c < bytes_per_pixel; ++c)
                {
                    float sum = 0;
                    int k_idx = 0;
                    for(int k = -kernel_dim_2; k <= kernel_dim_2; ++k)
                    {
                        for(int l = -kernel_dim_2; l <= kernel_dim_2; ++l, ++k_idx)
                        {
                            int pos_x = x + k, pos_y = y + l;
                            if(pos_x < 0 || pos_x >= (int)width || pos_y < 0 || pos_y >= (int)height)
                            { sum += at(x, y)[c] / (float)norm_kernel.size(); }
                            else{ sum += at(pos_x, pos_y)[c] * norm_kernel[k_idx]; }
                        }
                    }
                    dst_ptr[c] = clamp<float>(roundf(sum), 0, 255);
                }
            }
        }
        return ret;
    }
}
