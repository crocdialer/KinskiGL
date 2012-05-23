#include "TextureIO.h"
#include "opencv2/opencv.hpp"

using namespace std;
using namespace gl;
using namespace cv;

const Texture TextureIO::loadTexture(const string &imgPath)
{
    Mat img = imread(imgPath);
    
    GLenum format=0;
	
	switch(img.channels()) 
	{
		case 1:
            format = GL_LUMINANCE;
			break;
		case 2:
            format = GL_LUMINANCE_ALPHA;
			break;
		case 3:
			format = GL_BGR;
			break;
        case 4:
			format = GL_BGRA;
		default:
			break;
	}
    
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