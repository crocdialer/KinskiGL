//
//  Object3DComponent.cpp
//  kinskiGL
//
//  Created by Fabian on 6/28/13.
//
//

#include "Object3DComponent.h"

namespace kinski
{
    Object3DComponent::Object3DComponent():
    m_enabled(Property_<bool>::create("enabled", true)),
    m_position_x(Property_<float>::create("position X", 0)),
    m_position_y(Property_<float>::create("position Y", 0)),
    m_position_z(Property_<float>::create("position Z", 0)),
    m_scale(Property_<glm::vec3>::create("scale", glm::vec3(1))),
    m_rotation(Property_<glm::mat3>::create("rotation", glm::mat3()))
    {
        registerProperty(m_enabled);
        registerProperty(m_position_x);
        registerProperty(m_position_y);
        registerProperty(m_position_z);
        registerProperty(m_scale);
        registerProperty(m_rotation);
        set_name("3D_Objects");
    }
    
    Object3DComponent::~Object3DComponent(){}
    
    void Object3DComponent::updateProperty(const Property::ConstPtr &theProperty)
    {
        if(!m_object)
        {
            LOG_ERROR << "could not update: no component set ...";
            return;
        }
            
        if(theProperty == m_enabled)
        {
            m_object->set_enabled(*m_enabled);
        }
        else if(theProperty == m_position_x || theProperty == m_position_y || theProperty == m_position_z)
        {
            m_object->setPosition(glm::vec3(*m_position_x, *m_position_y, *m_position_z));
        }
        else if(theProperty == m_scale)
        {
            m_object->setScale(*m_scale);
        }
        else if(theProperty == m_rotation)
        {
            m_object->setRotation(*m_rotation);
        }
    }
    
    void Object3DComponent::setObject(const gl::Object3DPtr &theObject)
    {
        if(theObject)
        {
            m_object = theObject;
            refresh();
        }
    }
    
    void Object3DComponent::refresh()
    {
        observeProperties(false);
        *m_enabled = m_object->enabled();
        *m_position_x = m_object->position().x;
        *m_position_y = m_object->position().y;
        *m_position_z = m_object->position().z;
        *m_scale = m_object->scale();
        *m_rotation = glm::mat3_cast(m_object->rotation());
        
        observeProperties(true);
    }
}
