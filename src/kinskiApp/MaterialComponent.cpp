//
//  LightComponent.cpp
//  kinskiGL
//
//  Created by Fabian on 6/28/13.
//
//

#include "MaterialComponent.h"

namespace kinski
{
    MaterialComponent::MaterialComponent():
    m_index(RangedProperty<int>::create("index", -1, -1, 256)),
    m_ambient(Property_<gl::Color>::create("ambient", gl::Color(0))),
    m_diffuse(Property_<gl::Color>::create("diffuse", gl::Color())),
    m_specular(Property_<gl::Color>::create("specular", gl::Color())),
    m_blending(Property_<bool>::create("blending", false)),
    m_write_depth(Property_<bool>::create("write_depth", true)),
    m_read_depth(Property_<bool>::create("read_depth", true)),
    m_shader_vert(Property_<std::string>::create("vertex shader", "")),
    m_shader_frag(Property_<std::string>::create("frgament shader", "")),
    m_shader_geom(Property_<std::string>::create("geometry shader", "")),
    m_texture_path_1(Property_<std::string>::create("texture 1", "")),
    m_texture_path_2(Property_<std::string>::create("texture 2", "")),
    m_texture_path_3(Property_<std::string>::create("texture 3", "")),
    m_texture_path_4(Property_<std::string>::create("texture 4", ""))
    {
        registerProperty(m_index);
        registerProperty(m_ambient);
        registerProperty(m_diffuse);
        registerProperty(m_specular);
        registerProperty(m_blending);
        registerProperty(m_write_depth);
        registerProperty(m_read_depth);
        registerProperty(m_shader_vert);
        registerProperty(m_shader_frag);
        registerProperty(m_shader_geom);
        registerProperty(m_texture_path_1);
        registerProperty(m_texture_path_2);
        registerProperty(m_texture_path_3);
        registerProperty(m_texture_path_4);
        
        set_name("Materials");
    }
    
    MaterialComponent::~MaterialComponent(){}
    
    void MaterialComponent::updateProperty(const Property::ConstPtr &theProperty)
    {
        gl::MaterialPtr active_mat = m_materials.empty() ? gl::MaterialPtr() : m_materials[*m_index];
        if(!active_mat) return;
        
        if(theProperty == m_index)
        {
            refresh();
        }
        else if(theProperty == m_diffuse)
        {
            active_mat->setDiffuse(*m_diffuse);
        }
        else if(theProperty == m_ambient)
        {
            active_mat->setAmbient(*m_ambient);
        }
        else if(theProperty == m_specular)
        {
            active_mat->setSpecular(*m_specular);
        }
        else if(theProperty == m_blending)
        {
            active_mat->setBlending(*m_blending);
        }
        else if(theProperty == m_write_depth)
        {
            active_mat->setDepthWrite(*m_write_depth);
        }
        else if(theProperty == m_read_depth)
        {
            active_mat->setDepthTest(*m_read_depth);
        }
        else if(theProperty == m_shader_vert ||
                theProperty == m_shader_frag ||
                theProperty == m_shader_geom)
        {
            gl::Shader shader;
            
            if(!m_shader_vert->value().empty() && !m_shader_frag->value().empty())
            {
                try
                {
                    shader = gl::createShaderFromFile(*m_shader_vert, *m_shader_frag, *m_shader_geom);
                }
                catch (FileNotFoundException &fe)
                {
                    if(!fe.file_name().empty()) LOG_DEBUG << fe.what();
                }
                catch (Exception &e)
                {
                    LOG_ERROR << e.what();
                    return;
                }
                
                if(shader)
                    active_mat->setShader(shader);
            }
        }
        else if(theProperty == m_texture_path_1 ||
                theProperty == m_texture_path_2 ||
                theProperty == m_texture_path_3 ||
                theProperty == m_texture_path_4)
        {
            active_mat->textures().clear();
            
            std::list<std::string> tex_names = {*m_texture_path_1, *m_texture_path_2,
                *m_texture_path_3, *m_texture_path_4};
            
            for(const std::string& n : tex_names)
            {
                if(n.empty()) continue;
                
                try
                {
                    auto tex = gl::createTextureFromFile(n);
                    if(tex)
                    {
                        active_mat->addTexture(tex);
                    }
                }
                catch (Exception &e)
                {
                    LOG_ERROR << e.what();
                }
            }
        }
    }
    
    void MaterialComponent::set_index(int index)
    {
        *m_index = index;
    }
    
    void MaterialComponent::set_materials(const std::vector<gl::MaterialPtr> &m, bool copy_settings)
    {
        m_materials.assign(m.begin(), m.end());
        m_index->setRange(0, m.size() - 1);
        
        if(copy_settings)
        {
            observeProperties();
            m_index->set(*m_index);
        }
    }
    
    void MaterialComponent::refresh()
    {
        observeProperties(false);
        
        gl::MaterialPtr mat = m_materials.empty() ? gl::MaterialPtr() : m_materials[*m_index];
        if(!mat) return;
        
        *m_diffuse = mat->diffuse();
        *m_ambient = mat->ambient();
        *m_specular = mat->specular();
        *m_blending = mat->blending();
        *m_write_depth = mat->depthWrite();
        *m_read_depth = mat->depthTest();
        observeProperties(true);
    }
}