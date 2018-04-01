//
//  AssimpConnector.cpp
//  gl
//
//  Created by Fabian on 12/15/12.
//
//

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include "core/file_functions.hpp"
#include "gl/Mesh.hpp"
#include "gl/Scene.hpp"
#include "assimp.hpp"

using namespace std;
using namespace glm;

namespace kinski { namespace assimp{
    
typedef map<std::string, pair<int, mat4>> BoneMap;
typedef map<uint32_t, list< pair<uint32_t, float>>> WeightMap;
    
/////////////////////////////////////////////////////////////////
    
void mergeGeometries(gl::GeometryPtr src, gl::GeometryPtr dst);

gl::GeometryPtr createGeometry(const aiMesh *aMesh, const aiScene *theScene);

gl::MaterialPtr createMaterial(const aiMaterial *mtl);

void loadBones(const aiMesh *aMesh, uint32_t base_vertex, BoneMap& bonemap, WeightMap &weightmap);

void insertBoneVertexData(gl::GeometryPtr geom, const WeightMap &weightmap, uint32_t start_index = 0);


gl::BonePtr create_bone_hierarchy(const aiNode *theNode, const gl::mat4 &parentTransform,
                                  const map<std::string, pair<int, gl::mat4> > &boneMap,
                                  gl::BonePtr parentBone = gl::BonePtr());

gl::BonePtr find_bone_by_name(const std::string &the_name, gl::BonePtr the_root);

void create_bone_animation(const aiNode *theNode, const aiAnimation *theAnimation,
                           gl::BonePtr root_bone, gl::MeshAnimation &outAnim);

//void get_node_transform(const aiNode *the_node, mat4 &the_transform);

void process_node(const aiScene *the_scene, const aiNode *the_in_node, const gl::Object3DPtr &the_parent_node);
    
/////////////////////////////////////////////////////////////////
    
inline mat4 aimatrix_to_glm_mat4(aiMatrix4x4 theMat)
{
    mat4 ret;
    memcpy(&ret[0][0], theMat.Transpose()[0], sizeof(ret));
    return ret;
}

/////////////////////////////////////////////////////////////////

inline glm::vec3 aivector_to_glm_vec3(const aiVector3D &the_vec)
{
    glm::vec3 ret;
    for(int i = 0; i < 3; ++i){ ret[i] = the_vec[i]; }
    return ret;
}
    
/////////////////////////////////////////////////////////////////
    
inline gl::Color aicolor_convert(const aiColor4D &the_color)
{
    gl::Color ret;
    for(int i = 0; i < 4; ++i){ ret[i] = the_color[i]; }
    return ret;
}
 
/////////////////////////////////////////////////////////////////

inline gl::Color aicolor_convert(const aiColor3D &the_color)
{
    gl::Color ret;
    for(int i = 0; i < 3; ++i){ ret[i] = the_color[i]; }
    return ret;
}
    
/////////////////////////////////////////////////////////////////
    
gl::GeometryPtr createGeometry(const aiMesh *aMesh, const aiScene *theScene)
{
    gl::GeometryPtr geom = gl::Geometry::create();
    
    geom->vertices().insert(geom->vertices().end(), (vec3*)aMesh->mVertices,
                            (vec3*)aMesh->mVertices + aMesh->mNumVertices);
    
    if(aMesh->HasTextureCoords(0))
    {
        geom->tex_coords().reserve(aMesh->mNumVertices);
        for (uint32_t i = 0; i < aMesh->mNumVertices; i++)
        {
            geom->append_tex_coord(aMesh->mTextureCoords[0][i].x, aMesh->mTextureCoords[0][i].y);
        }
    }else
    {
        geom->tex_coords().resize(aMesh->mNumVertices, vec2(0));
    }
    
    std::vector<gl::index_t> &indices = geom->indices(); indices.reserve(aMesh->mNumFaces * 3);
    for(uint32_t i = 0; i < aMesh->mNumFaces; ++i)
    {
        const aiFace &f = aMesh->mFaces[i];
        if(f.mNumIndices != 3) throw Exception("Non triangle mesh loaded");
        indices.insert(indices.end(), f.mIndices, f.mIndices + 3);
    }
    geom->faces().resize(aMesh->mNumFaces);
    ::memcpy(&geom->faces()[0], &indices[0], indices.size() * sizeof(gl::index_t));
    
    if(aMesh->HasNormals())
    {
        geom->normals().insert(geom->normals().end(), (vec3*)aMesh->mNormals,
                               (vec3*) aMesh->mNormals + aMesh->mNumVertices);
    }
    else
    {
        geom->compute_vertex_normals();
    }
    
    if(aMesh->HasVertexColors(0))//TODO: test
    {
        geom->colors().insert(geom->colors().end(), (vec4*)aMesh->mColors,
                              (vec4*) aMesh->mColors + aMesh->mNumVertices);
    }
//        else{ geom->colors().resize(aMesh->mNumVertices, gl::COLOR_WHITE); }
    
    if(aMesh->HasTangentsAndBitangents())
    {
        geom->tangents().insert(geom->tangents().end(), (vec3*)aMesh->mTangents,
                                (vec3*) aMesh->mTangents + aMesh->mNumVertices);
    }
    else
    {
        // compute tangents
        geom->compute_tangents();
    }
    geom->compute_aabb();
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
    
void insertBoneVertexData(gl::GeometryPtr geom, const WeightMap &weightmap, uint32_t start_index)
{
    if(weightmap.empty()) return;
    
    // allocate storage for indices and weights
    geom->bone_vertex_data().resize(geom->vertices().size());
    
    for (WeightMap::const_iterator it = weightmap.begin(); it != weightmap.end(); ++it)
    {
        gl::BoneVertexData &boneData = geom->bone_vertex_data()[it->first + start_index];
        uint32_t i = 0, max_num_weights = boneData.indices.length();
        
        list< pair<uint32_t, float> > tmp_list(it->second.begin(), it->second.end());
        tmp_list.sort([](const pair<uint32_t, float> &lhs,
                         const pair<uint32_t, float> &rhs)
        {
            return lhs.second > rhs.second;
        });
        
        list< pair<uint32_t, float> >::const_iterator listIt = tmp_list.begin();
        for (; listIt != tmp_list.end(); ++listIt)
        {
            if(i >= max_num_weights) break;
            boneData.indices[i] = listIt->first;
            boneData.weights[i] = listIt->second;
            i++;
        }
    }
}

/////////////////////////////////////////////////////////////////

gl::LightPtr create_light(const aiLight* the_light)
{
    gl::Light::Type t = gl::Light::UNKNOWN;
    
    switch (the_light->mType)
    {
        case aiLightSource_DIRECTIONAL:
            t = gl::Light::DIRECTIONAL;
            break;
        case aiLightSource_SPOT:
            t = gl::Light::SPOT;
            break;
        case aiLightSource_POINT:
            t = gl::Light::POINT;
            break;
//        case aiLightSource_AREA:
//            t = gl::Light::AREA;
//            break;
        default:
            break;
    }
    auto l = gl::Light::create(t);
    l->set_name(the_light->mName.data);
    l->set_spot_cutoff(the_light->mAngleOuterCone);
    l->set_diffuse(aicolor_convert(the_light->mColorDiffuse));
    l->set_ambient(aicolor_convert(the_light->mColorAmbient));
    l->set_specular(aicolor_convert(the_light->mColorSpecular));
    l->set_attenuation(the_light->mAttenuationConstant, the_light->mAttenuationLinear,
                       the_light->mAttenuationQuadratic);

    auto pos = aivector_to_glm_vec3(the_light->mPosition);
    auto y_axis = gl::Y_AXIS;//glm::normalize(aivector_to_glm_vec3(the_light->mUp));
    auto z_axis = glm::normalize(-aivector_to_glm_vec3(the_light->mDirection));
    auto x_axis = glm::normalize(glm::cross(z_axis, y_axis));
    glm::mat4 m(vec4(x_axis, 0.f), vec4(y_axis, 0.f), vec4(z_axis, 0.f), vec4(pos, 1.f));
    l->set_transform(m);
    return l;
}

/////////////////////////////////////////////////////////////////
    
gl::CameraPtr create_camera(const aiCamera* the_cam)
{
    auto ret = gl::PerspectiveCamera::create();
    ret->set_name(the_cam->mName.data);
    ret->set_fov(the_cam->mHorizontalFOV);
    ret->set_aspect(the_cam->mAspect);
    ret->set_clipping(the_cam->mClipPlaneNear, the_cam->mClipPlaneFar);
    auto pos = aivector_to_glm_vec3(the_cam->mPosition);
    auto y_axis = glm::normalize(aivector_to_glm_vec3(the_cam->mUp));
    auto z_axis = glm::normalize(-aivector_to_glm_vec3(the_cam->mLookAt));
    auto x_axis = glm::normalize(glm::cross(z_axis, y_axis));
    glm::mat4 m(vec4(x_axis, 0.f), vec4(y_axis, 0.f), vec4(z_axis, 0.f), vec4(pos, 1.f));
    ret->set_transform(m);
    return ret;
}
    
/////////////////////////////////////////////////////////////////
    
gl::MaterialPtr createMaterial(const aiMaterial *mtl)
{
    gl::MaterialPtr theMaterial = gl::Material::create();
    theMaterial->set_blending(true);
    int ret1, ret2;
    aiColor4D c;
    float shininess, strength;
    int two_sided;
    int wireframe;
    aiString texPath;
    
    if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &c))
    {
        auto col = aicolor_convert(c);
        col.a = 1.f;
        theMaterial->set_diffuse(col);
    }
    
