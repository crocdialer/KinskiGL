#ifndef _TEXTUREIO_IS_INCLUDED_
#define _TEXTUREIO_IS_INCLUDED_

#include "Texture.h"
#include "opencv2/opencv.hpp"

namespace kinski
{
    class TextureIO 
    {
    private:
        TextureIO();
        
    public:
        
        static const gl::Texture loadTexture(const std::string &imgPath);
        static bool saveTexture(const std::string &imgPath, const gl::Texture &texture);
        
        static void updateTexture(gl::Texture &theTexture,
                                  const cv::Mat &theImage);
    };
}
#endif //_TEXTUREIO_IS_INCLUDED_