//
//  AssimpConnector.h
//  kinskiGL
//
//  Created by Fabian on 12/15/12.
//
//

#ifndef __kinskiGL__AssimpConnector__
#define __kinskiGL__AssimpConnector__

#include "kinskiGL/KinskiGL.h"

namespace kinski { namespace gl {

class AssimpConnector
{
 public:
    
    static MeshPtr loadModel(const std::string &thePath);
    static ScenePtr loadScene(const std::string &thePath);
};
    
}}//namespace

#endif /* defined(__kinskiGL__AssimpConnector__) */
