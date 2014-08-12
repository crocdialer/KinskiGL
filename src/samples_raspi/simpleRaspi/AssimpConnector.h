//
//  AssimpConnector.h
//  gl
//
//  Created by Fabian on 12/15/12.
//
//

#ifndef __gl__AssimpConnector__
#define __gl__AssimpConnector__

#include "gl/KinskiGL.h"

namespace kinski { namespace gl {

class AssimpConnector
{
 public:
    
    static MeshPtr loadModel(const std::string &thePath);
    static ScenePtr loadScene(const std::string &thePath);
};
    
}}//namespace

#endif /* defined(__gl__AssimpConnector__) */
