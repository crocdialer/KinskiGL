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
#include "core/Area.hpp"

namespace kinski{

template<class T> class Image_
{
public:

    using Ptr = std::shared_ptr<Image_<T>>;

    enum class Type{UNKNOWN = 0, GRAY, RGB, BGR, RGBA, BGRA};

    T* data = nullptr;
    uint32_t width = 0, height = 0;
    uint32_t m_num_components = 1;
    Area_<uint32_t> roi;
    bool do_not_dispose = false;
    Type m_type = Type::UNKNOWN;

    static Ptr create(T* theData, uint32_t the_width, uint32_t the_height,
                      uint32_t the_num_components = 1, bool not_dispose = false)
    {
        return Ptr(new Image_<T>(theData, the_width, the_height, the_num_components, not_dispose));
    };

    static Ptr create(uint32_t the_width, uint32_t the_height, uint32_t the_num_components = 1)
    {
        return Ptr(new Image_<T>(the_width, the_height, the_num_components));
    };

    inline T* at(uint32_t x, uint32_t y) const
    { return data + (x + y * width) * m_num_components * sizeof(T); };

    inline T* data_start_for_roi() const {return data + (roi.y0 * width + roi.x0) * m_num_components;}

    inline size_t num_bytes() const { return height * width * m_num_components * sizeof(T); }

    Ptr resize(uint32_t the_width, uint32_t the_height, uint32_t the_num_channels = 0);

    //! kernel is interpreted col-major
    Ptr convolve(const std::vector<float> &the_kernel);

    Ptr blur();
    void flip(bool horizontal = false);

    void offsets(uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a = nullptr) const;
    inline uint32_t num_components() const{ return m_num_components; };

    Image_(const Image_ &the_other);
    Image_(Image_ &&the_other);
    Image_& operator=(Image_ the_other);
    ~Image_();

private:

    Image_(T* theData,
           uint32_t the_width,
           uint32_t the_height,
           uint32_t the_num_components = 1,
           bool not_dispose = false);

    Image_(uint32_t the_width,
           uint32_t the_height,
           uint32_t the_num_components = 1);
};

class ImageLoadException : public Exception
{
public:
    ImageLoadException()
    :Exception("Got trouble decoding image file"){};
};

using Image = Image_<uint8_t>;
using ImageHDR = Image_<float>;
using ImagePtr = std::shared_ptr<Image_<uint8_t>>;

KINSKI_API ImagePtr create_image_from_file(const std::string &the_path, int num_channels = 0);
KINSKI_API ImagePtr create_image_from_data(const std::vector<uint8_t> &the_data, int num_channels = 0);
KINSKI_API ImagePtr create_image_from_data(const uint8_t *the_data, size_t the_num_bytes, int num_channels = 0);
KINSKI_API void copy_image(const ImagePtr &src_img, ImagePtr &dst_img);
KINSKI_API bool save_image_to_file(const ImagePtr &the_img, const std::string &the_path);
KINSKI_API std::vector<uint8_t> encode_png(const ImagePtr &the_img);
KINSKI_API std::vector<uint8_t> encode_jpg(const ImagePtr &the_img);
KINSKI_API ImagePtr compute_distance_field(ImagePtr the_img, float spread);
}
