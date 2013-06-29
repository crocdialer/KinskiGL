//
//  LightComponent.h
//  kinskiGL
//
//  Created by Fabian on 6/28/13.
//
//

#ifndef __kinskiGL__LightComponent__
#define __kinskiGL__LightComponent__

#include "kinskiCore/Component.h"
#include "kinskiGL/Light.h"

namespace kinski
{
    class KINSKI_API LightComponent : public kinski::Component
    {
    public:
        typedef std::shared_ptr<LightComponent> Ptr;
        
        LightComponent();
        ~LightComponent();
        
        void updateProperty(const Property::ConstPtr &theProperty);
        void set_lights(const std::list<gl::LightPtr> &l);
        
    private:
        std::vector<gl::LightPtr> m_lights;
        RangedProperty<int>::Ptr m_light_index;
        Property_<bool>::Ptr m_enabled;
        Property_<float>::Ptr m_position_x, m_position_y, m_position_z;
        Property_<glm::vec3>::Ptr m_direction;
        Property_<gl::Color>::Ptr m_ambient, m_diffuse, m_specular;
        RangedProperty<float>::Ptr m_att_constant, m_att_linear, m_att_quadratic;
        RangedProperty<float>::Ptr m_spot_cutoff, m_spot_exponent;
        
        void refresh();
        void set_values();
    };
}
#endif /* defined(__kinskiGL__LightComponent__) */
