//
//  AssimpConnector.cpp
//  gl
//
//  Created by Fabian on 12/15/12.
//
//

#include <boost/bind.hpp>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include "gl/Mesh.h"
#include "gl/Geometry.h"
#include "gl/Material.h"
#include "AssimpConnector.h"

using namespace std;
using namespace glm;

namespace kinski { namespace gl{
    
    typedef map<std::string, pair<int, mat4>> BoneMap;
    typedef map<uint32_t, list< pair<uint32_t, float>>> WeightMap;
    
/////////////////////////////////////////////////////////////////
    
    void mergeGeometries(GeometryPtr src, GeometryPtr dst);
    
    gl::GeometryPtr createGeometry(const aiMesh *aMesh, const aiScene *theScene);
    
    gl::MaterialPtr createMaterial(const aiMaterial *mtl);
    
    void loadBones(const aiMesh *aMesh, uint32_t base_vertex, BoneMap& bonemap, WeightMap &weightmap);
    
    void insertBoneVertexData(GeometryPtr geom, const WeightMap &weightmap, uint32_t start_index = 0);

    
    BonePtr create_bone_hierarchy(const aiNode *theNode, const mat4 &parentTransform,
                                  const map<std::string, pair<int, mat4> > &boneMap,
                                  BonePtr parentBone = BonePtr());
    
    BonePtr find_bone_by_name(const std::string &the_name, BonePtr the_root);
    
    void create_bone_animation(const aiNode *theNode, const aiAnimation *theAnimation,
                               BonePtr root_bone, MeshAnimation &outAnim);
    
    void get_node_transform(const aiNode *the_node, mat4 &the_transform);
    
/////////////////////////////////////////////////////////////////
    
    inline mat4 aimatrix_to_glm_mat4(aiMatrix4x4 theMat)
    {
        mat4 ret;
        memcpy(&ret[0][0], theMat.Transpose()[0], sizeof(ret));
        return ret;
    }
    
/////////////////////////////////////////////////////////////////
    
    gl::GeometryPtr createGeometry(const aiMesh *aMesh, const aiScene *theScene)
    {
        gl::GeometryPtr geom = Geometry::create();
        
        geom->vertices().insert(geom->vertices().end(), (vec3*)aMesh->mVertices,
                                (vec3*)aMesh->mVertices + aMesh->mNumVertices);
        
        if(aMesh->HasTextureCoords(0))
        {
            geom->texCoords().reserve(aMesh->mNumVertices);
            for (uint32_t i = 0; i < aMesh->mNumVertices; i++)
            {
                geom->appendTextCoord(aMesh->mTextureCoords[0][i].x, aMesh->mTextureCoords[0][i].y);
            }
        }else
        {
            geom->texCoords().resize(aMesh->mNumVertices, vec2(0));
        }
        
        std::vector<uint32_t> &indices = geom->indices(); indices.reserve(aMesh->mNumFaces * 3);
        for(uint32_t i = 0; i < aMesh->mNumFaces; i++)
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
            geom->normals().insert(geom->normals().end(), (vec3*)aMesh->mNormals,
                                   (vec3*) aMesh->mNormals + aMesh->mNumVertices);
        }
        else
        {
            geom->computeVertexNormals();
        }
        
        if(aMesh->HasVertexColors(0))//TODO: test
        {
            geom->colors().reserve(aMesh->mNumVertices);
            geom->colors().insert(geom->colors().end(), (vec4*)aMesh->mColors,
                                   (vec4*) aMesh->mColors + aMesh->mNumVertices);
        }
        
        if(aMesh->HasTangentsAndBitangents())
        {
            geom->tangents().reserve(aMesh->mNumVertices);
            geom->tangents().insert(geom->tangents().end(), (vec3*)aMesh->mTangents,
                                    (vec3*) aMesh->mTangents + aMesh->mNumVertices);
        }
        else
        {
            // compute tangents
            geom->computeTangents();
        }
        geom->computeBoundingBox();
        return geom;
    }

/////////////////////////////////////////////////////////////////
    
