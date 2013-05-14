//
//  OutstreamGL.cpp
//  kinskiGL
//
//  Created by Fabian on 5/13/13.
//
//

#include "OutstreamGL.h"

using namespace std;

namespace kinski{ namespace gl{
    
    OutstreamGL::OutstreamGL(const gl::Font &the_font)
    {
    
    }

    void OutstreamGL::draw()
    {
        list<string>::iterator it = m_lines.begin();
        for (; it != m_lines.end(); ++it)
        {
            //gl::drawText2D(<#const std::string &theText#>, <#const gl::Font &theFont#>)
        }
    }
    
    
}}//namespace