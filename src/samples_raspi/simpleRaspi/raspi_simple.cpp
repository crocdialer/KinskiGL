#include "kinskiApp/Raspi_App.h"
//#include "kinskiApp/TextureIO.h"

#include "kinskiGL/SerializerGL.h"
#include "kinskiGL/Scene.h"
#include "kinskiGL/Mesh.h"

using namespace std;
using namespace kinski;
using namespace glm;

class SimpleRaspiApp : public Raspi_App
{
private:
    
    gl::Texture m_textures[4];
    
    gl::Material::Ptr m_material, m_pointMaterial;
    gl::Geometry::Ptr m_geometry;
    gl::Geometry::Ptr m_straightPlane;
    
    gl::Mesh::Ptr m_mesh;
    gl::PerspectiveCamera::Ptr m_Camera;
    gl::Scene m_scene;
    
    RangedProperty<float>::Ptr m_distance;
    
    Property_<bool>::Ptr m_wireFrame;
    Property_<bool>::Ptr m_drawNormals;
    Property_<glm::vec3>::Ptr m_lightDir;
    
    Property_<glm::vec4>::Ptr m_color;
    Property_<glm::mat3>::Ptr m_rotation;
    RangedProperty<float>::Ptr m_rotationSpeed;

    Property_<glm::vec3>::Ptr m_camPosition;
    
public:
    
    void setup()
    {
        /*********** init our application properties ******************/
        
        m_distance = RangedProperty<float>::create("view distance", 25, 0, 5000);
        registerProperty(m_distance);
        
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
        
        m_rotationSpeed = RangedProperty<float>::create("Rotation Speed", 15, -100, 100);
        registerProperty(m_rotationSpeed);
        
        m_camPosition = Property_<glm::vec3>::create("Camera Position", vec3(0, 30, -120));
        registerProperty(m_camPosition);

        // enable observer mechanism
        observeProperties();
        
        /********************** construct a simple scene ***********************/
        
        //m_textures[0] = gl::TextureIO::loadTexture("/home/pi/Desktop/kinski.jpg");
        m_Camera = gl::PerspectiveCamera::Ptr(new gl::PerspectiveCamera);
        m_Camera->setClippingPlanes(.1, 5000);
        m_Camera->setAspectRatio(getAspectRatio());
        m_Camera->setPosition( m_camPosition->val() );
        m_Camera->setLookAt(glm::vec3(0, 0, 0)); 

        // test box shape
        gl::Geometry::Ptr myBox(new gl::Box(glm::vec3(40, 40, 40)));
        //gl::Geometry::Ptr myBox(new gl::Plane(50, 50));
        
        gl::Material::Ptr myMaterial(new gl::Material);
        myMaterial->setTwoSided();
        //myMaterial->setDiffuse(vec4(glm::linearRand(vec3(0), vec3(1)), 1.0f) );
        myMaterial->shader().loadFromFile("Shader.vert", "Shader.frag");

        gl::Mesh::Ptr myBoxMesh(new gl::Mesh(myBox, myMaterial));
        myBoxMesh->setPosition(vec3(0, 0, 0));
        m_scene.addObject(myBoxMesh);
       
        m_mesh = myBoxMesh;
        m_material = myMaterial;

        // load state from config file
        try
        {
            Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
        }catch(Exception &e)
        {
            LOG_WARNING << e.what();
            Serializer::saveComponentState(shared_from_this(), "config.json", PropertyIO_GL());
        }
    }
    
    void update(const float timeDelta)
    {
        glm::mat4 newTrans = glm::rotate(m_mesh->getTransform(),
                                         m_rotationSpeed->val() * timeDelta,
                                         vec3(0, 1, .5));
        m_mesh->setTransform(newTrans);
        
    }
    
    void draw()
    {
        //gl::drawTexture(m_textures[0], windowSize());
        
        gl::Material cloneMaterial = *m_material;
        cloneMaterial.setDepthTest(false);
        cloneMaterial.setDepthWrite(false);
        gl::drawQuad(cloneMaterial, windowSize() * .5f);

        gl::loadMatrix(gl::PROJECTION_MATRIX, m_Camera->getProjectionMatrix());
        gl::loadMatrix(gl::MODEL_VIEW_MATRIX, m_Camera->getViewMatrix());
        gl::drawGrid(500, 500);
        
        //m_scene.render(m_Camera);

        gl::loadMatrix(gl::MODEL_VIEW_MATRIX, m_Camera->getViewMatrix() * m_mesh->getTransform());
        gl::drawMesh(m_mesh);
        gl::drawBoundingBox(m_mesh);
        gl::drawPoints(m_mesh->geometry()->vertexBuffer().id(), m_mesh->geometry()->vertices().size());
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
            }catch(Exception &e)
            {
                LOG_WARNING << e.what();
            }
            break;
                
        default:
            break;
        }
    }

    void resize(int w, int h)
    {
        LOG_INFO<<"resize: "<< w <<" -- "<< h;

        m_Camera->setAspectRatio(getAspectRatio());
    }
    
    // Property observer callback
    void updateProperty(const Property::ConstPtr &theProperty)
    {
        // one of our porperties was changed
        if(theProperty == m_color)
        {
        }
        else if(theProperty == m_lightDir )
        {
            //m_material->uniform("u_lightDir", m_lightDir->val());
        }
        else if(theProperty == m_camPosition)
        {
            m_Camera->setPosition( m_camPosition->val() );
            m_Camera->setLookAt(glm::vec3(0, 0, 0));
        }
    }
    
    void tearDown()
    {
        LOG_PRINT<<"ciao simple raspi";
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new SimpleRaspiApp);
    return theApp->run();
}

