//
//  LightComponent.h
//  gl
//
//  Created by Fabian on 6/28/13.
//
//

#ifndef __gl__Object3DComponent__
#define __gl__Object3DComponent__

#include "core/Component.h"
#include "gl/Object3D.h"

namespace kinski
{
    class KINSKI_API Object3DComponent : public kinski::Component
    {
    public:
        typedef std::shared_ptr<Object3DComponent> Ptr;
        
        Object3DComponent();
        ~Object3DComponent();
        
        void updateProperty(const Property::ConstPtr &theProperty);
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
#endif /* defined(__gl__LightComponent__) */
