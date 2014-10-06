#include "app/Raspi_App.h"
#include "core/networking.h"

#include "gl/SerializerGL.h"
#include "gl/Scene.h"
#include "gl/Mesh.h"
#include "gl/Fbo.h"

#include "AssimpConnector.h"

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
    
    RangedProperty<float>::Ptr m_distance;
    
    Property_<bool>::Ptr m_wireFrame;
    Property_<bool>::Ptr m_drawNormals;
    Property_<glm::vec3>::Ptr m_lightDir;
    
    Property_<glm::vec4>::Ptr m_color;
    Property_<glm::mat3>::Ptr m_rotation;
    RangedProperty<float>::Ptr m_rotationSpeed;

    Property_<glm::vec3>::Ptr m_camPosition;
    
    Property_<std::string>::Ptr m_imagePath;

    net::tcp_server m_tcp_server;
    std::vector<net::tcp_connection_ptr> m_tcp_connections;
    
public:
    
    //SimpleRaspiApp(int width, int height):Raspi_App(width, height){};

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

        m_imagePath = Property_<std::string>::create("Image path", "test.png");
        registerProperty(m_imagePath);

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
        
        // setup a tcp server for kinski_remote
        m_tcp_server = net::tcp_server(io_service(), 33333,
                                       [this](net::tcp_connection_ptr con)
        {
            LOG_DEBUG << "port: "<< con->port()<<" -- new connection with: " << con->remote_ip()
              << " : " << con->remote_port();
        
            con->send(Serializer::serializeComponents({shared_from_this()}, PropertyIO_GL()));
            m_tcp_connections.push_back(con);
        
            con->set_receive_function([&](net::tcp_connection_ptr con, const std::vector<uint8_t>& response)
            {
                try
                {
                    Serializer::applyStateToComponents({shared_from_this()},
                                                       string(response.begin(),
                                                              response.end()), 
                                                       PropertyIO_GL());
                } catch (std::exception &e)
                {
                  LOG_ERROR << e.what();
                }
            });
        });
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

