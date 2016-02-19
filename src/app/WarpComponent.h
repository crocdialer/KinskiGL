//
//  WarpComponent
//  app
//
//  Created by Fabian on 12/05/15.
//
//

#ifndef __app__WarpComponent__
#define __app__WarpComponent__

#include "core/Component.h"
#include "gl/QuadWarp.h"

namespace kinski
{
    class KINSKI_API WarpComponent : public kinski::Component
    {
    public:
        typedef std::shared_ptr<WarpComponent> Ptr;
        
        WarpComponent();
        ~WarpComponent();
        
        void update_property(const Property::ConstPtr &the_property);
        
        void reset();
        
        gl::QuadWarp& quad_warp(){ return m_quad_warp; }
        
    private:
        gl::QuadWarp m_quad_warp;
        
        Property_<float>::Ptr m_top_left_x, m_top_right_x, m_bottom_left_x, m_bottom_right_x,
        m_top_left_y, m_top_right_y, m_bottom_left_y, m_bottom_right_y;
    };
}
#endif /* defined(__app__WarpComponent__) */