    if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &c))
    {
        //TODO: introduce cavity param!?
    }
    
    if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &c))
    {
        theMaterial->set_ambient(aicolor_convert(c));
    }
    
    if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &c))
    {
        theMaterial->set_emission(aicolor_convert(c));
    }
    
    // transparent material
    if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_TRANSPARENT, &c))
    {
        //TODO: needed ?
    }
//        float opacity = 1.f;
//        mtl->Get(AI_MATKEY_OPACITY, opacity);
//        if(opacity < 1.f)
//        {
//            Color d = theMaterial->diffuse();
//            d.a = opacity;
//            theMaterial->setDiffuse(d);
//            theMaterial->setBlending();
//            //theMaterial->setDepthWrite(false);
//        }
    
    ret1 = aiGetMaterialFloat(mtl, AI_MATKEY_SHININESS, &shininess);
    ret2 = aiGetMaterialFloat(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength);
    float roughness = 1.f;
    
    if((ret1 == AI_SUCCESS) && (ret2 == AI_SUCCESS))
    {
        roughness = 1.f - clamp(shininess * strength / 80.f, 0.f, 1.f);
    }
    theMaterial->set_roughness(roughness);
    
    if(AI_SUCCESS == aiGetMaterialInteger(mtl, AI_MATKEY_ENABLE_WIREFRAME, &wireframe))
        theMaterial->set_wireframe(wireframe);
    
    if((AI_SUCCESS == aiGetMaterialInteger(mtl, AI_MATKEY_TWOSIDED, &two_sided)))
        theMaterial->set_two_sided(two_sided);
    
    // DIFFUSE
    if(AI_SUCCESS == mtl->GetTexture(aiTextureType(aiTextureType_DIFFUSE), 0, &texPath))
    {
        theMaterial->enqueue_texture(string(texPath.data), (uint32_t)gl::Material::TextureType::COLOR);
    }
    
    // EMISSION
    if(AI_SUCCESS == mtl->GetTexture(aiTextureType(aiTextureType_EMISSIVE), 0, &texPath))
    {
        theMaterial->enqueue_texture(string(texPath.data), (uint32_t)gl::Material::TextureType::EMISSION);
    }
    
    // ROUGHNESS / SHINYNESS
    if(AI_SUCCESS == mtl->GetTexture(aiTextureType(aiTextureType_SHININESS), 0, &texPath))
    {
        theMaterial->enqueue_texture(string(texPath.data), (uint32_t)gl::Material::TextureType::ROUGHNESS);
    }
    
    if(AI_SUCCESS == mtl->GetTexture(aiTextureType(aiTextureType_NORMALS), 0, &texPath))
    {
        LOG_DEBUG << "adding normalmap: '" << string(texPath.data) << "'";
        theMaterial->enqueue_texture(string(texPath.data), (uint32_t)gl::Material::TextureType::NORMAL);
    }
    return theMaterial;
}

