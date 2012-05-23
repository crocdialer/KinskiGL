#include "TextureIO.h"
#include "opencv2/opencv.hpp"

using namespace std;
using namespace gl;
using namespace cv;

const Texture TextureIO::loadTexture(const string &imgPath)
{
    Mat tmp = imread(imgPath);
}

bool 
TextureIO::saveTexture(const Texture &texture, const string &imgPath)
{

}