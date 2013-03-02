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
    m_precise_selection(false),
    m_rotation_damping (.9)
    {
        /*********** init our application properties ******************/
        m_show_tweakbar = Property_<bool>::create("Show Tweakbar", true);
        m_show_tweakbar->setTweakable(false);
        registerProperty(m_show_tweakbar);
        
        m_window_size = Property_<glm::vec2>::create("Window Size", windowSize()) ;
        m_window_size->setTweakable(false);
        registerProperty(m_window_size);
        
        m_distance = RangedProperty<float>::create("view distance", 25, 0, 500);
        registerProperty(m_distance);
        
        m_rotation = Property_<glm::mat3>::create("Geometry Rotation", glm::mat3());
        registerProperty(m_rotation);
        
        m_rotationSpeed = RangedProperty<float>::create("Rotation Speed", 0, -100, 100);
        registerProperty(m_rotationSpeed);
        
        m_wireFrame = Property_<bool>::create("Wireframe", false);
        registerProperty(m_wireFrame);
        
        m_drawNormals = Property_<bool>::create("Normals", false);
        registerProperty(m_drawNormals);
        
        m_light_direction = Property_<glm::vec3>::create("Light dir", glm::vec3(1));
        registerProperty(m_light_direction);
        
        m_clear_color = Property_<glm::vec4>::create("Clear color", glm::vec4(0 ,0 ,0, 1.0));
        registerProperty(m_clear_color);
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
        
        setBarColor(glm::vec4(0, 0, 0, .5));
        setBarSize(glm::ivec2(250, 500));
        
        // enable observer mechanism
        observeProperties();
    }
    
    void ViewerApp::update(const float timeDelta)
    {
        m_camera->setAspectRatio(getAspectRatio());
        
        m_avg_filter.push(glm::vec2(0));
        m_inertia *= m_rotation_damping;
        
        if(!m_mouse_down && glm::length2(m_inertia) > 0.0025)
        {
            *m_rotation = glm::mat3_cast(glm::quat(*m_rotation) *
                                    glm::quat(glm::vec3(glm::radians(-m_inertia.y),
                                                        glm::radians(-m_inertia.x), 0)));
        }
        else if(!m_mouse_down || displayTweakBar())
        {
            *m_rotation = glm::mat3( glm::rotate(glm::mat4(m_rotation->value()),
                                                 *m_rotationSpeed * timeDelta,
                                                 glm::vec3(0, 1, .5)));
        }
        
    }
    
    void ViewerApp::mousePress(const MouseEvent &e)
    {
        m_clickPos = glm::vec2(e.getX(), e.getY());
        m_lastTransform = *m_rotation;
        m_mouse_down = true;
        
        if(gl::Object3DPtr picked_obj = m_scene.pick(gl::calculateRay(m_camera, e.getX(), e.getY()),
                                                     m_precise_selection))
        {
            LOG_TRACE<<"picked id: "<< picked_obj->getID();
            
            if( gl::MeshPtr m = std::dynamic_pointer_cast<gl::Mesh>(picked_obj)){
                
                if(m_selected_mesh != m)
                {
                    if(m_selected_mesh){ m_selected_mesh->material() = m_materials[0]; }
                    
                    m_selected_mesh = m;
                    m_materials[0] = m_selected_mesh->material();
                    m_materials[1]->shader() = m_materials[0]->shader();
                    m_selected_mesh->material() = m_materials[1];
                }
            }
        }
        else{
            if(e.isRight() && m_selected_mesh){
                m_selected_mesh->material() = m_materials[0];
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
        *m_distance -= e.getWheelIncrement();
    }
    
    void ViewerApp::keyPress(const KeyEvent &e)
    {
        GLFW_App::keyPress(e);
        
        switch (e.getChar())
        {
            case KeyEvent::KEY_SPACE:
                *m_show_tweakbar = !*m_show_tweakbar;
                break;
                
            case KeyEvent::KEY_s:
                Serializer::saveComponentState(shared_from_this(), "config.json", PropertyIO_GL());
                break;
                
            case KeyEvent::KEY_r:
                try
                {
                    m_inertia = glm::vec2(0);
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
    
    void ViewerApp::resize(int w, int h)
    {
        *m_window_size = glm::vec2(w, h);
    }
    
    // Property observer callback
    void ViewerApp::updateProperty(const Property::ConstPtr &theProperty)
    {
        if(theProperty == m_show_tweakbar)
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
//            if(m_selected_mesh)
//                look_at = gl::OBB(m_selected_mesh->boundingBox(), m_selected_mesh->transform()).center;
            
            glm::mat4 tmp = glm::mat4(m_rotation->value());
            tmp[3] = glm::vec4(look_at + m_rotation->value()[2] * m_distance->value(), 1.0f);
            m_camera->transform() = tmp;
        }
    }
}