    void loadBones(const aiMesh *aMesh, uint32_t base_vertex, BoneMap& bonemap, WeightMap &weightmap)
    {
        int num_bones = 0;
        if(base_vertex == 0) num_bones = 0;
        
        if(aMesh->HasBones())
        {
            int bone_index = 0, start_index = bonemap.size();
            
            for (uint32_t i = 0; i < aMesh->mNumBones; ++i)
            {
                aiBone* bone = aMesh->mBones[i];
                if(bonemap.find(bone->mName.data) == bonemap.end())
                {
                    bone_index = num_bones + start_index;
                    bonemap[bone->mName.data] = std::make_pair(bone_index,
                                                               aimatrix_to_glm_mat4(bone->mOffsetMatrix));
                    num_bones++;
                }
                else{ bone_index = bonemap[bone->mName.data].first; }
                
                for (uint32_t j = 0; j < bone->mNumWeights; ++j)
                {
                    const aiVertexWeight &w = bone->mWeights[j];
                    weightmap[w.mVertexId + base_vertex].push_back( std::make_pair(bone_index, w.mWeight) );
                }
            }
        }
    }

/////////////////////////////////////////////////////////////////
    
    void insertBoneVertexData(GeometryPtr geom, const WeightMap &weightmap, uint32_t start_index)
    {
        if(weightmap.empty()) return;
        
        // allocate storage for indices and weights
        geom->boneVertexData().resize(geom->vertices().size());
        
        for (WeightMap::const_iterator it = weightmap.begin(); it != weightmap.end(); ++it)
        {
            uint32_t i = 0;
            gl::BoneVertexData &boneData = geom->boneVertexData()[it->first + start_index];
            
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

/////////////////////////////////////////////////////////////////
    
    gl::MaterialPtr createMaterial(const aiMaterial *mtl)
    {
        gl::Material::Ptr theMaterial = gl::Material::create();
        int ret1, ret2;
        aiColor4D diffuse, specular, ambient, emission, transparent;
        float shininess, strength;
        int two_sided;
        int wireframe;
        aiString texPath;
        vec4 color;
        
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
        
        // transparent material
        if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_TRANSPARENT, &transparent))
        {
            //TODO: needed ?
        }
        float opacity = 1.f;
        mtl->Get(AI_MATKEY_OPACITY, opacity);
        if(opacity < 1.f)
        {
            Color d = theMaterial->diffuse();
            d.a = opacity;
            theMaterial->setDiffuse(d);
            theMaterial->setBlending();
            //theMaterial->setDepthWrite(false);
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
                try{theMaterial->addTexture(gl::createTextureFromFile(string(texPath.data), true, true));}
                catch(Exception &e){LOG_ERROR<<e.what();}
            }
        }
        return theMaterial;
    }

/////////////////////////////////////////////////////////////////
    
    void mergeGeometries(GeometryPtr src, GeometryPtr dst)
    {
        dst->appendVertices(src->vertices());
        dst->appendNormals(src->normals());
        dst->appendColors(src->colors());
        dst->tangents().insert(dst->tangents().end(), src->tangents().begin(), src->tangents().end());
        dst->appendTextCoords(src->texCoords());
        dst->appendIndices(src->indices());
        dst->boneVertexData().insert(dst->boneVertexData().end(), src->boneVertexData().begin(),
                                     src->boneVertexData().end());
        dst->faces().insert(dst->faces().end(), src->faces().begin(), src->faces().end());
    }

