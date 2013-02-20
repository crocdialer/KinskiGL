// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "TextureIO.h"

using namespace std;
using namespace cv;

namespace kinski{ namespace gl{
    
    
const Texture TextureIO::loadTexture(const string &imgName)
{
    string imgPath = searchFile(imgName);
    Mat img = imread(imgPath , -1);
    Texture ret;
    
    if(img.empty())
        throw ImageLoadException(imgName);
   
    //GLint maxSize;
    //glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    //LOG_INFO<<"maximum texture size: "<< maxSize;
    
#ifdef KINSKI_GLES
    if(img.channels() == 3) 
        cv::cvtColor(img, img, CV_BGR2RGBA);
#endif

    LOG_TRACE<<"loaded image '"<<searchFile(imgPath)<<"': "<<img.cols<<" x "<<img.rows<<" -- "
        <<img.channels()<<" channel(s)";

    updateTexture(ret, img);

    return ret;
}

bool 
TextureIO::saveTexture(const string &imgPath, const Texture &texture)
{
    Mat outMat;
    //TODO: read back data from Textureobject, query format and create cv::Mat
    return imwrite(imgPath, outMat);
}

void TextureIO::updateTexture(Texture &theTexture, const Mat &theImage)
{
    GLenum format=0;
    
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
			break;
		case 2:
            format = GL_RG;
			break;
		case 3:
			format = GL_BGR;
			break;
        case 4:
			format =  GL_BGRA;
		default:
			break;
#endif
	}
    // requires OpenGL 3.3
    //GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
    //glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    
    GLenum dataType = (theImage.type() & CV_32F) ? GL_FLOAT : GL_UNSIGNED_BYTE;

    theTexture.update(theImage.data, dataType, 
                      format, theImage.cols, theImage.rows, true);
    
}
}}//namespace
