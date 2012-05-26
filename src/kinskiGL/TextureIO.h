#ifndef _TEXTUREIO_IS_INCLUDED_
#define _TEXTUREIO_IS_INCLUDED_

#include "Texture.h"

namespace kinski
{
    class TextureIO 
    {
    private:
        TextureIO();
        
    public:
        
        static const gl::Texture loadTexture(const std::string &imgPath);
        static bool saveTexture(const std::string &imgPath, const gl::Texture &texture);
    };
}
#endif //_TEXTUREIO_IS_INCLUDED_