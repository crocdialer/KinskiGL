#include "kinskiApp/ViewerApp.h"
#include "kinskiApp/AppServer.h"
#include "kinskiGL/Fbo.h"
#include "AssimpConnector.h"


using namespace std;
using namespace kinski;
using namespace glm;


class Animator
{
public:
    
    struct Task
    {
        
    };
    void update()
    {
        for (auto &task : m_tasks){task();}
    };
    
    template<typename F, typename V>
    void add_animation_task(F functor, const V &from, const V &to, double start_time, double duration)
    {
        
    }
    
private:
    std::list<boost::function<void()> > m_tasks;
};

class SimpleGeometryApp : public ViewerApp
{
private:
    
    gl::Fbo m_frameBuffer;
    gl::Texture m_textures[4];
    gl::MeshPtr m_mesh, m_label;
    gl::Font m_font;
    
    gl::MaterialPtr m_draw_depth_material;
    
    RangedProperty<float>::Ptr m_textureMix;
    Property_<string>::Ptr m_modelPath;
    RangedProperty<float>::Ptr m_animationTime;
    Property_<glm::vec4>::Ptr m_color;
    Property_<float>::Ptr m_shinyness;
    
    vector<vec3> m_points;
    
    LightComponent::Ptr m_light_component;
    gl::MaterialPtr m_post_process_mat;
    
public:
    
    void setup()
    {
        ViewerApp::setup();
        set_precise_selection(true);
        
        /******************** add search paths ************************/
        kinski::addSearchPath("~/Desktop/creatures", true);
        kinski::addSearchPath("~/Desktop/doom3_base", true);
        kinski::addSearchPath("/Library/Fonts");
        
        list<string> files = kinski::getDirectoryEntries("~/Desktop/sample", true, "png");
        
        m_font.load("Courier New Bold.ttf", 24);
        
        /*********** init our application properties ******************/
        
        m_textureMix = RangedProperty<float>::create("texture mix ratio", 0.2, 0, 1);
        registerProperty(m_textureMix);
        
        m_modelPath = Property_<string>::create("Model path", "duck.dae");
        registerProperty(m_modelPath);

        m_animationTime = RangedProperty<float>::create("Animation time", 0, 0, 1);
        registerProperty(m_animationTime);
        
        m_color = Property_<glm::vec4>::create("Material color", glm::vec4(1 ,1 ,0, 0.6));
        registerProperty(m_color);
        
        m_shinyness = Property_<float>::create("Shinyness", 1.0);
        registerProperty(m_shinyness);
        
        create_tweakbar_from_component(shared_from_this());

        // enable observer mechanism
        observeProperties();
        
        // light component
        m_light_component.reset(new LightComponent());
        create_tweakbar_from_component(m_light_component);
        
        /********************** construct a simple scene ***********************/
        camera()->setClippingPlanes(1.0, 5000);
        
        gl::Fbo::Format fboFormat;
        //TODO: mulitsampling fails
        //fboFormat.setSamples(4);
        m_frameBuffer = gl::Fbo(getWidth(), getHeight(), fboFormat);
        //int numsamples = m_frameBuffer.getMaxSamples();//4
        //int numattachments = m_frameBuffer.getMaxAttachments();//8
        
        // Depth of field post processing
        m_post_process_mat = gl::Material::create();
        m_post_process_mat->setShader(gl::createShaderFromFile("shader_depth.vert", "shader_dof.frag"));
        m_post_process_mat->uniform("u_z_near", camera()->near());
        m_post_process_mat->uniform("u_z_far", camera()->far());
        m_post_process_mat->uniform("u_focalLength", 25.f);
        m_post_process_mat->uniform("u_fstop", 2.f);
        m_post_process_mat->uniform("u_showFocus", false);
        
        // create a simplex noise texture
        {
            int w = 512, h = 512;
            float data[w * h];
            
            for (int i = 0; i < h; i++)
                for (int j = 0; j < w; j++)
                {
                    data[i * h + j] = (glm::simplex( vec3(0.0125f * vec2(i, j), 0.025)) + 1) / 2.f;
                }
            
            gl::Texture::Format fmt;
            fmt.setInternalFormat(GL_RED);
            fmt.set_mipmapping(true);
            m_textures[1] = gl::Texture (w, h, fmt);
            m_textures[1].update(data, GL_RED, w, h, true);
        }

        gl::Geometry::Ptr plane_geom(gl::createPlane(1000, 1000));
        gl::MaterialPtr mat = gl::Material::create(gl::createShader(gl::SHADER_PHONG));
        materials().push_back(mat);
        gl::MeshPtr ground_plane = gl::Mesh::create(plane_geom, mat);
        ground_plane->setLookAt(vec3(0, -1, 0), vec3(0, 0, 1));
        scene().addObject(ground_plane);
        
        gl::Geometry::Ptr cone_geom(gl::createCone(100, 200, 32));
        mat = gl::Material::create(gl::createShader(gl::SHADER_PHONG));
        gl::MeshPtr cone_mesh = gl::Mesh::create(cone_geom, mat);
        //cone_mesh->setLookAt(vec3(0, -1, 0), vec3(0, 0, 1));
        scene().addObject(cone_mesh);
        
        try
        {
            m_textures[0] = gl::createTextureFromFile("Earth2.jpg", true);
            mat->addTexture(m_textures[0]);
            //mat->addTexture(m_textures[1]);
            mat->setShinyness(60);
            //mat->setShader(gl::createShader(gl::SHADER_PHONG));
//            mat->setShader(gl::createShaderFromFile("shader_normalMap.vert",
//                                                    "shader_normalMap.frag"));
        }catch(Exception &e)
        {
            LOG_ERROR<<e.what();
        }
        
        gl::LightPtr spot_light(new gl::Light(gl::Light::SPOT));
        //spot_light->transform() = glm::rotate(spot_light->transform(), 0.f, vec3(1, 0, 0));
        spot_light->setPosition(vec3(0, 300, 0));
        spot_light->setLookAt(vec3(0), vec3(1, 0, 0));
        
        spot_light->set_attenuation(0, .006f, 0);
        spot_light->set_specular(gl::Color(0.4));
        lights().push_back(spot_light);
        spot_light->set_spot_cutoff(45.f);
        spot_light->set_spot_exponent(15.f);
        
        gl::MeshPtr spot_mesh = gl::Mesh::create(gl::createSphere(10.f, 32), gl::Material::create());
        spot_light->children().push_back(spot_mesh);
        
        //lights().front()->set_enabled(false);
        lights().front()->set_diffuse(gl::Color(0.f, 0.4f, 0.f, 1.f));
        lights().front()->set_specular(gl::Color(0.f, 0.4f, 0.f, 1.f));
        
        for (auto &light : lights()){scene().addObject(light);}
        m_light_component->set_lights(lights());
                                     
        //gl::MeshPtr kafka_mesh = m_font.create_mesh(kinski::readFile("kafka_short.txt"));
        gl::MeshPtr kafka_mesh = m_font.create_mesh("Strauß, du hübscher Eiergäggelö!");
        kafka_mesh->setPosition(kafka_mesh->position() - kafka_mesh->boundingBox().center());
        //scene().addObject(kafka_mesh);
        
        // clear with transparent black
        gl::clearColor(gl::Color(0));
        
        for (int i = 0; i < 100; i++)
        {
            m_points.push_back(glm::linearRand(vec3(-windowSize() / 2.f, -2.f), vec3(windowSize() / 2.f, -2.f)));
        }
        
        // load state from config file(s)
        load_settings();
    }
    
