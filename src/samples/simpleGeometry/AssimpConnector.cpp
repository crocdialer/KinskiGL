//
//  AssimpConnector.cpp
//  kinskiGL
//
//  Created by Fabian on 12/15/12.
//
//

#include <boost/bind.hpp>
#include "AssimpConnector.h"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include "kinskiGL/Mesh.h"
#include "kinskiGL/Geometry.h"
#include "kinskiGL/Material.h"

using namespace std;
using namespace glm;

namespace kinski { namespace gl{
    
    typedef map<std::string, pair<int, mat4> > BoneMap;
    typedef map<uint32_t, list< pair<uint32_t, float> > > WeightMap;
    
    void mergeGeometries(GeometryPtr src, GeometryPtr dst);
    gl::GeometryPtr createGeometry(const aiMesh *aMesh, const aiScene *theScene);
    gl::MaterialPtr createMaterial(const aiMaterial *mtl);
    void loadBones(const aiMesh *aMesh, uint32_t base_vertex, BoneMap& bonemap, WeightMap &weightmap);
    void insertBoneData(GeometryPtr geom, const WeightMap &weightmap);
    BonePtr traverseNodes(const aiAnimation *theAnimation,
                          const aiNode *theNode,
                          const glm::mat4 &parentTransform,
                          const map<std::string, pair<int, glm::mat4> > &boneMap,
                          AnimationPtr &outAnim,
                          BonePtr parentBone = BonePtr());
    
    inline glm::mat4 aimatrix_to_glm_mat4(aiMatrix4x4 theMat)
    {
        glm::mat4 ret;
        memcpy(&ret[0][0], theMat.Transpose()[0], 16 * sizeof(float));
        return ret;
    }
    
    gl::GeometryPtr createGeometry(const aiMesh *aMesh, const aiScene *theScene)
    {
        gl::GeometryPtr geom = Geometry::create();
        
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
            geom->texCoords().resize(aMesh->mNumVertices, glm::vec2(0));
        }
        
        std::vector<uint32_t> &indices = geom->indices(); indices.reserve(aMesh->mNumFaces * 3);
        for(int i = 0; i < aMesh->mNumFaces; i++)
        {
            const aiFace &f = aMesh->mFaces[i];
            if(f.mNumIndices != 3) throw Exception("Non triangle mesh loaded");
            indices.insert(indices.end(), f.mIndices, f.mIndices + 3);
        }
        geom->faces().resize(aMesh->mNumFaces);
        ::memcpy(&geom->faces()[0], &indices[0], indices.size() * sizeof(uint32_t));
        
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
        
        if(aMesh->HasVertexColors(0))//TODO: test
        {
            geom->colors().reserve(aMesh->mNumVertices);
            geom->colors().insert(geom->colors().end(), (glm::vec4*)aMesh->mColors,
                                   (glm::vec4*) aMesh->mColors + aMesh->mNumVertices);
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
        geom->computeBoundingBox();
        return geom;
    }
    
    void loadBones(const aiMesh *aMesh, uint32_t base_vertex, BoneMap& bonemap, WeightMap &weightmap)
    {
        static int num_bones = 0;
        if(base_vertex == 0) num_bones = 0;
        
        if(aMesh->HasBones())
        {
            int bone_index;
            
            for (int i = 0; i < aMesh->mNumBones; ++i)
            {
                aiBone* bone = aMesh->mBones[i];
                if(bonemap.find(bone->mName.data) == bonemap.end())
                {
                    bonemap[bone->mName.data] = std::make_pair(num_bones,
                                                               aimatrix_to_glm_mat4(bone->mOffsetMatrix));
                    bone_index = num_bones;
                    num_bones++;
                }
                else
                {
                    bone_index = bonemap[bone->mName.data].first;
                }
                
                for (int j = 0; j < bone->mNumWeights; ++j)
                {
                    const aiVertexWeight &w = bone->mWeights[j];
                    weightmap[w.mVertexId + base_vertex].push_back( std::make_pair(bone_index, w.mWeight) );
                }
            }
        }
    }
    
    void insertBoneData(GeometryPtr geom, const WeightMap &weightmap)
    {
        if(weightmap.empty()) return;
        
        // allocate storage for indices and weights
        geom->boneVertexData().resize(geom->vertices().size());
        
        for (WeightMap::const_iterator it = weightmap.begin(); it != weightmap.end(); ++it)
        {
            int i = 0;
            gl::BoneVertexData &boneData = geom->boneVertexData()[it->first];
            
            list< pair<uint32_t, float> > tmp_list(it->second.begin(), it->second.end());
            tmp_list.sort(boost::bind(&pair<uint32_t, float>::second, _1) >
                          boost::bind(&pair<uint32_t, float>::second, _2));
            
            //if(it->second.size() > boneData.indices.length())
                
            list< pair<uint32_t, float> >::const_iterator listIt = tmp_list.begin();
            for (; listIt != tmp_list.end(); ++listIt)
            {
                if(i >= boneData.indices.length()) break;
                boneData.indices[i] = listIt->first;
                boneData.weights[i] = listIt->second;
                i++;
            }
        }
    }
    
    gl::MaterialPtr createMaterial(const aiMaterial *mtl)
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
        
        //int num_diffuse = aiGetMaterialTextureCount(mtl, aiTextureType_DIFFUSE);
        
