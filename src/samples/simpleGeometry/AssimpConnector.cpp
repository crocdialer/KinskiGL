//
//  AssimpConnector.cpp
//  kinskiGL
//
//  Created by Fabian on 12/15/12.
//
//

#include "AssimpConnector.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include "kinskiGL/Mesh.h"

using namespace std;
using namespace glm;

namespace kinski { namespace gl{

    glm::mat4 AssimpConnector::aiMatrixToGlmMat(aiMatrix4x4 theMat)
    {
        glm::mat4 ret;
        memcpy(&ret[0][0], theMat.Transpose()[0], 16 * sizeof(float));
        return ret;
    }
    
    gl::Mesh::Ptr AssimpConnector::loadModel(const std::string &theModelPath)
    {
        Assimp::Importer importer;
        LOG_DEBUG<<"trying to load model '"<<theModelPath<<"' ...";
        const aiScene *theScene = importer.ReadFile(searchFile(theModelPath), 0);
        
        if(false)
        {
            theScene = importer.ApplyPostProcessing(aiProcess_Triangulate
                                                | aiProcess_GenSmoothNormals
                                                | aiProcess_CalcTangentSpace);
        }
        
        if (theScene)
        {
            aiMesh *aMesh = theScene->mMeshes[0];
            LOG_DEBUG<<"loaded model: "<<aMesh->mNumVertices<<" vertices - " <<aMesh->mNumFaces<<" faces";
            gl::GeometryPtr geom( createGeometry(aMesh, theScene) );
            gl::MaterialPtr mat = createMaterial(theScene->mMaterials[aMesh->mMaterialIndex]);
            
            try
            {
                if(geom->hasBones())
                    mat->setShader(gl::createShader(gl::SHADER_PHONG_SKIN));
                else{
                    mat->setShader(gl::createShader(gl::SHADER_PHONG));
                }
                
            }catch (std::exception &e)
            {
                LOG_WARNING<<e.what();
            }
            gl::Mesh::Ptr mesh(new gl::Mesh(geom, mat));
            importer.FreeScene();
            return mesh;
        }
        else
        {
            throw Exception("could not load model: "+ theModelPath);
        }
    }
    
    BonePtr
    AssimpConnector::traverseNodes(const aiAnimation *theAnimation,
                                   const aiNode *theNode,
                                   const glm::mat4 &parentTransform,
                                   const map<std::string, pair<int, glm::mat4> > &boneMap,
                                   AnimationPtr &outAnim,
                                   BonePtr parentBone)
    {
        BonePtr currentBone;
        string nodeName(theNode->mName.data);
        glm::mat4 nodeTransform = aiMatrixToGlmMat(theNode->mTransformation);
        const aiNodeAnim* nodeAnim = NULL;
        
        if(theAnimation)
        {
            for (int i = 0; i < theAnimation->mNumChannels; i++)
            {
                aiNodeAnim *ptr = theAnimation->mChannels[i];
                
                if(string(ptr->mNodeName.data) == nodeName)
                {
                    nodeAnim = ptr;
                    break;
                }
            }
        }
        
        mat4 globalTransform = parentTransform * nodeTransform;
        map<std::string, pair<int, mat4> >::const_iterator it = boneMap.find(nodeName);
        
        // we have a Bone node
        if (it != boneMap.end())
        {
            int boneIndex = it->second.first;
            const mat4 &offset = it->second.second;
            currentBone = BonePtr(new gl::Bone);
            currentBone->name = nodeName;
            currentBone->index = boneIndex;
            currentBone->transform = nodeTransform;
            currentBone->worldtransform = globalTransform;
            currentBone->offset = offset;
            currentBone->parent = parentBone;
            
            // we have animation keys for this bone
            if(nodeAnim)
            {
                char buf[1024];
                sprintf(buf, "Found animation for %s: %d posKeys -- %d rotKeys -- %d scaleKeys\n",
                       nodeAnim->mNodeName.data,
                       nodeAnim->mNumPositionKeys,
                       nodeAnim->mNumRotationKeys,
                       nodeAnim->mNumScalingKeys);
                LOG_TRACE<<buf;
                
                gl::AnimationKeys animKeys;
                glm::vec3 bonePosition;
                glm::vec3 boneScale;
                glm::quat boneRotation;
                
                for (int i = 0; i < nodeAnim->mNumRotationKeys; i++)
                {
                    aiQuaternion rot = nodeAnim->mRotationKeys[i].mValue;
                    boneRotation = glm::quat(rot.w, rot.x, rot.y, rot.z);
                    animKeys.rotationkeys.push_back(gl::Key<glm::quat>(nodeAnim->mRotationKeys[i].mTime,
                                                                       boneRotation));
                }
                
                for (int i = 0; i < nodeAnim->mNumPositionKeys; i++)
                {
                    aiVector3D pos = nodeAnim->mPositionKeys[i].mValue;
                    bonePosition = vec3(pos.x, pos.y, pos.z);
                    animKeys.positionkeys.push_back(gl::Key<glm::vec3>(nodeAnim->mPositionKeys[i].mTime,
                                                                       bonePosition));
                }
                
                for (int i = 0; i < nodeAnim->mNumScalingKeys; i++)
                {
                    aiVector3D scaleTmp = nodeAnim->mScalingKeys[i].mValue;
                    boneScale = vec3(scaleTmp.x, scaleTmp.y, scaleTmp.z);
                    animKeys.scalekeys.push_back(gl::Key<glm::vec3>(nodeAnim->mScalingKeys[i].mTime,
                                                                    boneScale));
                }
                outAnim->boneKeys[currentBone] = animKeys;
            }
        }
        
        for (int i = 0 ; i < theNode->mNumChildren ; i++)
        {
            std::shared_ptr<gl::Bone> child = traverseNodes(theAnimation, theNode->mChildren[i],
                                                            globalTransform, boneMap, outAnim,
                                                            currentBone);
            
            if(currentBone && child)
                currentBone->children.push_back(child);
            else if(child)
                currentBone = child;
        }
        return currentBone;
    }
    