    void update(float timeDelta)
    {
        ViewerApp::update(timeDelta);
        
        if(m_mesh)
        {
            m_mesh->material()->setWireframe(wireframe());
            m_mesh->material()->uniform("u_lightDir", light_direction());
            m_mesh->material()->setDiffuse(m_color->value());
            m_mesh->material()->setBlending(m_color->value().a < 1.0f);

        }
        for (int i = 0; i < materials().size(); i++)
        {
            materials()[i]->uniform("u_time",getApplicationTime());
            materials()[i]->uniform("u_lightDir", light_direction());
            materials()[i]->setShinyness(*m_shinyness);
            materials()[i]->setAmbient(0.2 * clear_color());
        }
    }
    
    void draw()
    {
        // draw block
        {
            //background
            //gl::drawTexture(m_textures[0], windowSize());
            gl::setMatrices(camera());
            
            if(draw_grid()){ gl::drawGrid(500, 500, 20, 20); }
            
            //gl::drawCircle(m_frameBuffer.getSize() / 2.f, 320.f, false);
            //gl::drawLine(vec2(0), windowSize(), gl::Color(), 50.f);
            
            
            scene().render(camera());
            //gl::render_to_texture(scene(), m_frameBuffer, camera());
            //gl::drawTexture(m_frameBuffer.getTexture(), windowSize());
            //render_with_post_processing(m_frameBuffer, m_post_process_mat);
            
            if(selected_mesh())
            {
                gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix() * selected_mesh()->transform());
                gl::drawAxes(selected_mesh());
                gl::drawBoundingBox(selected_mesh());
                if(normals()) gl::drawNormals(selected_mesh());
                
                //            gl::drawPoints(selected_mesh()->geometry()->vertexBuffer().id(),
                //                           selected_mesh()->geometry()->vertices().size());
                
                if(selected_mesh()->geometry()->hasBones())
                {
                    static gl::MaterialPtr point_mat;
                    if(!point_mat)
                    {
                        point_mat = gl::Material::create(gl::createShader(gl::SHADER_POINTS_SPHERE));
                        point_mat->setPointSize(24.f);
                        point_mat->setPointAttenuation(0.f, 40.f, 0.f);
                    }
                    point_mat->uniform("u_lightDir", light_direction());
                    
                    vector<vec3> points;
                    buildSkeleton(selected_mesh()->rootBone(), points);
                    gl::drawPoints(points, point_mat);
                    gl::drawLines(points, vec4(1, 0, 0, 1), 3.f);
                }
                // Label
                gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix() * m_label->transform());
                m_label->setRotation(glm::mat3(camera()->transform()));
                gl::drawMesh(m_label);
            }
        }// FBO block
        
