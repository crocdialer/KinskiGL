#include "TextureIO.h"

using namespace std;
using namespace cv;

namespace kinski{ namespace gl{
    
    
const Texture TextureIO::loadTexture(const string &imgPath)
{
    Mat img = imread(searchFile(imgPath) , -1);
    Texture ret;
    
    if(img.empty())
        throw TextureNotFoundException(imgPath);
    
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
    
    GLenum dataType = (theImage.type() & CV_32F) ? GL_FLOAT : GL_UNSIGNED_BYTE;

    theTexture.update(theImage.data, dataType, 
                      format, theImage.cols, theImage.rows, true);
    
}
}}//namespace