//
//  OutstreamGL.cpp
//  kinskiGL
//
//  Created by Fabian on 5/13/13.
//
//

#include "OutstreamGL.h"
#include "kinskiGL/Material.h"

using namespace std;

namespace kinski{ namespace gl{
    
    OutstreamGL::OutstreamGL(uint32_t max_lines):
    ios(0),
    ostream(new StreamBufferGL(this)),
    m_max_lines(max_lines)
    {
        
    }
    
    OutstreamGL::OutstreamGL(const gl::Font &the_font, uint32_t max_lines):
    ios(0),
    ostream(new StreamBufferGL(this)),
    m_font(the_font),
    m_max_lines(max_lines)
    {
        
    }
    
    OutstreamGL::~OutstreamGL()
    {
        delete rdbuf();
    }
    
    void OutstreamGL::draw()
    {
        if(!m_font) return;
        
        if(m_lines.size() > m_max_lines) m_lines.pop_back();
        
        glm::vec2 step(0, m_font.getLineHeight() * 1.1f);
        glm::vec2 offset(10, windowDimension().y - step.y);
        
        int i = m_lines.size();
        for (const string &line : m_lines)
        {
            gl::Color color = gl::Color(1, 1, 1, (float) i / m_lines.size());
            if(line.find("WARNING") != std::string::npos){color = gl::COLOR_ORANGE;}
            else if(line.find("ERROR") != std::string::npos){color = gl::COLOR_RED;}

            gl::drawText2D(line.substr(0, line.size() - 1), m_font, color, offset);
            offset -= step;
            i--;
        }
    }
    
    StreamBufferGL::StreamBufferGL(OutstreamGL *ostreamGL, size_t buff_sz):
    m_outstreamGL(ostreamGL),
    m_buffer(buff_sz + 1)
    {
        //set putbase pointer and endput pointer
        char *base = &m_buffer[0];
        setp(base, base + buff_sz);
    }

    
    // flush the characters in the buffer
    int StreamBufferGL::flushBuffer ()
    {
        int num = pptr() - pbase();
        
        // pass the flushed char sequence
        m_outstreamGL->lines().push_front(std::string(pbase(), pptr()));
        
        pbump(-num); // reset put pointer accordingly
        return num;
    }
    
    int StreamBufferGL::overflow (int c)
    {
        if (c != EOF)
        {
            *pptr() = c;    // insert character into the buffer
            pbump(1);
        }
        if (flushBuffer() == EOF)
            return EOF;
        return c;
    }
    
    int StreamBufferGL::sync()
    {
        if (flushBuffer() == EOF)
            return -1;    // ERROR
        return 0;
    }
    
}}//namespace