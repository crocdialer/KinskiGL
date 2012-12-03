#include "kinskiApp/App.h"
#include "kinskiApp/TextureIO.h"

#include "kinskiCV/CVThread.h"

#include "kinskiGL/SerializerGL.h"
#include "kinskiGL/Scene.h"
#include "kinskiGL/Mesh.h"
#include "kinskiGL/Fbo.h"

#include <assimp/assimp.hpp>
#include <assimp/aiScene.h>
#include <assimp/aiPostProcess.h>


using namespace std;
using namespace kinski;
using namespace glm;

class SimpleGeometryApp : public App
{
private:
    
    gl::Texture m_noiseTexture;
    
    gl::Material::Ptr m_material, m_pointMaterial;
    gl::Geometry::Ptr m_geometry;
    gl::Geometry::Ptr m_straightPlane;
    
    gl::Mesh::Ptr m_mesh;
    gl::PerspectiveCamera::Ptr m_Camera;
    gl::Scene m_scene;
    
    RangedProperty<float>::Ptr m_distance;
    RangedProperty<float>::Ptr m_textureMix;
    Property_<string>::Ptr m_modelPath;
    Property_<bool>::Ptr m_wireFrame;
    Property_<bool>::Ptr m_drawNormals;
    Property_<glm::vec3>::Ptr m_lightDir;
    
    Property_<glm::vec4>::Ptr m_color;
    Property_<glm::mat3>::Ptr m_rotation;
    RangedProperty<float>::Ptr m_rotationSpeed;
    RangedProperty<float>::Ptr m_simplexDim;
    RangedProperty<float>::Ptr m_simplexSpeed;
    
    // opencv interface
    CVThread::Ptr m_cvThread;
    
    // mouse rotation control
    vec2 m_clickPos;
    mat4 m_lastTransform;
    float m_lastDistance;
    
    glm::mat4 aiMatrixToGlmMat(aiMatrix4x4 theMat)
    {
        glm::mat4 ret;
        memcpy(&ret[0][0], theMat.Transpose()[0], 16 * sizeof(float));
        return ret;
    }
    
    gl::Mesh::Ptr loadModel(const std::string &theModelPath)
    {
        Assimp::Importer importer;
        importer.ReadFile(theModelPath, 0);
        const aiScene *theScene = importer.ApplyPostProcessing(aiProcess_Triangulate
                                                               | aiProcess_GenSmoothNormals
                                                               | aiProcess_CalcTangentSpace);
        
        if (theScene)
        {
            //aiNode *root = theScene->mRootNode;
            
            aiMesh *aMesh = theScene->mMeshes[0];
            
            gl::Geometry::Ptr geom( createGeometry(aMesh, theScene) );
            gl::Material::Ptr mat = createMaterial(theScene->mMaterials[aMesh->mMaterialIndex]);
            mat->getShader() = m_material->getShader();
            
            gl::Mesh::Ptr mesh(new gl::Mesh(geom, mat));
            
            importer.FreeScene();
            
            return mesh;
        }
        else
        {
            throw Exception("could not load model: "+ theModelPath);
        }
    }
    
    void traverseNodes(const aiAnimation *theAnimation,
                       int frameIndex,
                       const aiNode *theNode,
                       const glm::mat4 &parentTransform,
                       const map<std::string, pair<int, mat4> > &boneMap,
                       shared_ptr<gl::Animation> &outAnim)
    {
        string nodeName(theNode->mName.data);
        
        glm::mat4 nodeTransform = aiMatrixToGlmMat(theNode->mTransformation);
        
        const aiNodeAnim* nodeAnim = NULL;
        float timeStamp = 0.0f;
        
        for (int i = 0; i < theAnimation->mNumChannels; i++)
        {
            aiNodeAnim *ptr = theAnimation->mChannels[i];
            
            if(string(ptr->mNodeName.data) == nodeName)
            {
                nodeAnim = ptr;
                break;
            }
        }
        
        if(nodeAnim)
        {
//            printf("Found animation for %s: %d posKeys -- %d rotKeys -- %d scaleKeys\n",
//                   nodeAnim->mNodeName.data,
//                   nodeAnim->mNumPositionKeys,
//                   nodeAnim->mNumRotationKeys,
//                   nodeAnim->mNumScalingKeys);
            
            outAnim->frames.reserve(nodeAnim->mNumRotationKeys);
            
            timeStamp = nodeAnim->mRotationKeys[frameIndex].mTime;
            
            assert(timeStamp == nodeAnim->mPositionKeys[frameIndex].mTime &&
                   timeStamp == nodeAnim->mScalingKeys[frameIndex].mTime);
            
            aiVector3D pos = nodeAnim->mPositionKeys[frameIndex].mValue;
            mat4 translate = glm::translate(mat4(), vec3(pos.x, pos.y, pos.z));
            
            aiQuaternion rot = nodeAnim->mRotationKeys[frameIndex].mValue;
            mat4 rotate = glm::mat4_cast(glm::quat(rot.w, rot.x, rot.y, rot.z));
            
            aiVector3D scaleTmp = nodeAnim->mScalingKeys[frameIndex].mValue;
            mat4 scale = glm::scale(mat4(), vec3(scaleTmp.x, scaleTmp.y, scaleTmp.z));
            
            nodeTransform = translate * rotate * scale;
        }
        
        mat4 globalTransform = parentTransform * nodeTransform;
        
        map<std::string, pair<int, mat4> >::const_iterator it = boneMap.find(nodeName);
        if (it != boneMap.end())
        {
            int boneIndex = it->second.first;
            const mat4 &offset = it->second.second;
            
            while (outAnim->frames.size() <= frameIndex)
                outAnim->frames.push_back(gl::AnimationFrame());
            
            gl::AnimationFrame &animFrame = outAnim->frames[frameIndex];
            animFrame.time = timeStamp;
            
            while (animFrame.boneTransforms.size() <= boneIndex)
                animFrame.boneTransforms.push_back(mat4());
            
            animFrame.boneTransforms[boneIndex] = globalTransform * offset;
        }

        for (int i = 0 ; i < theNode->mNumChildren ; i++)
        {
            traverseNodes(theAnimation, frameIndex, theNode->mChildren[i],
                          globalTransform, boneMap, outAnim);
        }
    }
    
