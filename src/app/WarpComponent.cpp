//
//  WarpComponent
//  app
//
//  Created by Fabian on 12/05/15.
//
//

#include "WarpComponent.h"

namespace kinski
{
    WarpComponent::WarpComponent()
    {
        set_name("QuadWarping");
        
        m_top_left_x = RangedProperty<float>::create("top left x", 0.f, -.2f, 1.2f);
        m_top_right_x = RangedProperty<float>::create("top right x", 1.f, -.2f, 1.2f);
        m_bottom_left_x = RangedProperty<float>::create("bottom left x", 0.f, -.2f, 1.2f);
        m_bottom_right_x = RangedProperty<float>::create("bottom right x", 1.f, -.2f, 1.2f);

        m_top_left_y = RangedProperty<float>::create("top left y", 0.f, -.2f, 1.2f);
        m_top_right_y = RangedProperty<float>::create("top right y", 0.f, -.2f, 1.2f);
        m_bottom_left_y = RangedProperty<float>::create("bottom left y", 1.f, -.2f, 1.2f);
        m_bottom_right_y = RangedProperty<float>::create("bottom right y", 1.f, -.2f, 1.2f);

        register_property(m_top_left_x);
        register_property(m_top_left_y);
        register_property(m_top_right_x);
        register_property(m_top_right_y);
        register_property(m_bottom_left_x);
        register_property(m_bottom_left_y);
        register_property(m_bottom_right_x);
        register_property(m_bottom_right_y);
        
        register_function("reset", std::bind(&WarpComponent::reset, this));
    }
    
    WarpComponent::~WarpComponent(){}
    
    void WarpComponent::reset()
    {
        *m_top_left_x = 0.f;
        *m_top_right_x = 1.f;
        *m_bottom_left_x = 0.f;
        *m_bottom_right_x = 1.f;
        
        *m_top_left_y = 0.f;
        *m_top_right_y = 0.f;
        *m_bottom_left_y = 1.f;
        *m_bottom_right_y = 1.f;
    }
    
    void WarpComponent::update_property(const Property::ConstPtr &the_property)
    {
        if(the_property == m_top_left_x || the_property == m_top_left_y)
        {
            m_quad_warp.control_point(0, 0) = gl::vec2(*m_top_left_x, *m_top_left_y);
        }
        else if(the_property == m_top_right_x || the_property == m_top_right_y)
        {
            m_quad_warp.control_point(1, 0) = gl::vec2(*m_top_right_x, *m_top_right_y);
        }
        else if(the_property == m_bottom_left_x || the_property == m_bottom_left_y)
        {
            m_quad_warp.control_point(0, 1) = gl::vec2(*m_bottom_left_x, *m_bottom_left_y);
        }
        else if(the_property == m_bottom_right_x || the_property == m_bottom_right_y)
        {
            m_quad_warp.control_point(1, 1) = gl::vec2(*m_bottom_right_x, *m_bottom_right_y);
        }
    }
}