        for (int i = 0; i < 10; i++)
        {
            if(AI_SUCCESS == mtl->GetTexture(aiTextureType(aiTextureType_DIFFUSE + i), 0, &texPath))
            {
                try{theMaterial->addTexture(gl::createTextureFromFile(string(texPath.data)));}
                catch(Exception &e){LOG_ERROR<<e.what();}
            }
        }
        return theMaterial;
    }
    
    void mergeGeometries(GeometryPtr src, GeometryPtr dst)
    {
        dst->appendVertices(src->vertices());
        dst->appendNormals(src->normals());
        dst->appendColors(src->colors());
        dst->appendTextCoords(src->texCoords());
        dst->appendIndices(src->indices());
        dst->faces().insert(dst->faces().end(), src->faces().begin(), src->faces().end());
    }
    
    gl::MeshPtr AssimpConnector::loadModel(const std::string &theModelPath)
    {
        Assimp::Importer importer;
        LOG_DEBUG<<"trying to load model '"<<theModelPath<<"' ...";
        const aiScene *theScene = importer.ReadFile(searchFile(theModelPath), 0);
        
        if(true)
        {
            theScene = importer.ApplyPostProcessing(aiProcess_Triangulate
                                                    | aiProcess_GenSmoothNormals
                                                    | aiProcess_JoinIdenticalVertices
                                                    | aiProcess_CalcTangentSpace);
        }
        if (theScene)
        {
            std::vector<gl::GeometryPtr> geometries;
            std::vector<gl::MaterialPtr> materials;
            uint32_t current_index = 0, current_vertex = 0;
            GeometryPtr combined_geom = gl::Geometry::create();
            BoneMap bonemap;
            WeightMap weightmap;
            std::vector<Mesh::Entry> entries;
            
            for (int i = 0; i < theScene->mNumMeshes; i++)
            {
                aiMesh *aMesh = theScene->mMeshes[i];
                GeometryPtr g = createGeometry(aMesh, theScene);
                loadBones(aMesh, current_vertex, bonemap, weightmap);
                Mesh::Entry m;
                m.num_vertices = g->vertices().size();
                m.numdices = g->indices().size();
                m.base_index = current_index;
                m.base_vertex = current_vertex;
                m.material_index = aMesh->mMaterialIndex;
                entries.push_back(m);
                current_vertex += g->vertices().size();
                current_index += g->indices().size();
                
                geometries.push_back(g);
                mergeGeometries(g, combined_geom);
                materials.push_back(createMaterial(theScene->mMaterials[aMesh->mMaterialIndex]));
            }
            combined_geom->computeBoundingBox();
            insertBoneData(combined_geom, weightmap);
            
            gl::GeometryPtr geom = combined_geom;
            gl::MaterialPtr mat = materials[0];
            gl::MeshPtr mesh = gl::Mesh::create(geom, mat);
            mesh->entries() = entries;
            mesh->materials() = materials;
            
            AnimationPtr dummy;
            mesh->rootBone() = traverseNodes(NULL, theScene->mRootNode, mat4(),
                                             bonemap, dummy);
            if(mesh->rootBone()) mesh->initBoneMatrices();
            
            for (int i = 0; i < theScene->mNumAnimations; i++)
            {
                aiAnimation *assimpAnimation = theScene->mAnimations[i];
                AnimationPtr anim(new gl::Animation());
                anim->duration = assimpAnimation->mDuration;
                anim->ticksPerSec = assimpAnimation->mTicksPerSecond;
                BonePtr rootBone = traverseNodes(assimpAnimation, theScene->mRootNode, mat4(),
                                                 bonemap, anim);
                mesh->addAnimation(anim);
                mesh->rootBone() = rootBone;
            }
            
            gl::Shader shader;
            try
            {
                if(geom->hasBones())
                    shader = gl::createShader(gl::SHADER_PHONG_SKIN);
                else{
                    shader = gl::createShader(gl::SHADER_PHONG);
                }
                
            }catch (std::exception &e)
            {
                LOG_WARNING<<e.what();
            }
            
            for (int i = 0; i < materials.size(); i++)
            {
                materials[i]->setShader(shader);
            }
            mesh->createVertexArray();
            LOG_DEBUG<<"loaded model: "<<geom->vertices().size()<<" vertices - " <<
                geom->faces().size()<<" faces";
            importer.FreeScene();
            return mesh;
        }
        else
        {
            throw Exception("could not load model: "+ theModelPath);
        }
    }
    
    BonePtr traverseNodes(const aiAnimation *theAnimation,
                          const aiNode *theNode,
                          const glm::mat4 &parentTransform,
                          const map<std::string, pair<int, glm::mat4> > &boneMap,
                          AnimationPtr &outAnim,
                          BonePtr parentBone)
    {
        BonePtr currentBone;
        string nodeName(theNode->mName.data);
        glm::mat4 nodeTransform = aimatrix_to_glm_mat4(theNode->mTransformation);
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
        BoneMap::const_iterator it = boneMap.find(nodeName);
        
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
                sprintf(buf, "Found animation for %s: %d posKeys -- %d rotKeys -- %d scaleKeys",
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
            BonePtr child = traverseNodes(theAnimation, theNode->mChildren[i], globalTransform,
                                          boneMap, outAnim, currentBone);
            
            if(currentBone && child)
                currentBone->children.push_back(child);
            else if(child)
                currentBone = child;
        }
        return currentBone;
    }

}}//namespace