    gl::Material::Ptr createMaterial(const aiMaterial *mtl)
    {
        gl::Material::Ptr theMaterial(new gl::Material);
        
        int ret1, ret2;
        struct aiColor4D diffuse;
        struct aiColor4D specular;
        struct aiColor4D ambient;
        struct aiColor4D emission;
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
                theMaterial->addTexture(gl::TextureIO::loadTexture(string(texPath.data)));
            }
        }
        
        return theMaterial;
    }
    
    gl::Geometry::Ptr createGeometry(const aiMesh *aMesh, const aiScene *theScene = NULL)
    {
        gl::Geometry::Ptr geom (new gl::Geometry);
        
        geom->getVertices().reserve(aMesh->mNumVertices);
        geom->getVertices().insert(geom->getVertices().end(), (glm::vec3*)aMesh->mVertices,
                                   (glm::vec3*)aMesh->mVertices + aMesh->mNumVertices);
        
        
        if(aMesh->HasTextureCoords(0))
        {
            geom->getTexCoords().reserve(aMesh->mNumVertices);
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
            geom->getNormals().reserve(aMesh->mNumVertices);
            geom->getNormals().insert(geom->getNormals().end(), (glm::vec3*)aMesh->mNormals,
                                      (glm::vec3*) aMesh->mNormals + aMesh->mNumVertices);
        }
        else
        {
            geom->computeVertexNormals();
        }
        
        if(aMesh->HasTangentsAndBitangents())
        {
            geom->getTangents().reserve(aMesh->mNumVertices);
            geom->getTangents().insert(geom->getTangents().end(), (glm::vec3*)aMesh->mTangents,
                                      (glm::vec3*) aMesh->mTangents + aMesh->mNumVertices);
        }
        else
        {
            // compute tangents
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
            for (int i = 0; i < geom->getVertices().size(); ++i)
            {
                geom->getBoneData().push_back(gl::BoneVertexData());
            }
            
            for (WeightMap::iterator it = weightMap.begin(); it != weightMap.end(); ++it)
            {
                int i = 0;
                gl::BoneVertexData &boneData = geom->getBoneData()[it->first];
                list< pair<uint32_t, float> >::iterator pairIt = it->second.begin();
                for (; pairIt != it->second.end(); ++pairIt)
                {
                    boneData.indices[i] = pairIt->first;
                    boneData.weights[i] = pairIt->second;
                    i++;
                }
            }
            
            if(theScene)
            {
                aiAnimation *assimpAnimation = theScene->mAnimations[0];
                shared_ptr<gl::Animation> anim(new gl::Animation);
                anim->duration = assimpAnimation->mDuration;
                anim->ticksPerSec = assimpAnimation->mTicksPerSecond;
                
                int numFrames = assimpAnimation->mChannels[0]->mNumRotationKeys;
                for (int i = 0; i < numFrames; ++i)
                {
                    // traverse aiScene and construct final transforms
                    traverseNodes(assimpAnimation, i, theScene->mRootNode, mat4(),
                                  boneMap, anim);
                }
                
                geom->setAnimation(anim);
                geom->boneMatrices() = anim->frames[0].boneTransforms;
            }
        }
        geom->computeBoundingBox();
        
        return geom;
    }

