#include "TextureIO.h"

using namespace std;
using namespace cv;

namespace kinski{
    
using namespace gl;
    
const Texture TextureIO::loadTexture(const string &imgPath)
{
    Mat img = imread(imgPath);
    
    GLenum format=0;
	
	switch(img.channels()) 
	{
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
			format = GL_BGRA;
		default:
			break;
	}
    
    // requires OpenGL 3.3
    //GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
    //glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    
    //m_texture.update(img.data, format, img.cols, img.rows, true);
    Texture ret = Texture(img.data, format, img.cols, img.rows);
    ret.setFlipped();
    
    return ret;
}

bool 
TextureIO::saveTexture(const string &imgPath, const Texture &texture)
{
    Mat outMat;
    //TODO: read baack data from Textureobject, query format and create cv::Mat
    return imwrite(imgPath, outMat);
}

void TextureIO::updateTexture(const Texture &theTexture, const Mat &theImage)
{
    
}
}