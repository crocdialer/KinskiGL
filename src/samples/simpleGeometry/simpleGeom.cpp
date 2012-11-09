#include "kinskiGL/App.h"

#include "kinskiGL/Scene.h"
#include "kinskiGL/Mesh.h"
#include "kinskiGL/TextureIO.h"

#include "kinskiCV/CVThread.h"

#include "kinskiGL/SerializerGL.h"

#include <assimp/assimp.hpp>
#include "assimp/aiScene.h"


using namespace std;
using namespace kinski;
using namespace glm;

class SimpleGeometryApp : public App
{
private:
    
    gl::Material::Ptr m_material, m_pointMaterial;
    gl::Geometry::Ptr m_geometry;
    gl::Geometry::Ptr m_straightPlane;
    
    gl::Mesh::Ptr m_mesh;
    gl::PerspectiveCamera::Ptr m_Camera;
    gl::Scene m_scene;
    
    RangedProperty<float>::Ptr m_distance;
    RangedProperty<float>::Ptr m_textureMix;
    Property_<string>::Ptr m_texturePath;
    Property_<bool>::Ptr m_wireFrame;
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
        
        return theMaterial;
    }

public:
    
    void setup()
    {
        glClearColor(0, 0, 0, 1);
        
        /*********** init our application properties ******************/
        
        m_distance = RangedProperty<float>::create("view distance", 25, -500, 500);
        registerProperty(m_distance);
        
        m_textureMix = RangedProperty<float>::create("texture mix ratio", 0.2, 0, 1);
        registerProperty(m_textureMix);
        
        m_texturePath = Property_<string>::create("Texture path", "smoketex.png");
        registerProperty(m_texturePath);
        
        m_wireFrame = Property_<bool>::create("Wireframe", false);
        registerProperty(m_wireFrame);
        
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
            m_material->getShader().loadFromFile("shader_vert.glsl", "shader_frag.glsl");
        }catch (std::exception &e)
        {
            fprintf(stderr, "%s\n",e.what());
        }
        
        m_straightPlane = gl::Geometry::Ptr( new gl::Plane(20, 13, 50, 50) );
        
        m_geometry =  gl::Geometry::Ptr( new gl::Geometry(*m_straightPlane) );
        m_geometry->createGLBuffers();
        
