//
//  OutstreamGL.h
//  kinskiGL
//
//  Created by Fabian on 5/13/13.
//
//

#ifndef __kinskiGL__OutstreamGL__
#define __kinskiGL__OutstreamGL__

#include "kinskiGL/KinskiGL.h"

namespace kinski{ namespace gl{
    
    class OutstreamGL : std::ostream
    {
    public:
        explicit OutstreamGL(const gl::Font &the_font);
        
        void set_num_lines(uint32_t the_num){m_num_lines = the_num;};
        uint32_t num_lines() const{return m_num_lines;};
        
        void draw();
        
    private:
        
        uint32_t m_num_lines;
        std::list<gl::MeshPtr> m_line_meshes;
    };
    
}}//namespace

#endif /* defined(__kinskiGL__OutstreamGL__) */
