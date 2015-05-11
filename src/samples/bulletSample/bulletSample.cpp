#ifdef KINSKI_RASPI
#include "app/Raspi_App.h"
typedef kinski::Raspi_App BaseAppType;
#else
#include "app/ViewerApp.h"
typedef kinski::ViewerApp BaseAppType;
#endif

#include "physics/physics_context.h"

#include "cv/CVThread.h"
#include "cv/TextureIO.h"
#include "ThreshNode.h"
#include "DopeRecorder.h"
#include "FaceFilter.h"

#include "gl/Renderer.h"

using namespace std;
using namespace kinski;
using namespace glm;

class BulletSample : public BaseAppType
{
private:
    
    vector<gl::Texture> m_textures;
    gl::MaterialPtr m_box_material;
    
    Property_<string>::Ptr m_font_name;
    Property_<int>::Ptr m_font_size;
    Property_<bool>::Ptr m_stepPhysics;
    Property_<glm::vec4>::Ptr m_color;
    
    kinski::physics::physics_context m_physics_context;
    std::shared_ptr<kinski::physics::BulletDebugDrawer> m_debugDrawer;
    
    gl::Renderer m_renderer;
    gl::Font m_font;
    std::list<std::string> m_font_paths;
    gl::MeshPtr m_label;
    
    // opencv interface
    CVThread::Ptr m_cvThread;

public:
    
    void draw_object_info2D(const gl::MeshPtr &mesh, const glm::vec2 &pos, const glm::vec4 &color)
    {
        glm::vec2 offset = pos;
        glm::vec2 step = vec2(0, m_font.getLineHeight() + 4);
        gl::drawText2D("id: " + kinski::as_string(mesh->getID()), m_font,
                       color, offset);
        gl::drawText2D("faces: " + kinski::as_string(mesh->geometry()->faces().size()), m_font,
                       color, offset + step);
    }
    
    void create_cube_stack(int size_x, int size_y, int size_z, const gl::MaterialPtr &theMat)
    {
        m_physics_context.teardown();
        scene().clear();
        
        float scaling = 8.0f;
        float start_pox_x = -5;
        float start_pox_y = -5;
        float start_pox_z = -3;
        
        auto big_box_shape = std::make_shared<btBoxShape>(btVector3(50., 50., 50.));
        
        gl::MeshPtr ground_mesh = gl::Mesh::create(gl::Geometry::createBox(glm::vec3(50.0f)), theMat);
        scene().addObject(ground_mesh);
        ground_mesh->position() = glm::vec3(0, -50, 0);
        
        m_physics_context.add_mesh_to_simulation(ground_mesh, 0.f, big_box_shape);
        
        {
            // geometry for our cubes
            gl::Geometry::Ptr geom = gl::Geometry::createBox(glm::vec3(scaling * 1));
            
            // collision shape for our cubes
            auto box_shape = std::make_shared<btBoxShape>(btVector3(scaling, scaling, scaling));
            
            float start_x = start_pox_x - size_x/2;
            float start_y = start_pox_y;
            float start_z = start_pox_z - size_z/2;
            
            for (int k=0;k<size_y;k++)
            {
                for (int i=0;i<size_x;i++)
                {
                    for(int j = 0;j<size_z;j++)
                    {
                        gl::MeshPtr mesh = gl::Mesh::create(geom, theMat);
                        
                        mesh->setPosition(scaling * vec3(2.0 * i + start_x,
                                                         20 + 2.0 * k + start_y,
                                                         2.0 * j + start_z));
                        m_physics_context.add_mesh_to_simulation(mesh, 1.f, box_shape);
                        scene().addObject(mesh);
                    }
                }
            }
        }
        LOG_INFO<<"created dynamicsworld with "<<
            m_physics_context.dynamicsWorld()->getNumCollisionObjects()<<" rigidbodies";
    }

