//
//  AssimpConnector.h
//  gl
//
//  Created by Fabian on 12/15/12.
//
//

#ifndef __gl__AssimpConnector__
#define __gl__AssimpConnector__

#include "gl/gl.h"

namespace kinski { namespace gl {

class AssimpConnector
{
 public:
    
    static MeshPtr loadModel(const std::string &thePath);
    static ScenePtr loadScene(const std::string &thePath);
    static size_t add_animations_to_mesh(const std::string &thePath,
                                         MeshPtr m);
};
    
}}//namespace

#endif /* defined(__gl__AssimpConnector__) */
