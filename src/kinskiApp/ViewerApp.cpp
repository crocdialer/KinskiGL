//
//  ViewerApp.cpp
//  kinskiGL
//
//  Created by Fabian on 3/1/13.
//
//

#include "ViewerApp.h"

namespace kinski {
    
    ViewerApp::ViewerApp():GLFW_App(),
    m_camera(new gl::PerspectiveCamera),
    m_precise_selection(true),
    m_center_selected(false),
    m_rotation_damping (.9)
    {
        /*********** init our application properties ******************/
        m_search_paths = Property_<std::vector<std::string> >::create("File search paths",
                                                                      std::vector<std::string>());
        m_search_paths->setTweakable(false);
        registerProperty(m_search_paths);
        
        m_logger_severity = RangedProperty<int>::create("Logger Severity", kinski::SEV_INFO, 0, 7);
        registerProperty(m_logger_severity);
        
        m_show_tweakbar = Property_<bool>::create("Show Tweakbar", true);
        m_show_tweakbar->setTweakable(false);
        registerProperty(m_show_tweakbar);
        
        m_window_size = Property_<glm::vec2>::create("Window Size", windowSize()) ;
        m_window_size->setTweakable(false);
        registerProperty(m_window_size);
        
        m_distance = RangedProperty<float>::create("view distance", 25, 0, 7500);
        registerProperty(m_distance);
        
        m_rotation = Property_<glm::mat3>::create("Geometry Rotation", glm::mat3());
        registerProperty(m_rotation);
        
        m_rotationSpeed = RangedProperty<float>::create("Rotation Speed", 0, -100, 100);
        registerProperty(m_rotationSpeed);
        
        m_draw_grid = Property_<bool>::create("Draw grid", true);
        registerProperty(m_draw_grid);
        
        m_wireFrame = Property_<bool>::create("Wireframe", false);
        registerProperty(m_wireFrame);
        
        m_drawNormals = Property_<bool>::create("Normals", false);
        registerProperty(m_drawNormals);
        
        m_light_direction = Property_<glm::vec3>::create("Light dir", glm::vec3(1));
        registerProperty(m_light_direction);
        
        m_clear_color = Property_<glm::vec4>::create("Clear color", glm::vec4(0 ,0 ,0, 1.0));
        registerProperty(m_clear_color);
        
        // viewer provides a directional light
        gl::LightPtr dir_light(new gl::Light(gl::Light::DIRECTIONAL));
        dir_light->setPosition(light_direction());
        lights().push_back(dir_light);
    }
    
    ViewerApp::~ViewerApp()
    {
        
    }
    
    void ViewerApp::setup()
    {
        
        m_materials.push_back(gl::MaterialPtr(new gl::Material));
        m_materials.push_back(gl::MaterialPtr(new gl::Material));
        m_materials[0]->setShader(gl::createShader(gl::SHADER_PHONG));
        m_materials[1]->setDiffuse(glm::vec4(0, 1, 0, 1));
        
        // enable observer mechanism
        observeProperties();
    }
    
    void ViewerApp::update(float timeDelta)
    {
        m_camera->setAspectRatio(getAspectRatio());
        m_avg_filter.push(glm::vec2(0));
        m_inertia *= m_rotation_damping;
        
        // rotation from inertia
        if(!m_mouse_down && glm::length2(m_inertia) > 0.0025)
        {
            *m_rotation = glm::mat3_cast(glm::quat(*m_rotation) *
                                         glm::quat(glm::vec3(glm::radians(-m_inertia.y),
                                                             glm::radians(-m_inertia.x), 0)));
        }
        // rotation from fixed rotationspeed
        else if(!m_mouse_down || displayTweakBar())
        {
            *m_rotation = glm::mat3( glm::rotate(glm::mat4(m_rotation->value()),
                                                 *m_rotationSpeed * timeDelta,
                                                 glm::vec3(0, 1, .5)));
        }
        m_scene.update(timeDelta);
    }
    