/////////////////////////////////////////////////////////////////
    
void mergeGeometries(gl::GeometryPtr src, gl::GeometryPtr dst)
{
    dst->append_vertices(src->vertices());
    dst->append_normals(src->normals());
    dst->append_colors(src->colors());
    dst->tangents().insert(dst->tangents().end(), src->tangents().begin(), src->tangents().end());
    dst->append_tex_coords(src->tex_coords());
    dst->append_indices(src->indices());
    dst->bone_vertex_data().insert(dst->bone_vertex_data().end(), src->bone_vertex_data().begin(),
                                 src->bone_vertex_data().end());
    dst->faces().insert(dst->faces().end(), src->faces().begin(), src->faces().end());
}

/////////////////////////////////////////////////////////////////
    
gl::MeshPtr load_model(const std::string &theModelPath)
{
    Assimp::Importer importer;
    std::string found_path;
    try { found_path = fs::search_file(theModelPath); }
    catch(fs::FileNotFoundException &e)
    {
        LOG_ERROR << e.what();
        return gl::MeshPtr();
    }
    load_scene(theModelPath);

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
        
        uint32_t current_base_index = 0, current_base_vertex = 0;
        gl::GeometryPtr combined_geom = gl::Geometry::create();
        BoneMap bonemap;
        WeightMap weightmap;
        std::vector<gl::Mesh::Entry> entries;
        
        for (uint32_t i = 0; i < theScene->mNumMeshes; i++)
        {
            aiMesh *aMesh = theScene->mMeshes[i];
            gl::GeometryPtr g = createGeometry(aMesh, theScene);
            loadBones(aMesh, current_base_vertex, bonemap, weightmap);
            gl::Mesh::Entry m;
            m.num_vertices = g->vertices().size();
            m.num_indices = g->indices().size();
            m.base_index = current_base_index;
            m.base_vertex = current_base_vertex;
            m.material_index = aMesh->mMaterialIndex;
            entries.push_back(m);
            current_base_vertex += g->vertices().size();
            current_base_index += g->indices().size();
            
            geometries.push_back(g);
            mergeGeometries(g, combined_geom);
            materials[aMesh->mMaterialIndex] = createMaterial(theScene->mMaterials[aMesh->mMaterialIndex]);
        }
        combined_geom->compute_aabb();
        
        // insert colors, if not present
        combined_geom->colors().resize(combined_geom->vertices().size(), gl::COLOR_WHITE);
        
        insertBoneVertexData(combined_geom, weightmap);
        
        gl::GeometryPtr geom = combined_geom;
        gl::MeshPtr mesh = gl::Mesh::create(combined_geom, materials[0]);
        mesh->entries() = entries;
        mesh->materials() = materials;
        mesh->set_root_bone(create_bone_hierarchy(theScene->mRootNode, mat4(), bonemap));
        
        for (uint32_t i = 0; i < theScene->mNumAnimations; i++)
        {
            aiAnimation *assimpAnimation = theScene->mAnimations[i];
            gl::MeshAnimation anim;
            anim.duration = assimpAnimation->mDuration;
            anim.ticks_per_sec = assimpAnimation->mTicksPerSecond;
            create_bone_animation(theScene->mRootNode, assimpAnimation, mesh->root_bone(), anim);
            mesh->add_animation(anim);
        }
        gl::ShaderType sh_type;
        
        try
        {
            if(geom->has_bones()){ sh_type = gl::ShaderType::PHONG_SKIN; }
            else{ sh_type = gl::ShaderType::PHONG; }
            
        }catch (std::exception &e){ LOG_WARNING<<e.what(); }
        
        for(uint32_t i = 0; i < materials.size(); i++)
        {
            materials[i]->enqueue_shader(sh_type);
        }
        LOG_DEBUG<<"loaded model: "<<geom->vertices().size()<<" vertices - " <<
        geom->faces().size()<<" faces - "<< mesh->get_num_bones(mesh->root_bone()) << " bones";
        LOG_DEBUG<<"bounds: " <<to_string(mesh->aabb().min)<<" - "<< to_string(mesh->aabb().max);
        importer.FreeScene();
        return mesh;
    }
    else
    {
        return gl::MeshPtr();
    }
}

