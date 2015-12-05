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
        
        m_top_left = Property_<gl::vec2>::create("top left", gl::vec2(0.f, 0.f));
        m_top_right = Property_<gl::vec2>::create("top right", gl::vec2(1.f, 0.f));
        m_bottom_left = Property_<gl::vec2>::create("bottom left", gl::vec2(0.f, 1.f));
        m_bottom_right = Property_<gl::vec2>::create("bottom right", gl::vec2(1.f, 1.f));
        
        register_property(m_top_left);
        register_property(m_top_right);
        register_property(m_bottom_left);
        register_property(m_bottom_right);
        
        register_function("reset", std::bind(&WarpComponent::reset, this));
    }
    
    WarpComponent::~WarpComponent(){}
    
    void WarpComponent::reset()
    {
        *m_top_left = gl::vec2(0.f, 0.f);
        *m_top_right = gl::vec2(1.f, 0.f);
        *m_bottom_left = gl::vec2(0.f, 1.f);
        *m_bottom_right = gl::vec2(1.f, 1.f);
    }
    
    void WarpComponent::update_property(const Property::ConstPtr &the_property)
    {
        if(the_property == m_top_left)
        {
            m_quad_warp.control_point(0, 0) = *m_top_left;
        }
        else if(the_property == m_top_right)
        {
            m_quad_warp.control_point(1, 0) = *m_top_right;
        }
        else if(the_property == m_bottom_left)
        {
            m_quad_warp.control_point(0, 1) = *m_bottom_left;
        }
        else if(the_property == m_bottom_right)
        {
            m_quad_warp.control_point(1, 1) = *m_bottom_right;
        }
    }
}