/////////////////////////////////////////////////////////////////
    
    gl::MeshPtr AssimpConnector::loadModel(const std::string &theModelPath)
    {
        Assimp::Importer importer;
        std::string found_path;
        try { found_path = search_file(theModelPath); }
        catch (FileNotFoundException &e)
        {
            LOG_ERROR << e.what();
            return gl::MeshPtr();
        }
        LOG_DEBUG << "loading model '" << theModelPath << "' ...";
        const aiScene *theScene = importer.ReadFile(found_path, 0);
        
        // super useful postprocessing steps
        theScene = importer.ApplyPostProcessing(aiProcess_Triangulate
                                                | aiProcess_GenSmoothNormals
                                                | aiProcess_JoinIdenticalVertices
                                                | aiProcess_CalcTangentSpace
                                                | aiProcess_LimitBoneWeights);
        if(theScene)
        {
            std::vector<gl::GeometryPtr> geometries;
            std::vector<gl::MaterialPtr> materials;
            materials.resize(theScene->mNumMaterials, gl::Material::create());
            
            uint32_t current_index = 0, current_vertex = 0;
            GeometryPtr combined_geom = gl::Geometry::create();
            BoneMap bonemap;
            WeightMap weightmap;
            std::vector<Mesh::Entry> entries;
            
            for (uint32_t i = 0; i < theScene->mNumMeshes; i++)
            {
                aiMesh *aMesh = theScene->mMeshes[i];
                GeometryPtr g = createGeometry(aMesh, theScene);
                loadBones(aMesh, current_vertex, bonemap, weightmap);
                Mesh::Entry m;
                m.num_vertices = g->vertices().size();
                m.num_indices = g->indices().size();
                m.base_index = current_index;
                m.base_vertex = current_vertex;
                m.material_index = aMesh->mMaterialIndex;
                entries.push_back(m);
                current_vertex += g->vertices().size();
                current_index += g->indices().size();
                
                geometries.push_back(g);
                mergeGeometries(g, combined_geom);
                materials[aMesh->mMaterialIndex] = createMaterial(theScene->mMaterials[aMesh->mMaterialIndex]);
            }
            combined_geom->computeBoundingBox();
            
            insertBoneVertexData(combined_geom, weightmap);
            
            gl::GeometryPtr geom = combined_geom;
            gl::MeshPtr mesh = gl::Mesh::create(combined_geom, materials[0]);
            mesh->entries() = entries;
            mesh->materials() = materials;
            mesh->rootBone() = create_bone_hierarchy(theScene->mRootNode, mat4(), bonemap);
            
            if(mesh->rootBone()) mesh->initBoneMatrices();
            
            for (uint32_t i = 0; i < theScene->mNumAnimations; i++)
            {
                aiAnimation *assimpAnimation = theScene->mAnimations[i];
                MeshAnimation anim;
                anim.duration = assimpAnimation->mDuration;
                anim.ticksPerSec = assimpAnimation->mTicksPerSecond;
                create_bone_animation(theScene->mRootNode, assimpAnimation, mesh->rootBone(), anim);
                mesh->addAnimation(anim);
            }
            
            gl::Shader shader;
            try
            {
                if(geom->hasBones())
                    shader = gl::createShader(gl::ShaderType::PHONG_SKIN);
                else{
                    shader = gl::createShader(gl::ShaderType::PHONG);
                }
                
            }catch (std::exception &e){ LOG_WARNING<<e.what(); }
            
            for (uint32_t i = 0; i < materials.size(); i++)
            {
                materials[i]->setShader(shader);
            }
            mesh->createVertexArray();
            
            LOG_DEBUG<<"loaded model: "<<geom->vertices().size()<<" vertices - " <<
            geom->faces().size()<<" faces - "<< mesh->get_num_bones(mesh->rootBone()) << " bones";
            
            LOG_DEBUG<<"bounds: " <<to_string(mesh->boundingBox().min)<<" - "<<
                to_string(mesh->boundingBox().max);
            
            importer.FreeScene();
            return mesh;
        }
        else
        {
            return gl::MeshPtr();
        }
    }

/////////////////////////////////////////////////////////////////
    
    BonePtr create_bone_hierarchy(const aiNode *theNode, const mat4 &parentTransform,
                                  const map<std::string, pair<int, mat4> > &boneMap,
                                  BonePtr parentBone)
    {
        BonePtr currentBone;
        string nodeName(theNode->mName.data);
        mat4 nodeTransform = aimatrix_to_glm_mat4(theNode->mTransformation);
        
        mat4 globalTransform = parentTransform * nodeTransform;
        BoneMap::const_iterator it = boneMap.find(nodeName);
        
        // current node corresponds to a bone
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
        }
        
        for(uint32_t i = 0 ; i < theNode->mNumChildren ; i++)
        {
            BonePtr child = create_bone_hierarchy(theNode->mChildren[i], globalTransform,
                                                  boneMap, currentBone);
            
            if(currentBone && child)
                currentBone->children.push_back(child);
            else if(child)// we are at root lvl
                currentBone = child;
        }
        return currentBone;
    }
    
