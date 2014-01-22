//
//  LightComponent.h
//  kinskiGL
//
//  Created by Fabian on 6/28/13.
//
//

#ifndef __kinskiGL__Object3DComponent__
#define __kinskiGL__Object3DComponent__

#include "kinskiCore/Component.h"
#include "kinskiGL/Object3D.h"

namespace kinski
{
    class KINSKI_API Object3DComponent : public kinski::Component
    {
    public:
        typedef std::shared_ptr<Object3DComponent> Ptr;
        
        Object3DComponent();
        ~Object3DComponent();
        
        void updateProperty(const Property::ConstPtr &theProperty);
        void setObject(const gl::Object3DPtr &theObject);
        gl::Object3DPtr object() {return m_object;}
        void refresh();
        
    private:
        Property_<bool>::Ptr m_enabled;
        Property_<float>::Ptr m_position_x, m_position_y, m_position_z;
        Property_<glm::vec3>::Ptr m_scale;
        Property_<glm::mat3>::Ptr m_rotation;
        
        gl::Object3DPtr m_object;
    };
}
#endif /* defined(__kinskiGL__LightComponent__) */
