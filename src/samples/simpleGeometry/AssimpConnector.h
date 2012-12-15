//
//  AssimpConnector.h
//  kinskiGL
//
//  Created by Fabian on 12/15/12.
//
//

#ifndef __kinskiGL__AssimpConnector__
#define __kinskiGL__AssimpConnector__

#include <assimp/assimp.hpp>
#include <assimp/aiScene.h>
#include <assimp/aiPostProcess.h>

#include "kinskiGL/KinskiGL.h"

namespace kinski { namespace gl {

class AssimpConnector
{
 public:
    
    static std::shared_ptr<Mesh> loadModel(const std::string &theModelPath);
 
 private:
    
    static glm::mat4 aiMatrixToGlmMat(aiMatrix4x4 theMat);
    
    static std::shared_ptr<Geometry> createGeometry(const aiMesh *aMesh,
                                                    const aiScene *theScene = NULL);
    
    static std::shared_ptr<Material> createMaterial(const aiMaterial *mtl);
    
    static std::shared_ptr<gl::Bone>
    traverseNodes(const aiAnimation *theAnimation,
                  const aiNode *theNode,
                  const glm::mat4 &parentTransform,
                  const std::map<std::string, std::pair<int, glm::mat4> > &boneMap,
                  std::shared_ptr<gl::Animation> &outAnim,
                  std::shared_ptr<gl::Bone> parentBone = std::shared_ptr<gl::Bone>());
    
};
    
}}//namespace

#endif /* defined(__kinskiGL__AssimpConnector__) */
