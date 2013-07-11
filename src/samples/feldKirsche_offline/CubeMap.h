#ifndef __kinskiGL__CubeMap__
#define __kinskiGL__CubeMap__
 
#include "kinskiGL/KinskiGL.h"

namespace kinski { namespace gl{
    
    class CubeMap
    {
    public:
        
        enum CubeMapType {V_CROSS, H_CROSS};
        
        CubeMap(){};
        CubeMap(GLsizei texWidth, GLsizei texHeight, const uint8_t *data_pos_x, const uint8_t *data_neg_x,
                const uint8_t *data_pos_y, const uint8_t *data_neg_y, const uint8_t *data_pos_z,
                const uint8_t *data_neg_z);
        void bind();
        void bindMulti( int loc );
        void unbind();
        static void enableFixedMapping();
        static void disableFixedMapping();
    private:
        unsigned int textureObject;
    };
    
    CubeMap create_cube_map(const std::string &path, CubeMap::CubeMapType type);
    CubeMap create_cube_map(const std::string &path_pos_x, const std::string &path_neg_x,
                            const std::string &path_pos_y, const std::string &path_neg_y,
                            const std::string &path_pos_z, const std::string &path_neg_z);
}}

#endif