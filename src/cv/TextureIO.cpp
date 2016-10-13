// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "gl/Buffer.hpp"
#include "core/file_functions.hpp"
#include "core/Image.hpp"
#include "TextureIO.h"

using namespace std;
using namespace cv;

namespace kinski{ namespace gl{
    
    
const Texture TextureIO::loadTexture(const string &imgName)
{
    string imgPath = fs::search_file(imgName);
    Mat img = imread(imgPath , -1);
    Texture ret;
    
    if(img.empty())
        throw ImageLoadException();
   
    //GLint maxSize;
    //glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    //LOG_INFO<<"maximum texture size: "<< maxSize;
    
#ifdef KINSKI_GLES
    if(img.channels() == 3) 
        cv::cvtColor(img, img, CV_BGR2RGBA);
#endif

    LOG_TRACE<<"loaded image '"<<fs::search_file(imgPath)<<"': "<<img.cols<<" x "<<img.rows<<" -- "
        <<img.channels()<<" channel(s)";

    updateTexture(ret, img);

    return ret;
}

bool TextureIO::saveTexture(const string &imgPath, const Texture &texture, bool flip)
{
    Mat outMat(texture.height(), texture.width(), CV_8UC4);
    texture.bind(); 
    glGetTexImage( texture.target(), 0, GL_BGRA, GL_UNSIGNED_BYTE, outMat.data );
    
    if(flip){cv::flip(outMat, outMat, 0);}
    return imwrite(imgPath, outMat);
}

bool TextureIO::encode_jpg(const gl::Texture &texture, std::vector<uint8_t> &out_bytes)
{
    static gl::Buffer pixel_buf(GL_PIXEL_PACK_BUFFER, GL_STATIC_COPY);
    try
    {
        Mat mat_uncompressed(texture.height(), texture.width(), CV_8UC4), mat_jpg;
        
        size_t num_bytes = texture.width() * texture.height() * 4;
        if(pixel_buf.num_bytes() != num_bytes)
        {
            pixel_buf.set_data(nullptr, num_bytes);
        }
        pixel_buf.bind();
        glGetTexImage(texture.target(), 0, texture.internal_format(), GL_UNSIGNED_BYTE, nullptr);
        pixel_buf.unbind();
        auto pix_ptr = pixel_buf.map();
        memcpy(mat_uncompressed.data, pix_ptr, num_bytes);
        pixel_buf.unmap();
        if(texture.flipped()){ cv::flip(mat_uncompressed, mat_uncompressed, 0); }
        cv::imencode(".jpg", mat_uncompressed, out_bytes);
    } catch (std::exception &e)
    {
        LOG_ERROR << e.what();
        return false;
    }
    
    return true;
}
    
void TextureIO::updateTexture(Texture &theTexture, const Mat &theImage, bool compress)
{
    GLenum format = 0, internal_format = 0;
    
    switch(theImage.channels()) 
	{
#ifdef KINSKI_GLES
		case 1:
            format = GL_LUMINANCE;
			break;
		case 2:
            format = GL_LUMINANCE_ALPHA;
			break;
		case 3:
			format = GL_RGB;
			break;
        case 4:
			format = GL_RGBA;
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
            format = GL_BGR;
            internal_format = compress? GL_COMPRESSED_RGB_S3TC_DXT1_EXT : GL_RGBA;
            break;
        case 4:
            format = GL_BGRA;
            internal_format = compress? GL_COMPRESSED_RGBA_S3TC_DXT5_EXT : GL_RGBA;
        default:
            break;
#endif
	}
    GLenum dataType = (theImage.type() == CV_32FC(theImage.channels())) ? GL_FLOAT : GL_UNSIGNED_BYTE;

    theTexture.update(theImage.data, dataType, 
                      format, theImage.cols, theImage.rows, true);
    
}
}}//namespace
