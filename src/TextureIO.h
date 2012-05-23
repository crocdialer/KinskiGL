#pragma once

#include "Texture.h"

class TextureIO 
{
   private:
    TextureIO();
    
public:
    
    static const gl::Texture loadTexture(const std::string &imgPath);
    static bool saveTexture(const std::string &imgPath, const gl::Texture &texture);
};