    void ViewerApp::mousePress(const MouseEvent &e)
    {
        m_arcball.mouseDown(e.getPos());
        
        m_clickPos = glm::vec2(e.getX(), e.getY());
        m_lastTransform = *m_rotation;
        m_mouse_down = true;
        
        if(gl::Object3DPtr picked_obj = m_scene.pick(gl::calculateRay(m_camera, e.getX(), e.getY()),
                                                     m_precise_selection))
        {
            LOG_TRACE<<"picked id: "<< picked_obj->getID();
            if( gl::MeshPtr m = std::dynamic_pointer_cast<gl::Mesh>(picked_obj))
            {
                if(m_selected_mesh != m)
                {
//                    if(m_selected_mesh){ m_selected_mesh->material() = m_materials[0]; }
                    m_selected_mesh = m;
//                    m_materials[0] = m_selected_mesh->material();
//                    m_materials[1]->shader() = m_materials[0]->shader();
//                    m_selected_mesh->material() = m_materials[1];
                }
            }
        }
        else{
            if(e.isRight() && m_selected_mesh){
//                m_selected_mesh->material() = m_materials[0];
                m_selected_mesh.reset();
            }
        }
    }
    
    void ViewerApp::mouseDrag(const MouseEvent &e)
    {
        glm::vec2 mouseDiff = glm::vec2(e.getX(), e.getY()) - m_clickPos;
        
        if(e.isLeft() && (e.isAltDown() || !displayTweakBar()))
        {
            *m_rotation = glm::mat3_cast(glm::quat(m_lastTransform) *
                                         glm::quat(glm::vec3(glm::radians(-mouseDiff.y),
                                                   glm::radians(-mouseDiff.x), 0)));
            
//            m_arcball.mouseDrag(e.getPos());
//            *m_rotation = glm::mat3_cast(m_arcball.getQuat());
        }
        m_avg_filter.push(glm::vec2(e.getX(), e.getY()) - m_dragPos);
        m_dragPos = glm::vec2(e.getX(), e.getY());
    }
    
    void ViewerApp::mouseRelease(const MouseEvent &e)
    {
        m_mouse_down = false;
        if(!displayTweakBar())
            m_inertia = m_avg_filter.filter();
    }
    
    void ViewerApp::mouseWheel(const MouseEvent &e)
    {
        *m_distance -= e.getWheelIncrement().y;
    }
    
    void ViewerApp::keyPress(const KeyEvent &e)
    {
        GLFW_App::keyPress(e);
        
        if(e.getChar() == KeyEvent::KEY_SPACE)
        {
            *m_show_tweakbar = !*m_show_tweakbar;
        }
        
        if(!displayTweakBar())
        {
            switch (e.getChar())
            {
                case KeyEvent::KEY_c:
                    m_center_selected = !m_center_selected;
                    break;
                    
                case KeyEvent::KEY_s:
                    Serializer::saveComponentState(shared_from_this(), "config.json", PropertyIO_GL());
                    break;
                    
                case KeyEvent::KEY_r:
                    try
                    {
                        m_inertia = glm::vec2(0);
                        m_selected_mesh.reset();
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
    }
    
    void ViewerApp::resize(int w, int h)
    {
        *m_window_size = glm::vec2(w, h);
        
        m_arcball.setWindowSize( windowSize() );
        m_arcball.setCenter( windowSize() / 2.f );
        m_arcball.setRadius( 150 );
    }
    
    // Property observer callback
    void ViewerApp::updateProperty(const Property::ConstPtr &theProperty)
    {
        if(theProperty == m_search_paths)
        {
            for (const auto &search_path : m_search_paths->value())
            {
                kinski::addSearchPath(search_path);
            }
        }
        else if(theProperty == m_logger_severity)
        {
            Logger::get()->setSeverity(static_cast<Severity>(m_logger_severity->value()));
        }
        else if(theProperty == m_show_tweakbar)
        {
            set_displayTweakBar(*m_show_tweakbar);
        }
        else if(theProperty == m_window_size)
        {
            setWindowSize(*m_window_size);
            // only set this once
            m_window_size->removeObserver(shared_from_this());
        }
        else if(theProperty == m_clear_color)
        {
            gl::clearColor(*m_clear_color);
        }
        else if(theProperty == m_distance || theProperty == m_rotation)
        {
            glm::vec3 look_at;
            if(m_selected_mesh && m_center_selected)
                look_at = gl::OBB(m_selected_mesh->boundingBox(), m_selected_mesh->transform()).center;
            
            glm::mat4 tmp = glm::mat4(m_rotation->value());
            tmp[3] = glm::vec4(look_at + m_rotation->value()[2] * m_distance->value(), 1.0f);
            m_camera->transform() = tmp;
        }
    }
}
