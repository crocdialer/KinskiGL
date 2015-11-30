#include "app/ViewerApp.h"
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

class SimpleRaspiApp : public ViewerApp
{
private:
   
    gl::Fbo m_frameBuffer;
    
    gl::MaterialPtr m_material, m_pointMaterial;
    gl::GeometryPtr m_geometry;
    gl::GeometryPtr m_straightPlane;
    
    gl::MeshPtr m_mesh;
    
    //RangedProperty<float>::Ptr m_distance = RangedProperty<float>::create("view distance", 25, 0, 5000);;
    //
    //Property_<bool>::Ptr m_wireFrame = Property_<bool>::create("Wireframe", false);
    //Property_<bool>::Ptr m_drawNormals = Property_<bool>::create("Normals", false);
    //Property_<glm::vec3>::Ptr m_lightDir = Property_<vec3>::create("Light dir", vec3(1));
    //
    //Property_<glm::vec4>::Ptr m_color = Property_<glm::vec4>::create("Material color", glm::vec4(1 ,1 ,0, 0.6));
    //Property_<glm::mat3>::Ptr m_rotation = Property_<glm::mat3>::create("Geometry Rotation", glm::mat3());;
    //RangedProperty<float>::Ptr m_rotationSpeed = RangedProperty<float>::create("Rotation Speed", 15, -100, 100);

    //Property_<glm::vec3>::Ptr m_camPosition = Property_<glm::vec3>::create("Camera Position", vec3(0, 30, -120));
    
    Property_<std::string>::Ptr m_imagePath = Property_<std::string>::create("Image path", "test.png");

    // remote control
    RemoteControl m_remote_control;
    
    // dmx vals
    DMXController m_dmx_control;
    RangedProperty<int>::Ptr
    m_dmx_start_index = RangedProperty<int>::create("DMX start index", 1, 0, 255);
    Property_<gl::Color>::Ptr m_dmx_color = Property_<gl::Color>::create("DMX color", gl::COLOR_OLIVE);
    
public:
    
    void setup()
    {
        ViewerApp::setup();

        /*********** init our application properties ******************/
        
        //register_property(m_distance);
        //register_property(m_wireFrame);
        //register_property(m_drawNormals);
        //register_property(m_lightDir);
        //register_property(m_color);
        //register_property(m_rotation);
        //register_property(m_rotationSpeed);
        //register_property(m_camPosition);
        register_property(m_imagePath);
        register_property(m_dmx_start_index);
        register_property(m_dmx_color);

        // enable observer mechanism
        observe_properties();
        
        /********************** construct a simple scene ***********************/
        
        gl::clearColor(glm::vec4(0));
        // init FBO
        m_frameBuffer = gl::Fbo(864, 486);

        textures()[0] = gl::createTextureFromFile(*m_imagePath);
        //camera() = gl::PerspectiveCamera::Ptr(new gl::PerspectiveCamera);
        //camera()->setClippingPlanes(1, 5000);
        //camera()->setAspectRatio(getAspectRatio());
        //camera()->setPosition( m_camPosition->value() );
        //camera()->setLookAt(glm::vec3(0, 0, 0)); 

        // test box shape
        gl::GeometryPtr myBox = gl::Geometry::createBox(glm::vec3(1, 1, 1));
        
        gl::MaterialPtr myMaterial = gl::Material::create();
        //myMaterial->setDepthTest(false);

        //myMaterial->setDiffuse(vec4(1.0f, 1.0f, 1.0f, .75f) );
        //myMaterial->setBlending(true);
        //myMaterial->shader() = gl::createShaderFromFile("Shader.vert", "Shader.frag");
        //myMaterial->addTexture(m_textures[1]);

        gl::MeshPtr myBoxMesh = gl::Mesh::create(myBox, myMaterial);
        myBoxMesh->setPosition(vec3(0, 0, 0));
        scene().addObject(myBoxMesh);
       
        m_mesh = myBoxMesh;
        m_material = myMaterial;
        
        GLubyte pixels[] =
        {  
            255,   0,   0, 255, // Red
            0, 255,   0,  255,  // Green
            0,   0, 255, 255,   // Blue
            255, 255,   0, 255  // Yellow
        };

        textures()[1].update(pixels, GL_UNSIGNED_BYTE, GL_RGBA, 2, 2, false);
        
        load_settings();

        // add tcp remote control
        m_remote_control = RemoteControl(io_service(), {shared_from_this()});
        m_remote_control.start_listen(); 
    }
    
    void update(const float timeDelta)
    {
        ViewerApp::update(timeDelta);
        glm::mat4 newTrans = glm::rotate(m_mesh->transform(),
                                         -m_rotation_speed->value() * timeDelta,
                                         vec3(0, 1, .5));
        m_mesh->setTransform(newTrans);
        m_mesh->material()->uniform("u_time", getApplicationTime()); 
    }
    
    void draw()
    {
        gl::drawTexture(m_textures[0], windowSize());
        
        gl::setMatrices(camera());
        gl::drawGrid(500, 500);
        
        scene().render(camera());

        // draw fbo content
        //gl::render_to_texture(scene(), m_frameBuffer, camera());
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

        //camera()->setAspectRatio(getAspectRatio());
    }
    
    // Property observer callback
    void update_property(const Property::ConstPtr &theProperty)
    {
        ViewerApp::update_property(theProperty);

        if(theProperty == m_imagePath)
        {
            textures()[0] = gl::createTextureFromFile(m_imagePath->value());
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
    auto theApp = std::make_shared<SimpleRaspiApp>();
    LOG_INFO<<"Running on IP: " << net::local_ip();
    return theApp->run();
}

