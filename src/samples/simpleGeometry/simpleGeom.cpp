#include "app/ViewerApp.h"
#include "gl/Fbo.h"
#include "AssimpConnector.h"
#include "app/LightComponent.h"
#include "app/Object3DComponent.h"
#include "core/Animation.h"

using namespace std;
using namespace kinski;
using namespace glm;

class SimpleGeometryApp : public ViewerApp
{
private:
    
    gl::Fbo m_frameBuffer;
    gl::Texture m_textures[4];
    gl::MeshPtr m_mesh, m_label;
    gl::Font m_font;
    
    gl::MaterialPtr m_draw_depth_material;
    
    Property_<bool>::Ptr m_use_phong;
    Property_<bool>::Ptr m_use_fbo;
    Property_<string>::Ptr m_modelPath;
    RangedProperty<float>::Ptr m_animationTime;
    Property_<glm::vec4>::Ptr m_color;
    Property_<float>::Ptr m_shinyness;
    
    vector<vec3> m_points;
    
    LightComponent::Ptr m_light_component;
    Object3DComponent::Ptr m_object_component;
    
    // dof test
    gl::MaterialPtr m_post_process_mat;
    Property_<float>::Ptr m_focal_range, m_focal_depth, m_fstop;
    Property_<bool>::Ptr m_show_focus, m_auto_focus;
    
    // animation test
    animation::AnimationPtr m_animation, m_property_animation;
    std::list<animation::AnimationPtr> m_animations;
    
    gl::OutstreamGL m_outstream;
    
public:
    
