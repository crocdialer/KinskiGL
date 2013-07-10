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
    
    gl::Fbo m_frameBuffer, m_mask_fbo, m_result_fbo;
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
    
    // dof test
    gl::MaterialPtr m_post_process_mat;
    Property_<float>::Ptr m_focal_length, m_focal_depth, m_fstop;
    Property_<bool>::Ptr m_show_focus;
    
public:
    
    void setup()
    {
        ViewerApp::setup();
        set_precise_selection(true);
        
        /******************** add search paths ************************/
        kinski::addSearchPath("~/Desktop/");
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
        
        m_focal_length = Property_<float>::create("Focal length", 25.0);
        registerProperty(m_focal_length);
        m_focal_depth = Property_<float>::create("Focal depth", 3.0);
        registerProperty(m_focal_depth);
        m_fstop = Property_<float>::create("F-stop", 2.0);
        registerProperty(m_fstop);
        m_show_focus = Property_<bool>::create("Show focus", false);
        registerProperty(m_show_focus);
        
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
        
        m_mask_fbo = gl::Fbo(getWidth() / 4.f, getHeight() / 4.f, fboFormat);
        m_result_fbo = gl::Fbo(getWidth(), getHeight(), fboFormat);
        
        // Depth of field post processing
        m_post_process_mat = gl::Material::create();
        m_post_process_mat->setShader(gl::createShaderFromFile("shader_depth.vert", "shader_dof.frag"));
        m_post_process_mat->uniform("u_z_near", camera()->near());
        m_post_process_mat->uniform("u_z_far", camera()->far());
        
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
        
        // groundplane
        gl::Geometry::Ptr plane_geom(gl::createPlane(1000, 1000));
        gl::MaterialPtr mat = gl::Material::create(gl::createShader(gl::SHADER_PHONG));
        materials().push_back(mat);
        gl::MeshPtr ground_plane = gl::Mesh::create(plane_geom, mat);
        ground_plane->setLookAt(vec3(0, -1, 0), vec3(0, 0, 1));
        scene().addObject(ground_plane);

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
        
        gl::LightPtr spot_light = lights()[1];
        spot_light->set_enabled();
        spot_light->setPosition(vec3(0, 300, 0));
        spot_light->setLookAt(vec3(0), vec3(1, 0, 0));
        spot_light->set_attenuation(0, .006f, 0);
        spot_light->set_specular(gl::Color(0.4));
        spot_light->set_spot_cutoff(45.f);
        spot_light->set_spot_exponent(15.f);
        
        // lightcone
        gl::Geometry::Ptr cone_geom(gl::createCone(100, 200, 32));
        gl::MeshPtr cone_mesh = gl::Mesh::create(cone_geom, gl::Material::create());
        cone_mesh->material()->setDiffuse(gl::Color(1.f, 1.f, .9f, .7f));
        cone_mesh->material()->setBlending();
        cone_mesh->material()->setDepthWrite(false);
        cone_mesh->transform() = glm::rotate(cone_mesh->transform(), 90.f, gl::X_AXIS);
        cone_mesh->position() += glm::vec3(0, 0, -200);
        
        gl::MeshPtr spot_mesh = gl::Mesh::create(gl::createSphere(10.f, 32), gl::Material::create());
        spot_light->children().push_back(spot_mesh);
        spot_light->children().push_back(cone_mesh);

        for (auto &light : lights()){scene().addObject(light);}

        // load state from config file(s)
        load_settings();
        
        m_light_component->set_lights(lights());
        m_light_component->refresh();
        
        resize(windowSize().x, windowSize().y);
        create_dof_test(scene());
        
        
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
            
            if(m_mesh)
                m_textures[1] = create_mask(m_mask_fbo, m_mesh);
            
            gl::setMatrices(camera());
            
            if(draw_grid()){ gl::drawGrid(500, 500, 20, 20); }
            
            //gl::drawCircle(m_frameBuffer.getSize() / 2.f, 320.f, false);
            //gl::drawLine(vec2(0), windowSize(), gl::Color(), 50.f);
            
            //scene().render(camera());
            m_textures[0] = gl::render_to_texture(scene(), m_frameBuffer, camera());
            
            m_textures[2] = apply_mask(m_result_fbo, m_textures[0], m_textures[1]);
            
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
            //m_frameBuffer.getTexture().set_mipmapping();
            
            float w = (windowSize()/8.f).x;
            float h = m_frameBuffer.getTexture().getHeight() * w / m_frameBuffer.getTexture().getWidth();
            glm::vec2 offset(getWidth() - w - 10, 10);
            glm::vec2 step(0, h + 10);
            
//            if(m_mesh)
//            {
//                for(int i = 0;i < m_mesh->materials().size();i++)
//                {
//                    gl::MaterialPtr m = m_mesh->materials()[i];
            
                    for (int j = 0; j < 4; j++)
                    {
                        const gl::Texture &t = m_textures[j];
                        
                        float h = t.getHeight() * w / t.getWidth();
                        glm::vec2 step(0, h + 10);
                        drawTexture(t, vec2(w, h), offset);
                        gl::drawText2D(as_string(t.getWidth()) + std::string(" x ") +
                                       as_string(t.getHeight()), m_font, glm::vec4(1),
                                       offset);
                        offset += step;
                    }
//                }
//            }
            
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
        m_mask_fbo = gl::Fbo(w / 4.f, h / 4.f, fboFormat);
        m_result_fbo = gl::Fbo(w, h, fboFormat);
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
        m_post_process_mat->uniform("u_focalLength", *m_focal_length);
        m_post_process_mat->uniform("u_focalDepth", *m_focal_depth);
        m_post_process_mat->uniform("u_fstop", *m_fstop);
        m_post_process_mat->uniform("u_showFocus", *m_show_focus);
        gl::drawQuad(the_post_process_mat, gl::windowDimension());
    }
    
    void create_dof_test(gl::Scene &the_scene)
    {
        gl::MaterialPtr mat = gl::Material::create(gl::createShader(gl::SHADER_PHONG));
        mat->addTexture(gl::createTextureFromFile("stone_color.jpg", true, true));
        
        for (int i = 0; i < 30; i++)
        {
            gl::MeshPtr box_mesh = gl::Mesh::create(gl::createBox(glm::linearRand(vec3(20), vec3(100))),
                                                    mat);
            box_mesh->transform() = glm::rotate(box_mesh->transform(), random(0.f, 180.f), gl::Y_AXIS);
            box_mesh->setPosition(glm::linearRand(vec3(-900), vec3(900)));
            box_mesh->position().y = -box_mesh->boundingBox().min.y;
            the_scene.addObject(box_mesh);
        } 
    }
    
    gl::Texture create_mask(gl::Fbo &theFbo, const gl::MeshPtr &mesh)
    {
        static gl::MeshPtr mask_mesh;
        if(!mask_mesh)
        {
            mask_mesh = gl::Mesh::create(mesh->geometry(), gl::Material::create());
        }
        
        mask_mesh->transform() = mesh->transform();
        
        // push framebuffer and viewport states
        gl::SaveViewPort sv; gl::SaveFramebufferBinding sfb;
        gl::setWindowDimension(theFbo.getSize());
        theFbo.bindFramebuffer();
        gl::clearColor(gl::Color(0));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if(m_mesh)
        {
            gl::setMatrices(camera());
            gl::drawMesh(mask_mesh);
        }
        gl::clearColor(clear_color());
        
        return theFbo.getTexture();
    }
    
    gl::Texture apply_mask(gl::Fbo &theFbo, gl::Texture &image, gl::Texture &mask)
    {
        static gl::MaterialPtr mat;
        if(!mat)
        {
            //mat = gl::Material::create(gl::createShaderFromFile("mask_shader.vert", "mask_shader.frag"));
            mat = gl::Material::create();
            
            mat->setDepthTest(false);
            mat->setDepthWrite(false);
        }
        mat->textures().clear();
        mat->addTexture(image);
        mat->addTexture(mask);
        
        // push framebuffer and viewport states
        gl::SaveViewPort sv; gl::SaveFramebufferBinding sfb;
        gl::setWindowDimension(theFbo.getSize());
        theFbo.bindFramebuffer();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // draw fullscreen quad with mask_shader
        gl::drawQuad(mat, theFbo.getSize());
        //gl::drawTexture(image, theFbo.getSize());

        return theFbo.getTexture();
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new SimpleGeometryApp);
    theApp->setWindowSize(1024, 768);
    AppServer s(theApp);
    
    return theApp->run();
}