//        gl::drawTexture(m_textures[0], windowSize());
//        gl::drawTexture(m_frameBuffer.getTexture(), windowSize() );
        
        // draw texture map(s)
        if(displayTweakBar())
        {
            m_frameBuffer.getTexture().set_mipmapping();
            
            float w = (windowSize()/8.f).x;
            float h = m_frameBuffer.getTexture().getHeight() * w / m_frameBuffer.getTexture().getWidth();
            glm::vec2 offset(getWidth() - w - 10, 10);
            glm::vec2 step(0, h + 10);
            
            if(m_mesh)
            {
                for(int i = 0;i < m_mesh->materials().size();i++)
                {
                    gl::MaterialPtr m = m_mesh->materials()[i];
                    
                    for (int j = 0; j < m->textures().size(); j++)
                    {
                        const gl::Texture &t = m->textures()[j];
                        
                        float h = t.getHeight() * w / t.getWidth();
                        glm::vec2 step(0, h + 10);
                        drawTexture(t, vec2(w, h), offset);
                        gl::drawText2D(as_string(t.getWidth()) + std::string(" x ") +
                                       as_string(t.getHeight()), m_font, glm::vec4(1),
                                       offset);
                        offset += step;
                    }
                    
                }
            }
            // draw fps string
            gl::drawText2D(kinski::as_string(framesPerSec()), m_font,
                           vec4(vec3(1) - clear_color().xyz(), 1.f),
                           glm::vec2(windowSize().x - 110, windowSize().y - 70));
        }
    }
    
    void buildSkeleton(gl::BonePtr currentBone, vector<vec3> &points)
    {
        if(!currentBone) return;
        list<gl::BonePtr>::iterator it = currentBone->children.begin();
        for (; it != currentBone->children.end(); ++it)
        {
            mat4 globalTransform = currentBone->worldtransform;
            mat4 childGlobalTransform = (*it)->worldtransform;
            points.push_back(globalTransform[3].xyz());
            points.push_back(childGlobalTransform[3].xyz());
            
            buildSkeleton(*it, points);
        }
    }

    void resize(int w, int h)
    {
        ViewerApp::resize(w, h);
        gl::Fbo::Format fboFormat;
        m_frameBuffer = gl::Fbo(w, h, fboFormat);
    }
    
    void mousePress(const MouseEvent &e)
    {
        ViewerApp::mousePress(e);
        
        if(selected_mesh())
        {
            m_label = m_font.create_mesh("Du liest Kafka: " + kinski::as_string(selected_mesh()->getID()));
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
        ViewerApp::updateProperty(theProperty);
        
        // one of our porperties was changed
        if(theProperty == m_color)
        {
            if(selected_mesh()) selected_mesh()->material()->setDiffuse(*m_color);
        }
        else if(theProperty == m_shinyness)
        {
            m_mesh->material()->setShinyness(*m_shinyness);
        }
        else if(theProperty == m_modelPath)
        {
            try
            {
                gl::MeshPtr m = gl::AssimpConnector::loadModel(*m_modelPath);
                scene().removeObject(m_mesh);
                m_mesh = m;
                m->material()->setShinyness(*m_shinyness);
                m->material()->setSpecular(glm::vec4(1));
                scene().addObject(m_mesh);
            } catch (Exception &e){ LOG_ERROR<< e.what(); }
        }
    }
    
    void tearDown()
    {
        LOG_PRINT<<"ciao simple geometry";
    }
    
    void render_with_post_processing(gl::Fbo &the_fbo, const gl::MaterialPtr &the_post_process_mat)
    {
        the_post_process_mat->textures().clear();
        the_post_process_mat->addTexture(the_fbo.getTexture());
        the_post_process_mat->addTexture(the_fbo.getDepthTexture());
        the_post_process_mat->uniform("u_window_size", the_fbo.getSize());
        gl::drawQuad(the_post_process_mat, gl::windowDimension());
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new SimpleGeometryApp);
    theApp->setWindowSize(1024, 600);
    AppServer s(theApp);
    
    return theApp->run();
}