/////////////////////////////////////////////////////////////////
    
    void create_bone_animation(const aiNode *theNode, const aiAnimation *theAnimation,
                               BonePtr root_bone, MeshAnimation &outAnim)
    {
        string nodeName(theNode->mName.data);
        const aiNodeAnim* nodeAnim = nullptr;
        
        if(theAnimation)
        {
            for (uint32_t i = 0; i < theAnimation->mNumChannels; i++)
            {
                aiNodeAnim *ptr = theAnimation->mChannels[i];
                
                if(string(ptr->mNodeName.data) == nodeName)
                {
                    nodeAnim = ptr;
                    break;
                }
            }
        }

        BonePtr bone = find_bone_by_name(nodeName, root_bone);
        
        // this node corresponds to a bone node in the hierarchy
        // and we have animation keys for this bone
        if(bone && nodeAnim)
        {
            char buf[1024];
            sprintf(buf, "Found animation for %s: %d posKeys -- %d rotKeys -- %d scaleKeys",
                    nodeAnim->mNodeName.data,
                    nodeAnim->mNumPositionKeys,
                    nodeAnim->mNumRotationKeys,
                    nodeAnim->mNumScalingKeys);
            LOG_TRACE << buf;
            
            gl::AnimationKeys animKeys;
            vec3 bonePosition;
            vec3 boneScale;
            quat boneRotation;
            
            for (uint32_t i = 0; i < nodeAnim->mNumRotationKeys; i++)
            {
                aiQuaternion rot = nodeAnim->mRotationKeys[i].mValue;
                boneRotation = quat(rot.w, rot.x, rot.y, rot.z);
                animKeys.rotationkeys.push_back(gl::Key<quat>(nodeAnim->mRotationKeys[i].mTime,
                                                              boneRotation));
            }
            
            for (uint32_t i = 0; i < nodeAnim->mNumPositionKeys; i++)
            {
                aiVector3D pos = nodeAnim->mPositionKeys[i].mValue;
                bonePosition = vec3(pos.x, pos.y, pos.z);
                animKeys.positionkeys.push_back(gl::Key<vec3>(nodeAnim->mPositionKeys[i].mTime,
                                                              bonePosition));
            }
            
            for (uint32_t i = 0; i < nodeAnim->mNumScalingKeys; i++)
            {
                aiVector3D scaleTmp = nodeAnim->mScalingKeys[i].mValue;
                boneScale = vec3(scaleTmp.x, scaleTmp.y, scaleTmp.z);
                animKeys.scalekeys.push_back(gl::Key<vec3>(nodeAnim->mScalingKeys[i].mTime,
                                                           boneScale));
            }
            outAnim.boneKeys[bone] = animKeys;
        }
        
        for (uint32_t i = 0 ; i < theNode->mNumChildren ; i++)
        {
            create_bone_animation(theNode->mChildren[i], theAnimation, root_bone, outAnim);
        }
    }

/////////////////////////////////////////////////////////////////
    
    BonePtr find_bone_by_name(const std::string &the_name, BonePtr the_root)
    {
        if(!the_root || the_name == the_root->name){ return the_root; }
        else
        {
            for(BonePtr child_bone : the_root->children)
            {
                auto b = find_bone_by_name(the_name, child_bone);
                if(b){ return b; }
            }
        }
        return BonePtr();
    }
    
/////////////////////////////////////////////////////////////////
    
    void get_node_transform(const aiNode *the_node, mat4 &the_transform)
    {
        if(the_node)
        {
            the_transform *= aimatrix_to_glm_mat4(the_node->mTransformation);
            
            for (uint32_t i = 0 ; i < the_node->mNumChildren ; i++)
            {
                get_node_transform(the_node->mChildren[i], the_transform);
            }
        }
    }
    
/////////////////////////////////////////////////////////////////
    
    size_t AssimpConnector::add_animations_to_mesh(const std::string &thePath,
                                                   MeshPtr m)
    {
        LOG_TRACE << "loading animations from '" << thePath << "' ...";
        
        Assimp::Importer importer;
        std::string found_path;
        const aiScene *theScene = nullptr;
        
        try { theScene = importer.ReadFile(search_file(thePath), 0); }
        catch (FileNotFoundException &e)
        {
            LOG_WARNING << e.what();
            return 0;
        }
        
        if(theScene && m)
        {
            for(uint32_t i = 0; i < theScene->mNumAnimations; i++)
            {
                aiAnimation *assimpAnimation = theScene->mAnimations[i];
                MeshAnimation anim;
                anim.duration = assimpAnimation->mDuration;
                anim.ticksPerSec = assimpAnimation->mTicksPerSecond;
                create_bone_animation(theScene->mRootNode, assimpAnimation, m->rootBone(), anim);
                m->addAnimation(anim);
            }
        }
        return theScene ? theScene->mNumAnimations : 0;
    }
}}//namespace