/////////////////////////////////////////////////////////////////

gl::ScenePtr load_scene(const std::string &the_path)
{
    gl::ScenePtr ret;

    Assimp::Importer importer;
    std::string found_path;
    try { found_path = fs::search_file(the_path); }
    catch(fs::FileNotFoundException &e)
    {
        LOG_ERROR << e.what();
        return ret;
    }
    LOG_DEBUG << "loading scene '" << the_path << "' ...";
    const aiScene *in_scene = importer.ReadFile(found_path, 0);

    // super useful postprocessing steps
    in_scene = importer.ApplyPostProcessing(aiProcess_Triangulate
                                            | aiProcess_GenSmoothNormals
                                            | aiProcess_JoinIdenticalVertices
                                            | aiProcess_CalcTangentSpace
                                            | aiProcess_LimitBoneWeights);

    if(in_scene)
    {
        ret = gl::Scene::create();

        LOG_DEBUG << "num lights: " << in_scene->mNumLights;
        LOG_DEBUG << "num cams: " << in_scene->mNumCameras;

        for(uint32_t i = 0; i < in_scene->mNumLights; ++i)
        {
            ret->add_object(create_light(in_scene->mLights[i]));
        }


        for(uint32_t i = 0; i < in_scene->mNumCameras; ++i)
        {
//            aiCamera* cam = in_scene->mCameras[i];
        }

//        process_node(in_scene, in_scene->mRootNode, ret->root());
        importer.FreeScene();
    }
    return ret;
}

/////////////////////////////////////////////////////////////////