    gl::Geometry::Ptr AssimpConnector::createGeometry(const aiMesh *aMesh, const aiScene *theScene)
    {
        gl::Geometry::Ptr geom (new gl::Geometry);
        
        geom->vertices().reserve(aMesh->mNumVertices);
        geom->vertices().insert(geom->vertices().end(), (glm::vec3*)aMesh->mVertices,
                                (glm::vec3*)aMesh->mVertices + aMesh->mNumVertices);
        
        if(aMesh->HasTextureCoords(0))
        {
            geom->texCoords().reserve(aMesh->mNumVertices);
            for (int i = 0; i < aMesh->mNumVertices; i++)
            {
                geom->appendTextCoord(aMesh->mTextureCoords[0][i].x, aMesh->mTextureCoords[0][i].y);
            }
        }else
        {
            for (int i = 0; i < aMesh->mNumVertices; i++)
            {
                geom->appendTextCoord(0, 0);
            }
        }
        
        for(int i = 0; i < aMesh->mNumFaces; i++)
        {
            const aiFace &f = aMesh->mFaces[i];
            geom->appendFace(f.mIndices[0], f.mIndices[1], f.mIndices[2]);
        }
        
        if(aMesh->HasNormals())
        {
            geom->normals().reserve(aMesh->mNumVertices);
            geom->normals().insert(geom->normals().end(), (glm::vec3*)aMesh->mNormals,
                                   (glm::vec3*) aMesh->mNormals + aMesh->mNumVertices);
        }
        else
        {
            geom->computeVertexNormals();
        }
        
        if(aMesh->HasTangentsAndBitangents())
        {
            geom->tangents().reserve(aMesh->mNumVertices);
            geom->tangents().insert(geom->tangents().end(), (glm::vec3*)aMesh->mTangents,
                                    (glm::vec3*) aMesh->mTangents + aMesh->mNumVertices);
        }
        else
        {
            // compute tangents
            geom->computeTangents();
        }
        
        
        if(aMesh->HasBones())
        {
            typedef map<uint32_t, list< pair<uint32_t, float> > > WeightMap;
            WeightMap weightMap;
            map<std::string, pair<int, mat4> > boneMap;
            
            for (int i = 0; i < aMesh->mNumBones; ++i)
            {
                aiBone* bone = aMesh->mBones[i];
                
                boneMap[bone->mName.data] = std::make_pair(i, aiMatrixToGlmMat(bone->mOffsetMatrix));
                
                for (int j = 0; j < bone->mNumWeights; ++j)
                {
                    const aiVertexWeight &w = bone->mWeights[j];
                    weightMap[w.mVertexId].push_back( std::make_pair(i, w.mWeight) );
                }
            }
            
            // generate empty indices and weights
            for (int i = 0; i < geom->vertices().size(); ++i)
            {
                geom->boneVertexData().push_back(gl::BoneVertexData());
            }
            
            for (WeightMap::iterator it = weightMap.begin(); it != weightMap.end(); ++it)
            {
                int i = 0;
                gl::BoneVertexData &boneData = geom->boneVertexData()[it->first];
                list< pair<uint32_t, float> >::iterator pairIt = it->second.begin();
                for (; pairIt != it->second.end(); ++pairIt)
                {
                    boneData.indices[i] = pairIt->first;
                    boneData.weights[i] = pairIt->second;
                    i++;
                }
            }
            
            if(theScene && theScene->mNumAnimations > 0)
            {
                aiAnimation *assimpAnimation = theScene->mNumAnimations > 0 ?
                theScene->mAnimations[0] : NULL;
                shared_ptr<gl::Animation> anim(new gl::Animation);
                anim->duration = assimpAnimation->mDuration;
                anim->ticksPerSec = assimpAnimation->mTicksPerSecond;
                std::shared_ptr<gl::Bone> rootBone = traverseNodes(assimpAnimation,
                                                                   theScene->mRootNode, mat4(),
                                                                   boneMap, anim);
                
                geom->setAnimation(anim);
                geom->rootBone() = rootBone;
            }
        }
        geom->computeBoundingBox();
        
        return geom;
    }
    
