//
//  Geometry.h
//  kinskiGL
//
//  Created by Fabian on 8/6/12.
//
//

#ifndef __kinskiGL__Geometry__
#define __kinskiGL__Geometry__

#include "kinskiGL/KinskiGL.h"

namespace kinski
{
namespace gl
{
    class Geometry
    {
    public:
        
        
    private:
        
        std::vector<glm::vec3> m_vertices;
        std::vector<glm::vec3> m_normals;
        std::vector<glm::vec2> m_texCoords;
        
        std::vector<glm::ivec3> m_faces;
        
    };
    
}//gl
}//kinski

#endif /* defined(__kinskiGL__Geometry__) */