void process_node(const aiScene *the_scene, const aiNode *the_in_node, const gl::Object3DPtr &the_parent_node)
{
    if(!the_in_node){ return; }

    string node_name(the_in_node->mName.data);
    mat4 node_transform = aimatrix_to_glm_mat4(the_in_node->mTransformation);

    const aiNode* p = the_in_node->mParent;
    string parent_string;

    while(p && p != the_scene->mRootNode)
    { parent_string = string(p->mName.data) + " -> " + parent_string; p = p->mParent; }

    LOG_DEBUG << "node: " << parent_string + node_name << " -- num meshes: " << the_in_node->mNumMeshes;

    // meshes assigned to this node
    for (uint32_t  n = 0; n < the_in_node->mNumMeshes; ++n)
    {
        const aiMesh *mesh = the_scene->mMeshes[the_in_node->mMeshes[n]];
    }

    for(uint32_t i = 0; i < the_in_node->mNumChildren; ++i)
    {
        process_node(the_scene, the_in_node->mChildren[i], the_parent_node);
    }
}

/////////////////////////////////////////////////////////////////

gl::BonePtr create_bone_hierarchy(const aiNode *theNode, const gl::mat4 &parentTransform,
                                  const map<std::string, pair<int, gl::mat4>> &boneMap,
                                  gl::BonePtr parentBone)
{
    gl::BonePtr currentBone;
    string nodeName(theNode->mName.data);
    mat4 nodeTransform = aimatrix_to_glm_mat4(theNode->mTransformation);
    
    mat4 globalTransform = parentTransform * nodeTransform;
    BoneMap::const_iterator it = boneMap.find(nodeName);
    
    // current node corresponds to a bone
    if (it != boneMap.end())
    {
        int boneIndex = it->second.first;
        const mat4 &offset = it->second.second;
        currentBone = std::make_shared<gl::Bone>();
        currentBone->name = nodeName;
        currentBone->index = boneIndex;
        currentBone->transform = nodeTransform;
        currentBone->worldtransform = globalTransform;
        currentBone->offset = offset;
        currentBone->parent = parentBone;
    }
    
    for(uint32_t i = 0 ; i < theNode->mNumChildren ; i++)
    {
        gl::BonePtr child = create_bone_hierarchy(theNode->mChildren[i], globalTransform,
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
                           gl::BonePtr root_bone, gl::MeshAnimation &outAnim)
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

    gl::BonePtr bone = find_bone_by_name(nodeName, root_bone);
    
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
        outAnim.bone_keys[bone] = animKeys;
    }
    
    for (uint32_t i = 0 ; i < theNode->mNumChildren ; i++)
    {
        create_bone_animation(theNode->mChildren[i], theAnimation, root_bone, outAnim);
    }
}

/////////////////////////////////////////////////////////////////
    
gl::BonePtr find_bone_by_name(const std::string &the_name, gl::BonePtr the_root)
{
    if(!the_root || the_name == the_root->name){ return the_root; }
    else
    {
        for(gl::BonePtr child_bone : the_root->children)
        {
            auto b = find_bone_by_name(the_name, child_bone);
            if(b){ return b; }
        }
    }
    return gl::BonePtr();
}
    
/////////////////////////////////////////////////////////////////
    
//void get_node_transform(const aiNode *the_node, mat4 &the_transform)
//{
//    if(the_node)
//    {
//        the_transform *= aimatrix_to_glm_mat4(the_node->mTransformation);
//
//        for (uint32_t i = 0 ; i < the_node->mNumChildren ; i++)
//        {
//            get_node_transform(the_node->mChildren[i], the_transform);
//        }
//    }
//}
    
/////////////////////////////////////////////////////////////////
    
size_t add_animations_to_mesh(const std::string &thePath, gl::MeshPtr m)
{
    LOG_TRACE << "loading animations from '" << thePath << "' ...";
    
    Assimp::Importer importer;
    std::string found_path;
    const aiScene *theScene = nullptr;
    
    try { theScene = importer.ReadFile(fs::search_file(thePath), 0); }
    catch (fs::FileNotFoundException &e)
    {
        LOG_WARNING << e.what();
        return 0;
    }
    
    if(theScene && m)
    {
        for(uint32_t i = 0; i < theScene->mNumAnimations; i++)
        {
            aiAnimation *assimpAnimation = theScene->mAnimations[i];
            gl::MeshAnimation anim;
            anim.duration = assimpAnimation->mDuration;
            anim.ticks_per_sec = assimpAnimation->mTicksPerSecond;
            create_bone_animation(theScene->mRootNode, assimpAnimation, m->root_bone(), anim);
            m->add_animation(anim);
        }
    }
    return theScene ? theScene->mNumAnimations : 0;
}
    
}}//namespace
