// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  LightComponent.cpp
//
//  Created by Fabian Schmidton 6/28/13.

#include "core/file_functions.hpp"
#include "MaterialComponent.hpp"

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
        register_property(m_index);
        register_property(m_ambient);
        register_property(m_diffuse);
        register_property(m_specular);
        register_property(m_blending);
        register_property(m_write_depth);
        register_property(m_read_depth);
        register_property(m_shader_vert);
        register_property(m_shader_frag);
        register_property(m_shader_geom);
        register_property(m_texture_path_1);
        register_property(m_texture_path_2);
        register_property(m_texture_path_3);
        register_property(m_texture_path_4);
        
        set_name("Materials");
    }
    
    MaterialComponent::~MaterialComponent(){}
    
    void MaterialComponent::update_property(const Property::ConstPtr &theProperty)
    {
        gl::MaterialPtr active_mat = m_materials.empty() ? gl::MaterialPtr() : m_materials[*m_index];
        if(!active_mat) return;
        
        if(theProperty == m_index)
        {
            refresh();
        }
        else if(theProperty == m_diffuse)
        {
            active_mat->set_diffuse(*m_diffuse);
        }
        else if(theProperty == m_ambient)
        {
            active_mat->set_ambient(*m_ambient);
        }
        else if(theProperty == m_specular)
        {
            active_mat->set_specular(*m_specular);
        }
        else if(theProperty == m_blending)
        {
            active_mat->set_blending(*m_blending);
        }
        else if(theProperty == m_write_depth)
        {
            active_mat->set_depth_write(*m_write_depth);
        }
        else if(theProperty == m_read_depth)
        {
            active_mat->set_depth_test(*m_read_depth);
        }
        else if(theProperty == m_shader_vert ||
                theProperty == m_shader_frag ||
                theProperty == m_shader_geom)
        {
            gl::ShaderPtr shader;
            
            if(!m_shader_vert->value().empty() && !m_shader_frag->value().empty())
            {
                try
                {
                    shader = gl::create_shader_from_file(*m_shader_vert, *m_shader_frag, *m_shader_geom);
                }
                catch(fs::FileNotFoundException &fe)
                {
                    if(!fe.file_name().empty()) LOG_DEBUG << fe.what();
                }
                catch(Exception &e)
                {
                    LOG_ERROR << e.what();
                    return;
                }
                
                if(shader)
                    active_mat->set_shader(shader);
            }
        }
        else if(theProperty == m_texture_path_1 ||
                theProperty == m_texture_path_2 ||
                theProperty == m_texture_path_3 ||
                theProperty == m_texture_path_4)
        {
            active_mat->clear_textures();
            
            std::list<std::string> tex_names = {*m_texture_path_1, *m_texture_path_2,
                *m_texture_path_3, *m_texture_path_4};
            
            for(const std::string& n : tex_names)
            {
                if(n.empty()) continue;
                
                try
                {
                    auto tex = gl::create_texture_from_file(n);
                    if(tex)
                    {
                        active_mat->add_texture(tex);
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
        m_index->set_range(0, m.size() - 1);
        
        if(copy_settings)
        {
            observe_properties();
            m_index->set(*m_index);
        }
    }
    
    void MaterialComponent::refresh()
    {
        observe_properties(false);
        
        gl::MaterialPtr mat = m_materials.empty() ? gl::MaterialPtr() : m_materials[*m_index];
        if(!mat) return;
        
        *m_diffuse = mat->diffuse();
        *m_ambient = mat->ambient();
        *m_specular = mat->specular();
        *m_blending = mat->blending();
        *m_write_depth = mat->depth_write();
        *m_read_depth = mat->depth_test();
        observe_properties(true);
    }
}
