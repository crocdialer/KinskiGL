#include "app/Raspi_App.h"
#include "core/networking.h"

#include "gl/SerializerGL.h"
#include "gl/Scene.h"
#include "gl/Mesh.h"
#include "gl/Fbo.h"

// remote control
#include "app/RemoteControl.h"

// module headers
#include "AssimpConnector.h"
#include "DMXController.h"

using namespace std;
using namespace kinski;
using namespace glm;

class SimpleRaspiApp : public Raspi_App
{
private:
   
    gl::Fbo m_frameBuffer;
    gl::Texture m_textures[4];
    
    gl::MaterialPtr m_material, m_pointMaterial;
    gl::GeometryPtr m_geometry;
    gl::GeometryPtr m_straightPlane;
    
    gl::MeshPtr m_mesh;
    gl::PerspectiveCamera::Ptr m_Camera;
    gl::Scene m_scene;
    
    RangedProperty<float>::Ptr m_distance = = RangedProperty<float>::create("view distance", 25, 0, 5000);;
    
    Property_<bool>::Ptr m_wireFrame = Property_<bool>::create("Wireframe", false);
    Property_<bool>::Ptr m_drawNormals = Property_<bool>::create("Normals", false);
    Property_<glm::vec3>::Ptr m_lightDir = Property_<vec3>::create("Light dir", vec3(1));
    
    Property_<glm::vec4>::Ptr m_color = Property_<glm::vec4>::create("Material color", glm::vec4(1 ,1 ,0, 0.6));
    Property_<glm::mat3>::Ptr m_rotation = Property_<glm::mat3>::create("Geometry Rotation", glm::mat3());;
    RangedProperty<float>::Ptr m_rotationSpeed = RangedProperty<float>::create("Rotation Speed", 15, -100, 100);

    Property_<glm::vec3>::Ptr m_camPosition = Property_<glm::vec3>::create("Camera Position", vec3(0, 30, -120));
    
    Property_<std::string>::Ptr m_imagePath = Property_<std::string>::create("Image path", "test.png");

    // remote control
    RemoteControl m_remote_control;
    
    // dmx vals
    DMXController m_dmx_control;
    RangedProperty<int>::Ptr
    m_dmx_start_index = RangedProperty<int>::create("DMX start index", 1, 0, 255);
    Property_<gl::Color>::Ptr m_dmx_color = Property_<gl::Color>::create("DMX color", gl::COLOR_OLIVE);
    
public:
    
    //SimpleRaspiApp(int width, int height):Raspi_App(width, height){};

    void setup()
    {
        /*********** init our application properties ******************/
        
        registerProperty(m_distance);
        registerProperty(m_wireFrame);
        registerProperty(m_drawNormals);
        registerProperty(m_lightDir);
        registerProperty(m_color);
        registerProperty(m_rotation);
        registerProperty(m_rotationSpeed);
        registerProperty(m_camPosition);
        registerProperty(m_imagePath);
        registerProperty(m_dmx_start_index);
        registerProperty(m_dmx_color);

        // enable observer mechanism
        observeProperties();
        
        /********************** construct a simple scene ***********************/
        
        gl::clearColor(glm::vec4(0));
        // init FBO
        m_frameBuffer = gl::Fbo(800, 600);

        m_textures[0] = gl::createTextureFromFile(*m_imagePath);
        m_Camera = gl::PerspectiveCamera::Ptr(new gl::PerspectiveCamera);
        m_Camera->setClippingPlanes(1, 5000);
        m_Camera->setAspectRatio(getAspectRatio());
        m_Camera->setPosition( m_camPosition->value() );
        m_Camera->setLookAt(glm::vec3(0, 0, 0)); 

        // test box shape
        gl::GeometryPtr myBox = gl::Geometry::createBox(glm::vec3(40, 40, 40));
        
        gl::MaterialPtr myMaterial = gl::Material::create();
        //myMaterial->setDepthTest(false);

        //myMaterial->setDiffuse(vec4(1.0f, 1.0f, 1.0f, .75f) );
        //myMaterial->setBlending(true);
        //myMaterial->shader() = gl::createShaderFromFile("Shader.vert", "Shader.frag");
        //myMaterial->addTexture(m_textures[1]);

        gl::MeshPtr myBoxMesh = gl::Mesh::create(myBox, myMaterial);
        myBoxMesh->setPosition(vec3(0, 0, 0));
        m_scene.addObject(myBoxMesh);
       
        m_mesh = myBoxMesh;
        m_material = myMaterial;
        
        GLubyte pixels[] =
        {  
            255,   0,   0, 255, // Red
            0, 255,   0,  255,  // Green
            0,   0, 255, 255,   // Blue
            255, 255,   0, 255  // Yellow
        };

        m_textures[1].update(pixels, GL_UNSIGNED_BYTE, GL_RGBA, 2, 2, false);

        // load state from config file
        try
        {
            Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
        }catch(Exception &e)
        {
            LOG_WARNING << e.what();
            Serializer::saveComponentState(shared_from_this(), "config.json", PropertyIO_GL());
        }
        
        // add tcp remote control
        m_remote_control = RemoteControl(io_service(), {shared_from_this(), m_light_component});
        m_remote_control.start_listen(); 
    }
    
    void update(const float timeDelta)
    {
        glm::mat4 newTrans = glm::rotate(m_mesh->transform(),
                                         m_rotationSpeed->value() * timeDelta,
                                         vec3(0, 1, .5));
        m_mesh->setTransform(newTrans);
        m_mesh->material()->uniform("u_time", getApplicationTime()); 
    }
    
    void draw()
    {
        gl::drawTexture(m_textures[0], windowSize());
        
        gl::loadMatrix(gl::PROJECTION_MATRIX, m_Camera->getProjectionMatrix());
        gl::loadMatrix(gl::MODEL_VIEW_MATRIX, m_Camera->getViewMatrix());
        gl::drawGrid(500, 500);
        
        //gl::render_to_texture(m_scene, m_frameBuffer, m_Camera);
        m_scene.render(m_Camera);

        //m_frameBuffer.unbindFramebuffer();
        //glViewport(0, 0, getWidth(), getHeight());
       
        //// draw fbo content
        //gl::drawTexture(m_frameBuffer.getTexture(), windowSize());
    }
    
    
    void keyPress(const KeyEvent &e)
    {
        switch (e.getCode())
        {
                
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
            //m_material->uniform("u_lightDir", m_lightDir->value());
        }
        else if(theProperty == m_camPosition)
        {
            m_Camera->setPosition( m_camPosition->value() );
            m_Camera->setLookAt(glm::vec3(0, 0, 0));
        }
        else if(theProperty == m_imagePath)
        {
            m_textures[0] = gl::createTextureFromFile(m_imagePath->value());
        }
        else if(theProperty == m_dmx_color)
        {
            const gl::Color &c = *m_dmx_color;
        
            // set manual control
            m_dmx_control[*m_dmx_start_index] = 0;
          
            // R
            m_dmx_control[*m_dmx_start_index + 1] = (uint8_t)(c.r * 255);
            
            // G
            m_dmx_control[*m_dmx_start_index + 2] = (uint8_t)(c.g * 255);
            
            // B
            m_dmx_control[*m_dmx_start_index + 3] = (uint8_t)(c.b * 255);
            
            m_dmx_control.update();
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
    LOG_INFO<<"Running on IP: " << net::local_ip();
    return theApp->run();
}

