//
//  LightComponent.h
//  gl
//
//  Created by Fabian on 6/28/13.
//
//

#pragma once

#include "core/Component.hpp"
#include "gl/Material.hpp"

namespace kinski
{
    class KINSKI_API MaterialComponent : public kinski::Component
    {
    public:
        typedef std::shared_ptr<MaterialComponent> Ptr;
        
        MaterialComponent();
        ~MaterialComponent();
        
        void update_property(const Property::ConstPtr &theProperty);
        void set_index(int index);
        void set_materials(const std::vector<gl::MaterialPtr> &m, bool copy_settings = true);
        void refresh();
        
    private:
        std::vector<gl::MaterialPtr> m_materials;
        RangedProperty<int>::Ptr m_index;
        Property_<gl::Color>::Ptr m_ambient, m_diffuse, m_specular;
        Property_<bool>::Ptr m_blending, m_write_depth, m_read_depth, m_two_sided;
        
        Property_<std::string>::Ptr m_shader_vert, m_shader_frag, m_shader_geom,
            m_texture_path_1, m_texture_path_2, m_texture_path_3, m_texture_path_4;
    };
}