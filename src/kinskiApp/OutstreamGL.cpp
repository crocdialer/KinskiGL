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
    
    OutstreamGL::OutstreamGL(const gl::Font &the_font):
    ios(0),
    ostream(new StreamBufferGL(this)),
    m_font(the_font)
    {
        
    }
    
    OutstreamGL::~OutstreamGL()
    {
        delete rdbuf();
    }
    
    void OutstreamGL::draw()
    {
        glm::vec2 step(0, m_font.getLineHeight() * 1.1f);
        glm::vec2 offset(windowDimension().x / 2.f, step.y);
        
        for (const string &line : m_lines)
        {
            gl::drawText2D(line.substr(0, line.size() - 1), m_font, gl::Color(1), windowDimension().y - offset);
            offset += step;
        }
        if(m_lines.size() > 10) m_lines.pop_back();
    }
    
    
}}//namespace