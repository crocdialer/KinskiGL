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

DEFINE_CLASS_PTR(Image);

class Image
{
public:
    enum class Type{UNKNOWN = 0, GRAY, RGB, BGR, RGBA, BGRA};

    virtual uint32_t width() const = 0;

    virtual uint32_t height() const = 0;

    virtual uint32_t num_components() const = 0;

    virtual void* data() = 0;

    virtual size_t num_bytes() const = 0;

    virtual Type type() const = 0;

    virtual const Area_<uint32_t>& roi() const = 0;

    virtual void offsets(uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a = nullptr) const = 0;

    virtual ImagePtr resize(uint32_t the_width, uint32_t the_height, uint32_t the_num_channels = 0) = 0;

    //! kernel is interpreted col-major
    virtual ImagePtr convolve(const std::vector<float> &the_kernel) = 0;

    virtual ImagePtr blur() = 0;
    virtual void flip(bool horizontal = false) = 0;
};

template<class T> class Image_ : public Image
{
public:

    using Ptr = std::shared_ptr<Image_<T>>;

    T* m_data = nullptr;
    uint32_t m_width = 0, m_height = 0;
    uint32_t m_num_components = 1;
    Area_<uint32_t> m_roi;
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
    { return m_data + (x + y * m_width) * m_num_components * sizeof(T); };

    inline T* data_start_for_roi() const
    { return m_data + (m_roi.y0 * m_width + m_roi.x0) * m_num_components * sizeof(T);}

    inline size_t num_bytes() const override { return m_height * m_width * m_num_components * sizeof(T); }

    inline const Area_<uint32_t>& roi() const override {return m_roi; }

    inline Type type() const override{ return m_type; };

    ImagePtr resize(uint32_t the_width, uint32_t the_height, uint32_t the_num_channels = 0) override;

    //! kernel is interpreted col-major
    ImagePtr convolve(const std::vector<float> &the_kernel) override;

    ImagePtr blur() override;
    void flip(bool horizontal = false) override;

    void offsets(uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a = nullptr) const override;

    inline uint32_t width() const override{ return m_width; };

    inline uint32_t height() const override{ return m_height; };

    inline void* data(){ return (void*)m_data; };

    inline uint32_t num_components() const override { return m_num_components; } ;

    Image_(const Image_ &the_other);
    Image_(Image_ &&the_other);
    Image_& operator=(Image_ the_other);
    virtual ~Image_();

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

//using Image = Image_<uint8_t>;
//using ImageHDR = Image_<float>;
//using ImagePtr = std::shared_ptr<Image_<uint8_t>>;

KINSKI_API ImagePtr create_image_from_file(const std::string &the_path, int num_channels = 0);
KINSKI_API ImagePtr create_image_from_data(const std::vector<uint8_t> &the_data, int num_channels = 0);
KINSKI_API ImagePtr create_image_from_data(const uint8_t *the_data, size_t the_num_bytes, int num_channels = 0);
template<class T> KINSKI_API void copy_image(const typename Image_<T>::Ptr &src_img,
                                             typename Image_<T>::Ptr &dst_img);
KINSKI_API bool save_image_to_file(const ImagePtr &the_img, const std::string &the_path);
KINSKI_API std::vector<uint8_t> encode_png(const ImagePtr &the_img);
KINSKI_API std::vector<uint8_t> encode_jpg(const ImagePtr &the_img);
KINSKI_API ImagePtr compute_distance_field(ImagePtr the_img, float spread);
}