public:
    
    void setup()
    {
        /*********** init our application properties ******************/
        
        m_distance = RangedProperty<float>::create("view distance", 25, 0, 5000);
        registerProperty(m_distance);
        
        m_textureMix = RangedProperty<float>::create("texture mix ratio", 0.2, 0, 1);
        registerProperty(m_textureMix);
        
        m_modelPath = Property_<string>::create("Model path", "duck.dae");
        registerProperty(m_modelPath);
        
        m_wireFrame = Property_<bool>::create("Wireframe", false);
        registerProperty(m_wireFrame);
        
        m_drawNormals = Property_<bool>::create("Normals", false);
        registerProperty(m_drawNormals);
        
        m_lightDir = Property_<vec3>::create("Light dir", vec3(1));
        registerProperty(m_lightDir);
        
        m_color = Property_<glm::vec4>::create("Material color", glm::vec4(1 ,1 ,0, 0.6));
        registerProperty(m_color);
        
        m_rotation = Property_<glm::mat3>::create("Geometry Rotation", glm::mat3());
        registerProperty(m_rotation);
        
        m_rotationSpeed = RangedProperty<float>::create("Rotation Speed", 0, -100, 100);
        registerProperty(m_rotationSpeed);
        
        m_simplexDim = RangedProperty<float>::create("Simplex Resolution", 1/8.f, 0, 2);
        registerProperty(m_simplexDim);
        
        m_simplexSpeed = RangedProperty<float>::create("Simplex Speed", .5, 0, 5);
        registerProperty(m_simplexSpeed);
        
        // add properties
        addPropertyListToTweakBar(getPropertyList());
        
        setBarColor(vec4(0, 0 ,0 , .5));
        setBarSize(ivec2(250, 500));

        // enable observer mechanism
        observeProperties();
        
        /********************** construct a simple scene ***********************/
        
        m_material = gl::Material::Ptr(new gl::Material);
        m_material->uniform("u_textureMix", m_textureMix->val());
        m_material->setDiffuse(m_color->val());
        m_material->addTexture(gl::TextureIO::loadTexture("/Users/Fabian/Pictures/artOfNoise.png"));
        m_material->addTexture(gl::TextureIO::loadTexture("/Users/Fabian/Pictures/David_Jien_02.png"));
        //m_material->setBlending();
        //m_material->setTwoSided();
        
        m_pointMaterial = gl::Material::Ptr(new gl::Material);
        m_pointMaterial->addTexture(gl::TextureIO::loadTexture("smoketex.png"));
        m_pointMaterial->setPointSize(30.f);
        m_pointMaterial->setBlending();
        
        try
        {
            m_material->getShader().loadFromFile("shader_phong_skin.vert", "shader_phong.frag");
        }catch (std::exception &e)
        {
            fprintf(stderr, "%s\n",e.what());
        }
        
        m_Camera = gl::PerspectiveCamera::Ptr(new gl::PerspectiveCamera);
        m_Camera->setClippingPlanes(.1, 5000);
        
//        {
//            int w = 1024, h = 1024;
//            float data[w * h];
//            
//            for (int i = 0; i < h; i++)
//                for (int j = 0; j < w; j++)
//                {
//                    data[i * h + j] = (glm::simplex( vec3( m_simplexDim->val() * vec2(i ,j),
//                                                           m_simplexSpeed->val() * 0.5)) + 1) / 2.f;
//                }
//            
//            m_noiseTexture.update(data, GL_RED, w, h, true);
//        }
        
//        m_cvThread = CVThread::Ptr(new CVThread());
//        m_cvThread->streamUSBCamera();
        
        // load state from config file
        try
        {
            Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
        }catch(FileNotFoundException &e)
        {
            printf("%s\n", e.what());
        }
    }
    
    void update(const float timeDelta)
    {
        *m_rotation = mat3( glm::rotate(mat4(m_rotation->val()),
                                        m_rotationSpeed->val() * timeDelta,
                                        vec3(0, 1, .5)));
        
        if(m_mesh)
        {
            m_mesh->getGeometry()->updateAnimation(getApplicationTime());
        }
    }
    
    void draw()
    {
        gl::Material cloneMat1 = *m_material;
        cloneMat1.setDepthWrite(false);
        cloneMat1.setBlending(false);
        cloneMat1.setWireframe(false);
        
        //gl::drawQuad(cloneMat1, getWindowSize() / 1.2f);
        gl::drawTexture(cloneMat1.getTextures()[0], getWindowSize());

        gl::loadMatrix(gl::PROJECTION_MATRIX, m_Camera->getProjectionMatrix());
        gl::loadMatrix(gl::MODEL_VIEW_MATRIX, m_Camera->getViewMatrix());
        gl::drawGrid(500, 500);
        
        m_scene.render(m_Camera);
        
        if(m_mesh)
        {
            gl::loadMatrix(gl::MODEL_VIEW_MATRIX, m_Camera->getViewMatrix() * m_mesh->getTransform());
            gl::drawAxes(m_mesh);
            gl::drawBoundingBox(m_mesh);
            if(m_drawNormals->val()) gl::drawNormals(m_mesh);
            
//            gl::drawPoints(m_mesh->getGeometry()->getInterleavedBuffer(),
//                           m_mesh->getGeometry()->getVertices().size(),
//                           m_pointMaterial,
//                           m_mesh->getGeometry()->getNumComponents() * sizeof(GLfloat),
//                           5 * sizeof(GLfloat));
            
//            vector<vec3> points;
//            vector<mat4>::iterator it = m_mesh->getGeometry()->getBoneMatrices().begin();
//            for (; it != m_mesh->getGeometry()->getBoneMatrices().end(); ++it)
//            {
//                points.push_back((*it)[0].xyz());
//            }
//            
//            gl::drawPoints(points);
        }
    }
    
    void mousePress(const MouseEvent &e)
    {
        m_clickPos = vec2(e.getX(), e.getY());
        m_lastTransform = mat4(m_rotation->val());
        m_lastDistance = m_distance->val();
    }
    
    void mouseDrag(const MouseEvent &e)
    {
        vec2 mouseDiff = vec2(e.getX(), e.getY()) - m_clickPos;
        if(e.isLeft() && e.isAltDown())
        {
            mat4 mouseRotate = glm::rotate(m_lastTransform, mouseDiff.x, vec3(m_lastTransform[1]) );
            mouseRotate = glm::rotate(mouseRotate, mouseDiff.y, vec3(m_lastTransform[0]) );
            *m_rotation = mat3(mouseRotate);
        }
        else if(e.isRight())
        {
            *m_distance = m_lastDistance + 0.3f * mouseDiff.y;
        }
    }
    
    void keyPress(const KeyEvent &e)
    {
        switch (e.getChar())
        {
        case KeyEvent::KEY_s:
            Serializer::saveComponentState(shared_from_this(), "config.json", PropertyIO_GL());
            break;
            
        case KeyEvent::KEY_r:
            try
            {
                Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
            }catch(FileNotFoundException &e)
            {
                printf("%s\n", e.what());
            }
            break;
                
        default:
            break;
        }
    }
    
    void resize(int w, int h)
    {
        m_Camera->setAspectRatio(getAspectRatio());
    }
    
    // Property observer callback
    void updateProperty(const Property::Ptr &theProperty)
    {
        // one of our porperties was changed
        if(theProperty == m_wireFrame)
        {
            if(m_mesh) m_mesh->getMaterial()->setWireframe(m_wireFrame->val());
        }
        else if(theProperty == m_lightDir)
        {
            if(m_mesh) m_mesh->getMaterial()->uniform("u_lightDir", m_lightDir->val());
        }
        
        else if(theProperty == m_color)
        {
            if(m_mesh)
            {
                m_mesh->getMaterial()->setDiffuse(m_color->val());
                m_mesh->getMaterial()->setBlending(m_color->val().a < 1.0f);
            }
            m_material->setDiffuse(m_color->val());
            m_pointMaterial->setDiffuse(m_color->val());
            
//            m_pointMaterial->setBlending();
//            m_pointMaterial->setDepthWrite(false);
        }
        else if(theProperty == m_textureMix)
        {
            if(m_mesh) m_mesh->getMaterial()->uniform("u_textureMix", m_textureMix->val());
        }
        else if(theProperty == m_distance ||
                theProperty == m_rotation)
        {
            m_Camera->setPosition( m_rotation->val() * glm::vec3(0, 0, m_distance->val()) );
            m_Camera->setLookAt(glm::vec3(0, 100, 0));
        }
        else if(theProperty == m_modelPath)
        {
            try
            {
                m_modelPath->val();
                gl::Mesh::Ptr m = loadModel(m_modelPath->val());
                
                m_scene.removeObject(m_mesh);
                m_mesh = m;
                
//                m_mesh->getMaterial()->addTexture(gl::TextureIO::loadTexture("stone.png"));
//                
//                //m_mesh->getMaterial()->addTexture(m_noiseTexture);
//                m_mesh->getMaterial()->addTexture(gl::TextureIO::loadTexture("asteroid_normal.png"));
//                m_mesh->getMaterial()->setBlending();
                
                m_scene.addObject(m);
                
            } catch (Exception &e)
            {
                cout<<"WARNING: "<< e.what() << endl;
                
                m_modelPath->removeObserver(shared_from_this());
                m_modelPath->val("- not found -");
                m_modelPath->addObserver(shared_from_this());
            }
        }
    }
    
    void tearDown()
    {
        printf("ciao simple geometry\n");
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new SimpleGeometryApp);
    
    return theApp->run();
}
