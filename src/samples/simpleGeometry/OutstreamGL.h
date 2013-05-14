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
    
    class OutstreamGL : std::basic_ostream<char>
    {
    public:
        explicit OutstreamGL(const gl::Font &the_font);

        const std::list<std::string>& lines() const {return m_lines;};
        
        void draw();
        
    private:
        std::list<std::string> m_lines;
    };
    
}}//namespace

#endif /* defined(__kinskiGL__OutstreamGL__) */
