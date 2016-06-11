// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  texture_loading.cpp
//
//  Created by Fabian on 14.02.13.

#include "core/file_functions.hpp"
#include "gl/gl.hpp"
#include "Texture.hpp"
#include "Buffer.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.inl"

namespace kinski { namespace gl {
    
    bool isPowerOf2(int v)
    {
        int tmp = 1;
        while (tmp < v) {tmp <<= 1;}
        return tmp == v;
    }
    
    ImagePtr decode_image(const std::vector<uint8_t> &the_data, int num_channels)
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
        
        return Image::create(data, height, width, num_components);
    }
    
    Texture create_texture_from_image(const ImagePtr& the_img, bool mipmap,
                                      bool compress, GLfloat anisotropic_filter_lvl)
    {
        Texture ret;
        
        if(!the_img){ return ret; }
        GLenum format = 0, internal_format = 0;
        
        switch(the_img->bytes_per_pixel)
        {
#ifdef KINSKI_GLES
            case 1:
                internal_format = format = GL_LUMINANCE;
                break;
            case 2:
                internal_format = format = GL_LUMINANCE_ALPHA;
                break;
            case 3:
                format = GL_RGB;
                internal_format = GL_RGB;
                // needs precompressed image and call to glCompressedTexImage2D
//                internal_format = compress ? GL_ETC1_RGB8_OES : GL_RGB;
                break;
            case 4:
                internal_format = format = GL_RGBA;
            default:
                break;
#else
            case 1:
                format = GL_RED;
                internal_format = compress? GL_COMPRESSED_RED_RGTC1 : GL_RGBA;
                break;
            case 2:
                format = GL_RG;
                internal_format = compress? GL_COMPRESSED_RG_RGTC2 : GL_RGBA;
                break;
            case 3:
                format = GL_RGB;
                internal_format = compress? GL_COMPRESSED_RGB_S3TC_DXT1_EXT : GL_RGBA;
                break;
            case 4:
                format = GL_RGBA;
                internal_format = compress? GL_COMPRESSED_RGBA_S3TC_DXT5_EXT : GL_RGBA;
            default:
                break;
#endif
        }
        Texture::Format fmt;
        fmt.setInternalFormat(internal_format);
        
        if(mipmap)
        {
            fmt.set_mipmapping();
            fmt.setMinFilter(GL_LINEAR_MIPMAP_NEAREST);
        }
        uint8_t *data = the_img->data;
        
#if !defined(KINSKI_GLES)
        gl::Buffer pixel_buf;
        pixel_buf.set_data(the_img->data, the_img->cols * the_img->rows * the_img->bytes_per_pixel);
        pixel_buf.bind(GL_PIXEL_UNPACK_BUFFER);
        data = nullptr;
#endif
        ret = Texture(data, format, the_img->cols, the_img->rows, fmt);
        ret.setFlipped();
        KINSKI_CHECK_GL_ERRORS();
        
        ret.set_anisotropic_filter(anisotropic_filter_lvl);
        return ret;
    }
    
    Texture create_texture_from_data(const std::vector<uint8_t> &the_data, bool mipmap,
                                     bool compress, GLfloat anisotropic_filter_lvl)
    {
        ImagePtr img;
        Texture ret;
        try {img = decode_image(the_data);}
        catch (ImageLoadException &e)
        {
            LOG_ERROR << e.what();
            return ret;
        }
        ret = create_texture_from_image(img, mipmap, compress, anisotropic_filter_lvl);
        return ret;
    }
    
    Texture create_texture_from_file(const std::string &theFileName, bool mipmap, bool compress,
                                     GLfloat anisotropic_filter_lvl)
    {
        std::vector<uint8_t> dataVec;
        Texture ret;
        
        try{ dataVec = fs::read_binary_file(theFileName); }
        catch (fs::FileNotFoundException &e)
        {
            LOG_WARNING << e.what();
            return ret;
        }
        ret = create_texture_from_data(dataVec, mipmap, compress, anisotropic_filter_lvl);
        return ret;
    }
}}