    void setup()
    {
        ViewerApp::setup();
        set_precise_selection(true);
        
        /******************** add search paths ************************/
        kinski::add_search_path("~/Desktop/creatures", true);
        kinski::add_search_path("~/Desktop/doom3_base", true);
        kinski::add_search_path("/Library/Fonts");
        
        m_font.load("Courier New Bold.ttf", 24);

        // set font for graphical outstream
        outstream_gl().set_font(m_font);
        
        /*********** init our application properties ******************/
        
        m_use_phong = Property_<bool>::create("Use Phong shader (per pixel)", true);
        registerProperty(m_use_phong);
        
        m_use_fbo = Property_<bool>::create("Use FBO", false);
        registerProperty(m_use_fbo);
        
        m_modelPath = Property_<string>::create("Model path", "duck.dae");
        registerProperty(m_modelPath);

        m_animationTime = RangedProperty<float>::create("Animation time", 0, 0, 1);
        registerProperty(m_animationTime);
        
        m_color = Property_<glm::vec4>::create("Material color", glm::vec4(1 ,1 ,0, 0.6));
        registerProperty(m_color);
        
        m_shinyness = Property_<float>::create("Shinyness", 1.0);
        registerProperty(m_shinyness);
        
        m_focal_range = Property_<float>::create("Focal range", 25.0);
        registerProperty(m_focal_range);
        m_focal_depth = Property_<float>::create("Focal depth", 3.0);
        registerProperty(m_focal_depth);
        m_fstop = Property_<float>::create("F-stop", 2.0);
        //registerProperty(m_fstop);
        m_show_focus = Property_<bool>::create("Show focus", false);
        registerProperty(m_show_focus);
        m_auto_focus = Property_<bool>::create("Auto focus", false);
        registerProperty(m_auto_focus);
        
        create_tweakbar_from_component(shared_from_this());

        // enable observer mechanism
        observeProperties();
        
        // light component
        m_light_component.reset(new LightComponent());
        create_tweakbar_from_component(m_light_component);
        
        // object component
        m_object_component.reset(new Object3DComponent());
        create_tweakbar_from_component(m_object_component);
        
        /********************** construct a simple scene ***********************/
        camera()->setClippingPlanes(1, 5000);
        
        // configure fbo
        int numsamples = m_frameBuffer.getMaxSamples();//4
        gl::Fbo::Format fboFormat;
        fboFormat.setSamples(numsamples);
        m_frameBuffer = gl::Fbo(getWidth(), getHeight(), fboFormat);
        
        // Depth of field post processing
//        m_post_process_mat = gl::Material::create();
//        m_post_process_mat->setShader(gl::createShaderFromFile("shader_depth.vert", "DoF_bokeh_2.2.frag"));
//        m_post_process_mat->uniform("u_znear", camera()->near());
//        m_post_process_mat->uniform("u_zfar", camera()->far());
        
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
        gl::Geometry::Ptr plane_geom(gl::Geometry::createPlane(1000, 1000, 2, 2));
        gl::MaterialPtr mat = gl::Material::create(gl::createShader(gl::SHADER_PHONG));
        materials().push_back(mat);
        gl::MeshPtr ground_plane = gl::Mesh::create(plane_geom, mat);
        ground_plane->setLookAt(vec3(0, -1, 0), vec3(0, 0, 1));
        scene().addObject(ground_plane);

        try
        {
            //m_textures[0] = gl::createTextureFromFile("Earth2.jpg", true);
            //mat->addTexture(m_textures[0]);
            mat->setShinyness(60);
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
        gl::Geometry::Ptr cone_geom(gl::Geometry::createCone(100, 200, 32));
        gl::MeshPtr cone_mesh = gl::Mesh::create(cone_geom, gl::Material::create());
        cone_mesh->material()->setDiffuse(gl::Color(1.f, 1.f, .9f, .7f));
        cone_mesh->material()->setBlending();
        cone_mesh->material()->setDepthWrite(false);
        cone_mesh->transform() = glm::rotate(cone_mesh->transform(), 90.f, gl::X_AXIS);
        cone_mesh->position() += glm::vec3(0, 0, -200);
        
        gl::MeshPtr spot_mesh = gl::Mesh::create(gl::Geometry::createSphere(10.f, 32), gl::Material::create());
        spot_light->children().push_back(spot_mesh);
        spot_light->children().push_back(cone_mesh);

        for (auto &light : lights()){scene().addObject(light);}
        
        // test animation
        m_property_animation = animation::create(m_animationTime, 0.f, 1.f, 5.f);
        m_property_animation->set_loop(animation::LOOP_BACK_FORTH);
        m_property_animation->set_ease_function(animation::EaseOutBounce(.5f));
        
        // add to animations to list
        m_animations.push_back(m_animation);
        m_animations.push_back(m_property_animation);

        
        // load state from config file(s)
        load_settings();
        
        m_light_component->set_lights(lights());
        m_light_component->refresh();
        
        resize(windowSize().x, windowSize().y);
        create_dof_test(scene());
        
        //m_animation->start(10.f);
    }
    
    void update(float timeDelta)
    {
        ViewerApp::update(timeDelta);
        
        if(m_mesh)
        {
            m_mesh->material()->setWireframe(wireframe());
            m_mesh->material()->setDiffuse(m_color->value());
            m_mesh->material()->setBlending(m_color->value().a < 1.0f);

        }
        for (int i = 0; i < materials().size(); i++)
        {
            materials()[i]->uniform("u_time",getApplicationTime());
            materials()[i]->setShinyness(*m_shinyness);
            materials()[i]->setAmbient(0.2 * clear_color());
        }
        
        if(selected_mesh())
        {
            m_object_component->set_object(selected_mesh());
            //selected_mesh()->setLookAt(camera());
            //selected_mesh()->setScale(5 * *m_animationTime);
        }
        
        if(m_animation)
            m_animation->update(timeDelta);
        
        m_property_animation->update(timeDelta);
        
        //for(auto &animation : m_animations){animation->update(timeDelta);}
    }
    
    void draw()
    {
        // draw block
        {
            //background
            //gl::drawTexture(m_textures[0], windowSize());
            
            
            gl::setMatrices(camera());
            if(draw_grid()){ gl::drawGrid(500, 500, 20, 20); }
            
            if(*m_use_fbo)
            {
                // render to fbo
                //m_textures[0] = gl::render_to_texture(scene(), m_frameBuffer, camera());
            
                m_textures[0] = gl::render_to_texture(m_frameBuffer, [&]{
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    gl::setMatrices(camera());
                    if(draw_grid()){ gl::drawGrid(500, 500, 20, 20); }
                    
                    gl::drawCircle(m_frameBuffer.getSize() / 2.f, 320.f, false);
                    gl::drawLine(vec2(0), windowSize(), gl::COLOR_OLIVE, 5.f);
                    scene().render(camera());
                });
                
                // draw the fbo output
                gl::drawTexture(m_textures[0], windowSize());
                
                //render_with_post_processing(m_frameBuffer, m_post_process_mat);
            }
            else
            {
                scene().render(camera());
            }
            
            draw_lights(lights());
            
            if(selected_mesh())
            {
                gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix() * selected_mesh()->transform());
                gl::drawAxes(selected_mesh());
                gl::drawBoundingBox(selected_mesh());
                if(normals()) gl::drawNormals(selected_mesh());
                
                if(selected_mesh()->geometry()->hasBones())
                {
                    static gl::MaterialPtr point_mat;
                    if(!point_mat)
                    {
                        point_mat = gl::Material::create(gl::createShader(gl::SHADER_POINTS_SPHERE));
                        point_mat->setPointSize(32.f);
                        point_mat->setPointAttenuation(0.f, 0.01f, 0.f);
                    }
                    
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
        }

        gl::drawCircle(m_frameBuffer.getSize() / 2.f, 320.f, false);
        gl::drawLine(vec2(0), windowSize(), gl::COLOR_OLIVE, 35.f);
        
        // draw texture map(s)
        if(displayTweakBar())
        {
            //m_frameBuffer.getTexture().set_mipmapping();
            
            float w = (windowSize()/8.f).x;
            float h = m_frameBuffer.getTexture().getHeight() * w / m_frameBuffer.getTexture().getWidth();
            glm::vec2 offset(getWidth() - w - 10, 10);
            glm::vec2 step(0, h + 10);
            
            for (int j = 0; j < 4; j++)
            {
                const gl::Texture &t = m_textures[j];
                if(!t) continue;
                
                float h = t.getHeight() * w / t.getWidth();
                glm::vec2 step(0, h + 10);
                drawTexture(t, vec2(w, h), offset);
                gl::drawText2D(as_string(t.getWidth()) + std::string(" x ") +
                               as_string(t.getHeight()), m_font, glm::vec4(1),
                               offset);
                offset += step;
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
        for (auto child_bone : currentBone->children)
        {
            mat4 globalTransform = currentBone->worldtransform;
            mat4 childGlobalTransform = child_bone->worldtransform;
            points.push_back(globalTransform[3].xyz());
            points.push_back(childGlobalTransform[3].xyz());
            
            buildSkeleton(child_bone, points);
        }
    }

    void resize(int w, int h)
    {
        ViewerApp::resize(w, h);
        gl::Fbo::Format fboFormat;
        fboFormat.setSamples(gl::Fbo::getMaxSamples());
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
            
//            m_property_animation = animation::create(&selected_mesh()->position(), vec3(0.f), vec3(100.f), 5.f);
//            m_property_animation->set_loop(animation::LOOP_BACK_FORTH);
          
            m_animation = animation::create(&selected_mesh()->position().y, 0.f, 100.f, 5.f);
            m_animation->set_ease_function(animation::EaseOutBounce());
            m_animation->set_loop(animation::LOOP_BACK_FORTH);
            
            m_animation->set_finish_callback([]{LOG_DEBUG<<"forth end";});
            m_animation->set_reverse_finish_callback([]{LOG_DEBUG<<"back end";});
            m_animation->set_start_callback([]{LOG_DEBUG<<"forth start";});
            m_animation->set_reverse_start_callback([]{LOG_DEBUG<<"back start";});
            
            m_animation->start(2);
        }
        
        // create a ray
        gl::Ray ray = gl::calculateRay(camera(), vec2(e.getPos()));
        
        // calculate intersection of ray and an origin-centered plane
        gl::Plane plane = gl::Plane(vec3(0, 0, 0), vec3(0, 1, 0));
        
        gl::ray_intersection ri = plane.intersect(ray);
        
        if(ri)
        {
            vec3 p = ray * ri.distance;
            LOG_DEBUG<<"hit at distance: "<<ri.distance;
            LOG_DEBUG<<glm::to_string(p);
        }
    }
    
    void keyPress(const KeyEvent &e)
    {
        ViewerApp::keyPress(e);
        
        switch (e.getCode())
        {
            default:
                break;
        }
    }
    
    // Property observer callback
    void updateProperty(const Property::ConstPtr &theProperty)
    {
        ViewerApp::updateProperty(theProperty);
        
        // one of our properties was changed
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
                if(!m) return;
                
                scene().removeObject(m_mesh);
                m_mesh = m;
                m->position().y = -m->boundingBox().min.y;
                m->material()->setShinyness(*m_shinyness);
                m->material()->setSpecular(glm::vec4(1));
                scene().addObject(m_mesh);
                m_use_phong->notifyObservers();
            } catch (Exception &e){ LOG_ERROR<< e.what(); }
        }
        else if(theProperty == m_use_phong)
        {
            gl::Shader shader = gl::createShader(*m_use_phong ? gl::SHADER_PHONG_SKIN :
                                                 gl::SHADER_GOURAUD);
            if(m_mesh){m_mesh->material()->setShader(shader);}
            if(selected_mesh()){selected_mesh()->material()->setShader(shader);}
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
        m_post_process_mat->uniform("u_fstop", *m_fstop);
        
        the_post_process_mat->uniform("u_window_size", the_fbo.getSize());
        m_post_process_mat->uniform("u_range", *m_focal_range);
        m_post_process_mat->uniform("u_autofocus", *m_auto_focus);
        m_post_process_mat->uniform("u_showFocus", *m_show_focus);
        m_post_process_mat->uniform("u_focalDepth", *m_focal_depth);
        
        m_post_process_mat->uniform("bgl_RenderedTexture", 0);
        m_post_process_mat->uniform("bgl_DepthTexture", 1);
        m_post_process_mat->uniform("bgl_RenderedTextureWidth", (float)the_fbo.getWidth());
        m_post_process_mat->uniform("bgl_RenderedTextureHeight", (float)the_fbo.getHeight());
        
                                    
        gl::drawQuad(the_post_process_mat, gl::windowDimension());
    }
    
    void create_dof_test(gl::Scene &the_scene)
    {
        gl::MaterialPtr mat = gl::Material::create(gl::createShader(gl::SHADER_GOURAUD));
        mat->addTexture(gl::createTextureFromFile("stone_color.jpg", true, true));
        
        for (int i = 0; i < 90; i++)
        {
            gl::MeshPtr box_mesh = gl::Mesh::create(gl::Geometry::createBox(glm::linearRand(vec3(20), vec3(100))),
                                                    mat);
            box_mesh->transform() = glm::rotate(box_mesh->transform(), random(0.f, 180.f), gl::Y_AXIS);
            box_mesh->setPosition(glm::linearRand(vec3(-2900), vec3(2900)));
            box_mesh->position().y = -box_mesh->boundingBox().min.y;
            the_scene.addObject(box_mesh);
        } 
    }
    
    void draw_lights(const std::vector<gl::LightPtr> &lights)
    {
        for(auto &light : lights)
        {
            static gl::MeshPtr sphere_mesh;
            if(!sphere_mesh)
            {
                sphere_mesh = gl::Mesh::create(gl::Geometry::createSphere(30.f, 32), gl::Material::create());
            }
            
            if(light->enabled() && light->type() != gl::Light::DIRECTIONAL)
            {
                gl::ScopedMatrixPush modelview(gl::MODEL_VIEW_MATRIX);
                gl::multMatrix(gl::MODEL_VIEW_MATRIX, light->global_transform());
                gl::drawMesh(sphere_mesh);
            }
        }
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new SimpleGeometryApp);
    theApp->setWindowSize(1024, 768);
    LOG_INFO<<net::local_ip();
    
    return theApp->run();
}