//        m_mesh = gl::Mesh::Ptr(new gl::Mesh(m_geometry, m_material));
//        m_scene.addObject(m_mesh);
        
        m_Camera = gl::PerspectiveCamera::Ptr(new gl::PerspectiveCamera);
        
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
//            m_material->getTextures()[0].update(data, GL_RED, w, h, true);
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
        
        Assimp::Importer importer;
        
        const aiScene *theScene = importer.ReadFile("duck.dae", 0);
        
        if (theScene)
        {
            // aiNode *root = theScene->mRootNode;
            aiMesh *aMesh = theScene->mMeshes[0];
            
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
            }
            
            for(int i = 0; i < aMesh->mNumFaces; i++)
            {
                const aiFace &f = aMesh->mFaces[i];
                geom->appendFace(f.mIndices[0], f.mIndices[1], f.mIndices[2]);
            }
            
            if(aMesh->mNormals)
            {
                geom->getNormals().reserve(aMesh->mNumVertices);
                geom->getNormals().insert(geom->getNormals().end(), (glm::vec3*)aMesh->mNormals,
                                          (glm::vec3*) aMesh->mNormals + aMesh->mNumVertices);
            }
            else
            {
                geom->computeVertexNormals();
            }

            m_geometry = geom;
            m_geometry->createGLBuffers();
            m_geometry->computeBoundingBox();
            m_mesh = gl::Mesh::Ptr(new gl::Mesh(m_geometry, m_material ));
            //m_mesh->setRotation( mat3(glm::rotate(mat4(), -90.f, vec3(1, 0, 0))) );
            
            m_scene.addObject(m_mesh);
        }
    
        
    }
    
    void update(const float timeDelta)
    {

//        if(m_cvThread->hasImage())
//        {
//            vector<cv::Mat> images = m_cvThread->getImages();
//            TextureIO::updateTexture(m_material->getTextures()[0], images[0]);
//        }
        
        *m_rotation = mat3( glm::rotate(mat4(m_rotation->val()),
                                        m_rotationSpeed->val() * timeDelta,
                                        vec3(0, 1, .5)));
        
        // geometry update
//        m_geometry->getVertices() = m_straightPlane->getVertices();
//        vector<vec3>::iterator vertexIt = m_geometry->getVertices().begin();
//        for (; vertexIt != m_geometry->getVertices().end(); vertexIt++)
//        {
//            vec3 &theVert = *vertexIt;
//            theVert.z = 3 * glm::simplex( vec3( m_simplexDim->val() * vec2(theVert.xy()),
//                                                m_simplexSpeed->val() * getApplicationTime()));
//        }
//        m_geometry->createGLBuffers();
        
        // generate normals lineArray
        
    }
    
    void draw()
    {
        gl::Material cloneMat1 = *m_material;
        cloneMat1.setDepthWrite(false);
        cloneMat1.setBlending(false);
        cloneMat1.setWireframe(false);
        
        gl::drawQuad(cloneMat1, getWindowSize() / 1.2f);

        gl::loadMatrix(gl::PROJECTION_MATRIX, m_Camera->getProjectionMatrix());
        gl::loadMatrix(gl::MODEL_VIEW_MATRIX, m_Camera->getViewMatrix());
        gl::drawGrid(70, 70);
        
        m_scene.render(m_Camera);
        
        gl::loadMatrix(gl::MODEL_VIEW_MATRIX, m_Camera->getViewMatrix() * m_mesh->getTransform());
        gl::drawAxes(m_mesh);
        gl::drawBoundingBox(m_mesh);
        gl::drawNormals(m_mesh);
        
        //gl::drawPoints(m_mesh->getGeometry()->getVertices(), m_pointMaterial);
        
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
            mat4 mouseRotate = glm::rotate(m_lastTransform, mouseDiff.x, vec3(0, 1, 0));
            mouseRotate = glm::rotate(mouseRotate, mouseDiff.y, vec3(1, 0, 0));
            *m_rotation = mat3(mouseRotate);
        }
        else if(e.isRight())
        {
            *m_distance = m_lastDistance + 0.3f * mouseDiff.y;
        }
    }
    
    void keyPress(const KeyEvent &e)
    {
        
        //if(e.isControlDown())
        {
            switch (e.getChar())
            {
            case KeyEvent::KEY_s:
                Serializer::saveComponentState(shared_from_this(), "config.json", PropertyIO_GL());
                break;
                
            case KeyEvent::KEY_r:
                Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
                break;
                    
            default:
                break;
            }
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
            m_material->setWireframe(m_wireFrame->val());
        
        else if(theProperty == m_color)
        {
            m_material->setDiffuse(m_color->val());
            m_pointMaterial->setDiffuse(m_color->val());
        }
        else if(theProperty == m_textureMix)
            m_material->uniform("u_textureMix", m_textureMix->val());
        
        else if(theProperty == m_distance ||
                theProperty == m_rotation)
        {
            m_Camera->setPosition( m_rotation->val() * glm::vec3(0, 0, m_distance->val()) );
            m_Camera->setLookAt(glm::vec3(0));
        }
        else if(theProperty == m_texturePath)
        {
            try
            {
                m_pointMaterial->getTextures().clear();
                m_pointMaterial->addTexture( gl::TextureIO::loadTexture(m_texturePath->val()) );
            } catch (gl::TextureIO::TextureNotFoundException &e)
            {
                cout<<"WARNING: "<< e.what() << endl;
                
                m_texturePath->removeObserver(shared_from_this());
                m_texturePath->val("- not found -");
                m_texturePath->addObserver(shared_from_this());
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