    void setup()
    {
        BaseAppType::setup();
        
        kinski::add_search_path("~/Desktop");
        kinski::add_search_path("/Library/Fonts");
        
        m_font.load("Arial.ttf", 24);
        outstream_gl().set_font(m_font);
        
        for (int i = 0; i < 4; ++i){ m_textures.push_back(gl::Texture()); }
        
        /*********** init our application properties ******************/
        
        m_font_name = Property_<string>::create("Font Name", "Arial.ttf");
        registerProperty(m_font_name);
        
        m_font_size = Property_<int>::create("Font Size", 24);
        registerProperty(m_font_size);
        
        m_stepPhysics = Property_<bool>::create("Step Physics", true);
        registerProperty(m_stepPhysics);
        
        m_color = Property_<glm::vec4>::create("Material color", glm::vec4(1 ,1 ,0, 0.6));
        registerProperty(m_color);
            
        // enable observer mechanism
        observeProperties();
        
        /********************** construct a simple scene ***********************/
        
        // test box shape
        gl::Geometry::Ptr myBox(gl::Geometry::createBox(vec3(50, 100, 50)));
        
        m_box_material = gl::Material::create();
//        m_box_material->setShader(gl::createShader(gl::SHADER_PHONG_NORMALMAP));
        //m_box_material->setShader(gl::createShaderFromFile("shader_normalMap.vert", "shader_normalMap.frag"));
        
        m_box_material->addTexture(m_textures[3]);
        //m_textures[3].set_anisotropic_filter(8);
        materials().push_back(m_box_material);
        m_box_material->setBlending();
        
        // camera input
        m_cvThread = CVThread::create();
        CVProcessNode::Ptr thresh_node(new ThreshNode(-1)), record_node(new DopeRecorder(5000)),
            face_node(new FaceFilter());
        CVCombinedProcessNode::Ptr combi_node = face_node >> thresh_node >> record_node;
        combi_node->observeProperties();
        m_cvThread->setProcessingNode(combi_node);
        m_cvThread->streamUSBCamera();
        
        // init physics pipeline
        m_physics_context.init();
        m_debugDrawer = shared_ptr<physics::BulletDebugDrawer>(new physics::BulletDebugDrawer);
        m_physics_context.dynamicsWorld()->setDebugDrawer(m_debugDrawer.get());
        
        // create a physics scene
        create_cube_stack(4, 32, 4, m_box_material);
        
        // create a simplex noise texture
        if(true)
        {
            int w = 512, h = 512;
            float data[w * h];
            
            for (int i = 0; i < h; i++)
                for (int j = 0; j < w; j++)
                {
                    data[i * h + j] = (glm::simplex( vec3(0.0125f * vec2(i, j), 0.025)) + 1) / 2.f;
                }
            
            gl::Texture::Format fmt;
            fmt.setInternalFormat(GL_COMPRESSED_RED_RGTC1);
            fmt.set_mipmapping(true);
            gl::Texture noise_tex = gl::Texture (w, h, fmt);
            noise_tex.update(data, GL_RED, w, h, true);
            noise_tex.set_anisotropic_filter(8);
            m_box_material->addTexture(noise_tex);
        }
        
        // load state from config file
        try
        {
            Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
            Serializer::loadComponentState(m_cvThread->getProcessingNode(), "config_cv.json", PropertyIO_GL());
        }catch(Exception &e)
        {
            LOG_WARNING << e.what();
        }
#ifndef KINSKI_RASPI
        create_tweakbar_from_component(shared_from_this());
        create_tweakbar_from_component(m_cvThread);
        addPropertyListToTweakBar(m_cvThread->getProcessingNode()->getPropertyList(),
                                  "", tweakBars().back());
#endif
    }
    
    void update(float timeDelta)
    {
        BaseAppType::update(timeDelta);
        
        if (m_physics_context.dynamicsWorld() && *m_stepPhysics)
        {
            m_physics_context.dynamicsWorld()->stepSimulation(timeDelta);
        }
        
        if(m_cvThread->hasImage())
        {
            vector<cv::Mat> images = m_cvThread->getImages();
            
            for(int i = 0;i < images.size();i++)
            {
                if(i < 4)
                {
                    gl::TextureIO::updateTexture(m_textures[i], images[i]);
                }
            }
        }
        for (int i = 0; i < materials().size(); i++)
        {
            materials()[i]->uniform("u_time",getApplicationTime());
            materials()[i]->setAmbient(0.2 * clear_color());
        }
    }
    
