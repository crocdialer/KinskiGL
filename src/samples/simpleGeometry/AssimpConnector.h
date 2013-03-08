//
//  AssimpConnector.h
//  kinskiGL
//
//  Created by Fabian on 12/15/12.
//
//

#ifndef __kinskiGL__AssimpConnector__
#define __kinskiGL__AssimpConnector__

#include <assimp/scene.h>
#include "kinskiGL/KinskiGL.h"

namespace kinski { namespace gl {

class AssimpConnector
{
 public:
    
    static MeshPtr loadModel(const std::string &theModelPath);
 
 private:
    
    static glm::mat4 aiMatrixToGlmMat(aiMatrix4x4 theMat);
    static GeometryPtr createGeometry(const aiMesh *aMesh, const aiScene *theScene = NULL);
    static MaterialPtr createMaterial(const aiMaterial *mtl);
    
    static BonePtr
    traverseNodes(const aiAnimation *theAnimation,
                  const aiNode *theNode,
                  const glm::mat4 &parentTransform,
                  const std::map<std::string, std::pair<int, glm::mat4> > &boneMap,
                  AnimationPtr &outAnim,
                  BonePtr parentBone = BonePtr());
    
};
    
}}//namespace

#endif /* defined(__kinskiGL__AssimpConnector__) */
