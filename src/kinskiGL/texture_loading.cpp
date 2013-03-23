//
//  texture_loading.cpp
//  kinskiGL
//
//  Created by Fabian on 14.02.13.
//
//

#include "KinskiGL.h"
#include "Texture.h"
#include "stb_image.c"

namespace kinski { namespace gl {
  
    Texture createTextureFromFile(const std::string &theFileName)
    {
        Texture ret;
        std::vector<uint8_t> dataVec = kinski::readBinaryFile(theFileName);
        if(dataVec.empty())
            return ret;
        
        int width, height, num_components;
        unsigned char *data = stbi_load_from_memory(&dataVec[0], dataVec.size(),
                                                    &width, &height, &num_components, 0);
        
        if(!data) throw ImageLoadException(theFileName);
        
    
        // ... process data if not NULL ...
        // ... x = width, y = height, n = # 8-bit components per pixel ...
        // ... replace '0' with '1'..'4' to force that many components per pixel
        // ... but 'n' will always be the number that it would have been if you said 0
        
        GLenum format=0;
        
        switch(num_components)
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
                format = GL_RGB;
                break;
            case 4:
                format =  GL_RGBA;
            default:
                break;
#endif
        }
        
        Texture::Format fmt;
        fmt.setInternalFormat(format);
        ret = Texture (data, format, width, height, fmt);
        ret.setFlipped();
        
        // requires OpenGL 3.3+
        //GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
        //glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
        
        stbi_image_free(data);
        return ret;
    }
    
}}