    void draw()
    {
        gl::setMatrices(camera());
        
        if(draw_grid())
        {
            gl::drawGrid(500, 500);
        }
        
        if(wireframe())
        {
            m_physics_context.dynamicsWorld()->debugDrawWorld();
            m_debugDrawer->flush();
        }else
        {
            //scene().render(camera());
            gl::RenderBinPtr bin = scene().cull(camera());
            m_renderer.render(bin);
        }
        
        if(selected_mesh())
        {
            gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix() * selected_mesh()->transform());
            gl::drawAxes(selected_mesh());
            gl::drawBoundingBox(selected_mesh());
            if(normals()) gl::drawNormals(selected_mesh());
            
            gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix() * m_label->transform());
            m_label->setRotation(glm::mat3(camera()->transform()));
            gl::drawMesh(m_label);
        }
        // draw texture map(s)
        if(displayTweakBar())
        {
            // draw opencv maps
            float w = (windowSize()/8.f).x;
            glm::vec2 offset(getWidth() - w - 10, 10);
            for(auto &t : m_textures)
            {
                if(!t) continue;
                
                float h = t.getHeight() * w / t.getWidth();
                glm::vec2 step(0, h + 10);
                drawTexture(t, vec2(w, h), offset);
                gl::drawText2D(as_string(t.getWidth()) + std::string(" x ") +
                               as_string(t.getHeight()), m_font, glm::vec4(1),
                               offset);
                offset += step;
            }
            
            if(selected_mesh())
            {
                draw_object_info2D(selected_mesh(), glm::vec2(10, windowSize().y - 120),
                                   vec4(vec3(1) - clear_color().xyz(), 1.f));
            }
            
            gl::drawText2D(kinski::as_string(scene().num_visible_objects()), m_font,
                           vec4(vec3(1) - clear_color().xyz(), 1.f),
                           glm::vec2(windowSize().x - 90, windowSize().y - 70));
            
            // draw fps string
            gl::drawText2D(kinski::as_string(framesPerSec()), m_font,
                           vec4(vec3(1) - clear_color().xyz(), 1.f),
                           glm::vec2(windowSize().x - 110, windowSize().y - 40));
        }
    }
    
    void keyPress(const KeyEvent &e)
    {
        BaseAppType::keyPress(e);
        
        switch (e.getCode())
        {
        case GLFW_KEY_W:
            set_wireframe(!wireframe());
            break;
                
        case GLFW_KEY_P:
            *m_stepPhysics = !*m_stepPhysics;
            break;
                
        case GLFW_KEY_R:
            Serializer::loadComponentState(m_cvThread->getProcessingNode(), "config_cv.json", PropertyIO_GL());
            m_physics_context.teardown();
            create_cube_stack(4, 32, 4, m_box_material);
            break;
                
        case GLFW_KEY_S:
            try
            {
                Serializer::saveComponentState(m_cvThread->getProcessingNode(), "config_cv.json", PropertyIO_GL());
            }catch(const Exception &e){ LOG_ERROR<<e.what(); }
            break;
                
        default:
            break;
        }
    }
    
    void mousePress(const MouseEvent &e)
    {
        BaseAppType::mousePress(e);
        
        if(selected_mesh())
        {
            m_label = m_font.create_mesh("My Id is " + kinski::as_string(selected_mesh()->getID()));
            m_label->setPosition(selected_mesh()->position()
                                 + camera()->up() * (selected_mesh()->boundingBox().height() / 2.f
                                                     + m_label->boundingBox().height())
                                 - m_label->boundingBox().center());
            m_label->setRotation(glm::mat3(camera()->transform()));
        }
        
    }
    
    // Property observer callback
    void updateProperty(const Property::ConstPtr &theProperty)
    {
        BaseAppType::updateProperty(theProperty);
        
        if(theProperty == m_color)
        {
            m_box_material->setDiffuse(*m_color);
            if(m_color->value().a == 1.f) m_box_material->setBlending();
        }
        else if(theProperty == m_font_name || theProperty == m_font_size)
        {
            m_font.load(*m_font_name, *m_font_size);
        }
    }
    
    void tearDown()
    {
        LOG_PRINT<<"ciao bullet sample";
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new BulletSample);
    return theApp->run();
}
