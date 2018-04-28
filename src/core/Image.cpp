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
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "core/file_functions.hpp"
#include "Image.hpp"

namespace kinski
{
    namespace
    {
        void stbi_write_func(void *context, void *data, int size)
        {
            std::vector<uint8_t> &out = *reinterpret_cast<std::vector<uint8_t>*>(context);
            out.insert(out.end(), (uint8_t*)data, (uint8_t*)data + size);
        }
    }
    
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
        return create_image_from_data(the_data.data(), the_data.size(), num_channels);
    }

    ImagePtr create_image_from_data(const uint8_t *the_data, size_t the_num_bytes, int num_channels)
    {
        int width, height, num_components;
        unsigned char *data = stbi_load_from_memory(the_data, the_num_bytes, &width, &height, &num_components,
                                                    num_channels);

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
        if(!src_mat){ return; }
        if(!dst_mat){ dst_mat = Image::create(src_mat->width, src_mat->height, src_mat->num_components()); }
        uint32_t bytes_per_pixel = src_mat->num_components();
        
        assert(src_mat->roi.width() <= src_mat->width && src_mat->roi.height() <= src_mat->height);
        assert(dst_mat->roi.width() <= dst_mat->width && dst_mat->roi.height() <= dst_mat->height);
        assert(src_mat->roi.width() == src_mat->roi.width() && src_mat->roi.height() == src_mat->roi.height());
        
        uint32_t src_row_offset = src_mat->width - src_mat->roi.width();
        uint32_t dst_row_offset = dst_mat->width - dst_mat->roi.width();
        
        const uint8_t* src_area_start = src_mat->data + (src_mat->roi.y0 * src_mat->width + src_mat->roi.x0) * bytes_per_pixel;
        uint8_t* dst_area_start = dst_mat->data + (dst_mat->roi.y0 * dst_mat->width + dst_mat->roi.x0) * bytes_per_pixel;
        
        for(uint32_t r = 0; r < src_mat->roi.height(); r++)
        {
            const uint8_t* src_row_start = src_area_start + r * (src_mat->roi.width() + src_row_offset) * bytes_per_pixel;
            uint8_t* dst_row_start = dst_area_start + r * (dst_mat->roi.width() + dst_row_offset) * bytes_per_pixel;
            
            for(uint32_t c = 0; c < src_mat->roi.width(); c++)
            {
                for(uint32_t ch = 0; ch < bytes_per_pixel; ch++)
                    dst_row_start[c * bytes_per_pixel + ch] = src_row_start[c * bytes_per_pixel + ch];
            }
        }
    }
    
    bool save_image_to_file(const ImagePtr &the_img, const std::string &the_path)
    {
        return fs::write_file(the_path, encode_png(the_img));
    }
    
    std::vector<uint8_t> encode_png(const ImagePtr &the_img)
    {
        auto ret = std::vector<uint8_t>();
        stbi_write_png_to_func(&stbi_write_func, &ret, the_img->width, the_img->height,
                               the_img->m_num_components, the_img->data, 0);
        return ret;
    }
    
    std::vector<uint8_t> encode_jpg(const ImagePtr &the_img)
    {
        auto ret = std::vector<uint8_t>();
        stbi_write_jpg_to_func(&stbi_write_func, &ret, the_img->width, the_img->height,
                               the_img->m_num_components, the_img->data, 83);
        return ret;
    }

    template<class T>
    Image_<T>::Image_(T* the_data, uint32_t the_width, uint32_t the_height,
                      uint32_t the_num_components, bool not_dispose):
    data(the_data),
    width(the_width),
    height(the_height),
    m_num_components(the_num_components),
    roi(Area_<uint32_t>(0, 0, the_width - 1, the_height - 1)),
    do_not_dispose(not_dispose)
    {
        if(!do_not_dispose)
        {
            size_t num_bytes = height * width * m_num_components;
            data = new T[num_bytes];
            memcpy(data, the_data, num_bytes);
        }
    };

    template<class T>
    Image_<T>::Image_(uint32_t the_width, uint32_t the_height, uint32_t the_num_components):
    data(new T[the_width * the_height * the_num_components]()),
    width(the_width),
    height(the_height),
    m_num_components(the_num_components),
    roi(Area_<uint32_t>(0, 0, the_width - 1, the_height - 1)),
    do_not_dispose(false),
    m_type(Type::UNKNOWN)
    {
    
    }

    template<class T>
    Image_<T>::Image_(const Image_<T> &the_other):
    data(new T[the_other.width * the_other.height * the_other.m_num_components]),
    width(the_other.width),
    height(the_other.height),
    m_num_components(the_other.m_num_components),
    roi(the_other.roi),
    do_not_dispose(the_other.do_not_dispose),
    m_type(the_other.m_type)
    {
        memcpy(data, the_other.data, width * height * m_num_components);
    }

    template<class T>
    Image_<T>::Image_(Image_<T> &&the_other):
    data(the_other.data),
    width(the_other.width),
    height(the_other.height),
    m_num_components(the_other.m_num_components),
    roi(the_other.roi),
    do_not_dispose(the_other.do_not_dispose),
    m_type(the_other.m_type)
    {
        the_other.data = nullptr;
    }

    template<class T>
    Image_<T>& Image_<T>::operator=(Image_<T> the_other)
    {
        std::swap(data, the_other.data);
        std::swap(height, the_other.height);
        std::swap(width, the_other.width);
        std::swap(m_num_components, the_other.m_num_components);
        std::swap(roi, the_other.roi);
        std::swap(do_not_dispose, the_other.do_not_dispose);
        std::swap(m_type, the_other.m_type);
        return *this;
    }

    template<class T>
    Image_<T>::~Image_()
    {
        if(!do_not_dispose){ delete[](data); }
    }

    template<class T>
    void Image_<T>::flip(bool horizontal)
    {
        size_t line_offset = width * m_num_components * sizeof(T);
        size_t total_bytes = num_bytes();
        
        if(horizontal)
        {
            // swap lines
            for(uint32_t r = 0; r < height; r++)
            {
                for(uint32_t c = 0; c < width / 2; c++)
                {
                    T* lhs = data + line_offset * r + m_num_components * c;
                    T* rhs = data + line_offset * (r + 1) - m_num_components * (c + 1);
                    std::swap_ranges(lhs, lhs + m_num_components, rhs);
                }
            }
        }
        else
        {
            // swap lines
            for(uint32_t i = 0; i < height / 2; i++)
            {
                std::swap_ranges(data + line_offset * i, data +  line_offset * (i + 1),
                                 data + total_bytes - line_offset * (i + 1));
            }
        }
    }

    template<class T>
    typename Image_<T>::Ptr Image_<T>::blur()
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
        return convolve(gaussian);
    }

    template<class T>
    typename Image_<T>::Ptr Image_<T>::resize(uint32_t the_width, uint32_t the_height, uint32_t the_num_channels)
    {
        if(!the_num_channels){ the_num_channels = m_num_components; };

        Image_<T>::Ptr ret = Image_<T>::create(the_width, the_height, the_num_channels);
        float scale_x = the_width / (float)width, scale_y = the_height / (float)height;
        
        // blur
//        ImagePtr blur_img = blur();
        
        // for all components in all pixels, calculate new value
        for(uint32_t y = 0; y < the_height; ++y)
        {
            for(uint32_t x = 0; x < the_width; ++x)
            {
                float src_x = x / scale_x, src_y = y / scale_y;
                T* dst_ptr = ret->at(x, y);
                
                // nearest neighbour
                T* src_ptr = at(roundf(src_x), roundf(src_y));
                
                if(the_num_channels <= m_num_components)
                {
                    for(uint32_t c = 0; c < the_num_channels; ++c){ dst_ptr[c] = src_ptr[c]; }
                }
                else
                {
                    for(uint32_t c = 0; c < the_num_channels; ++c)
                    {
                        dst_ptr[c] = c > m_num_components ? 0 : src_ptr[c];
                    }
                }
            }
        }
        return ret;
    }

    template<class T>
    typename Image_<T>::Ptr Image_<T>::convolve(const std::vector<float> &the_kernel)
    {
        Image_<T>::Ptr ret;
        auto norm_kernel = the_kernel;
        float kernel_sum = sum(the_kernel);
        for(auto &e : norm_kernel){ e /= kernel_sum; }
        
        int kernel_dim = sqrt(the_kernel.size());
        int kernel_dim_2 = kernel_dim / 2;
        
        if(kernel_dim * kernel_dim != (int)the_kernel.size())
        {
            LOG_WARNING << "only quadratic kernels accepted";
            return ret;
        }
        ret = Image_<T>::create(width, height, m_num_components);
        
        for(uint32_t y = 0; y < height; ++y)
        {
            for(uint32_t x = 0; x < width; ++x)
            {
                T* dst_ptr = ret->at(x, y);
                
                for(uint32_t c = 0; c < m_num_components; ++c)
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

    template<class T>
    void Image_<T>::offsets(uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a) const
    {
        switch(m_type)
        {
            case Type::BGR:
                *r = 2 * sizeof(T); *g = 1 * sizeof(T); *b = 0; if(a){ *a = 0; }
                break;

            case Type::RGB:
                *r = 0; *g = 1 * sizeof(T); *b = 2 * sizeof(T); if(a){ *a = 0; }
                break;

            case Type::RGBA:
                *r = 0; *g = 1 * sizeof(T); *b = 2 * sizeof(T); if(a){ *a = 3 * sizeof(T); }
                break;

            case Type::BGRA:
                *r = 2 * sizeof(T); *g = 1 * sizeof(T); *b = 0; if(a){ *a = 3 * sizeof(T); }
                break;

            case Type::GRAY:
            case Type::UNKNOWN:
            default:
                *r = *g = *b = 0; if(a){ *a = 0; }
                break;
        }
    }

    // create class-template instantiations
    template class Image_<uint8_t>;
    template class Image_<float>;

    template<typename T> struct Point_
    {
        T x, y;
        Point_():x(0), y(0){}
        Point_(const T &lhs, const T &rhs):x(lhs), y(rhs){}
        Point_(const T &lhs):x(lhs), y(lhs){}
    };
    typedef Point_<float> Point;
    
    template<typename T> float length(const Point_<T> &the_point)
    {
        return sqrtf(the_point.x * the_point.x + the_point.y * the_point.y);
    };
    
    template<typename T> float length2(const Point_<T> &the_point)
    {
        return the_point.x * the_point.x + the_point.y * the_point.y;
    };
    
    template <typename T>
    class Grid_
    {
    public:
        
        static inline constexpr T zero(){ return T(0); }
        static inline constexpr T inf(){ return T(1 << 16); }
        
        Grid_(uint32_t the_width, uint32_t the_height):
        m_width(the_width),
        m_height(the_height),
        m_data(new T[the_width * the_height]()){}
        
        Grid_(const Grid_ &the_other):
        m_width(the_other.m_width),
        m_height(the_other.m_height),
        m_data(new T[m_width * m_height])
        {
            memcpy(m_data, the_other.m_data, m_width * m_height * sizeof(T));
        }
        
        Grid_(Grid_&& the_other):
        m_width(the_other.m_width),
        m_height(the_other.m_height),
        m_data(the_other.m_data)
        {
            the_other.m_data = nullptr;
        }
        
        Grid_& operator=(Grid_ the_other)
        {
            std::swap(m_width, the_other.m_width);
            std::swap(m_height, the_other.m_height);
            std::swap(m_data, the_other.m_data);
            return *this;
        }
        ~Grid_(){ delete[] m_data; }
        
        inline const T at(uint32_t x, uint32_t y) const
        {
            if(x >= 0 && y >= 0 && x < m_width && y < m_height)
                return *(m_data + x + m_width * y);
            else
                return inf();
        }
        
        inline void set(uint32_t x, uint32_t y, const T& the_value)
        {
            *(m_data + x + m_width * y) = the_value;
        }
        
        void compare(T& the_point, uint32_t x, uint32_t y, int32_t offset_x, int32_t offset_y)
        {
            T other = at(x + offset_x, y + offset_y);
            other.x += offset_x;
            other.y += offset_y;
            
            if(length2(other) < length2(the_point)){ the_point = other; }
        }
        
        void compute_distances()
        {
            // 1st pass
            for (uint32_t y = 0; y < m_height ; ++y)
            {
                for (uint32_t x = 0; x < m_width; ++x)
                {
                    T p = at(x, y);
                    compare(p, x, y, -1,  0);
                    compare(p, x, y,  0, -1);
                    compare(p, x, y, -1, -1);
                    compare(p, x, y,  1, -1);
                    set(x, y, p);
                }
                
                for (int x = m_width - 1; x >= 0; --x)
                {
                    T p = at(x, y);
                    compare(p, x, y, 1, 0);
                    set(x, y, p);
                }
            }
            
            // 2nd pass
            for (int y = m_height - 1; y >=0 ; --y)
            {
                for (int x = m_width - 1; x >= 0; --x)
                {
                    T p = at(x, y);
                    compare(p, x, y,  1,  0);
                    compare(p, x, y,  0,  1);
                    compare(p, x, y, -1,  1);
                    compare(p, x, y,  1,  1);
                    set(x, y, p);
                }
                
                for (uint32_t x = 0; x < m_width; ++x)
                {
                    T p = at(x, y);
                    compare(p, x, y, -1, 0);
                    set(x, y, p);
                }
            }
        }
        
        T* data(){ return m_data; }
        size_t size(){ return m_width * m_height; }
        
    private:
        uint32_t m_width, m_height;
        T* m_data;
    };
    typedef Grid_<Point> Grid;
    
    ImagePtr compute_distance_field(ImagePtr the_img, float spread)
    {
        if(!the_img || the_img->m_num_components > 1){ return nullptr; }
        
        // create two grids
        Grid grid1(the_img->width, the_img->height), grid2(the_img->width, the_img->height);
        
        // init grids
        for (uint32_t x = 0; x < the_img->width; ++x)
            for (uint32_t y = 0; y < the_img->height; ++y)
            {
                bool is_inside = *the_img->at(x, y) > 32;
                grid1.set(x, y, is_inside ? Grid::inf() : Grid::zero());
                grid2.set(x, y, is_inside ? Grid::zero() : Grid::inf());
            }
        
        // run propagation algorithm
        grid1.compute_distances();
        grid2.compute_distances();
        
        // gather result
        ImagePtr ret = Image::create(the_img->width, the_img->height);
        
        for(uint32_t y = 0; y < ret->height; ++y)
            for(uint32_t x = 0; x < ret->width; ++x)
            {
                // Calculate the actual distance from the dx/dy
                float dist1 = length(grid1.at(x, y));
                float dist2 = length(grid2.at(x, y));
                float dist = dist2 - dist1;
                
                // quantize distance
                *ret->at(x, y) = roundf(map_value<float>(dist, 3 * spread, -spread, 0, 255));
            }
        return ret;
    }
}