    gl::Material::Ptr AssimpConnector::createMaterial(const aiMaterial *mtl)
    {
        gl::Material::Ptr theMaterial(new gl::Material);
        int ret1, ret2;
        aiColor4D diffuse;
        aiColor4D specular;
        aiColor4D ambient;
        aiColor4D emission;
        float shininess, strength;
        int two_sided;
        int wireframe;
        aiString texPath;
        glm::vec4 color;
        
        if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
        {
            color.r = diffuse.r; color.g = diffuse.g; color.b = diffuse.b; color.a = diffuse.a;
            theMaterial->setDiffuse(color);
        }
        
        if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular))
        {
            color.r = specular.r; color.g = specular.g; color.b = specular.b; color.a = specular.a;
            theMaterial->setSpecular(color);
        }
        
        if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient))
        {
            color.r = ambient.r; color.g = ambient.g; color.b = ambient.b; color.a = ambient.a;
            theMaterial->setAmbient(color);
        }
        
        if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission))
        {
            color.r = emission.r; color.g = emission.g; color.b = emission.b; color.a = emission.a;
            theMaterial->setEmission(color);
        }
        ret1 = aiGetMaterialFloat(mtl, AI_MATKEY_SHININESS, &shininess);
        ret2 = aiGetMaterialFloat(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength);
        
        if((ret1 == AI_SUCCESS) && (ret2 == AI_SUCCESS))
            theMaterial->setShinyness(shininess * strength);
        else
        {
            theMaterial->setShinyness(0.f);
            theMaterial->setSpecular(vec4(0));
        }
        
        if(AI_SUCCESS == aiGetMaterialInteger(mtl, AI_MATKEY_ENABLE_WIREFRAME, &wireframe))
            theMaterial->setWireframe(wireframe);
        
        if((AI_SUCCESS == aiGetMaterialInteger(mtl, AI_MATKEY_TWOSIDED, &two_sided)))
            theMaterial->setTwoSided(two_sided);
        
        for (int i = 0; i < 10; i++)
        {
            if(AI_SUCCESS == mtl->GetTexture(aiTextureType(aiTextureType_DIFFUSE + i), 0, &texPath))
            {
                theMaterial->addTexture(gl::createTextureFromFile(string(texPath.data)));
            }
        }
        return theMaterial;
    }

}}//namespace