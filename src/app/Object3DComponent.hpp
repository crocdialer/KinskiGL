// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  LightComponent.hpp
//
//  Created by Fabian on 6/28/13.

#pragma once

#include "core/Component.hpp"
#include "gl/Object3D.hpp"

namespace kinski
{
    DEFINE_CLASS_PTR(Object3DComponent);
    
    class KINSKI_API Object3DComponent : public kinski::Component
    {
    public:
        
        Object3DComponent();
        ~Object3DComponent();
        
        void update_property(const Property::ConstPtr &theProperty);
        void set_object(const gl::Object3DPtr &theObject){set_objects({theObject});};
        void refresh();
        
        const std::vector<gl::Object3DPtr>& objects() const {return m_objects;}
        std::vector<gl::Object3DPtr>& objects() {return m_objects;}
        void set_objects(const std::vector<gl::Object3DPtr> &the_objects, bool copy_settings = true);
        void set_objects(const std::vector<gl::MeshPtr> &the_objects, bool copy_settings = true);
        
        void set_index(int index);
        
    private:
        RangedProperty<int>::Ptr m_object_index;
        
        Property_<bool>::Ptr m_enabled;
        Property_<float>::Ptr m_position_x, m_position_y, m_position_z;
        Property_<glm::vec3>::Ptr m_scale;
        Property_<glm::mat3>::Ptr m_rotation;
        
        std::vector<gl::Object3DPtr> m_objects;
    };
}
