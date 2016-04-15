// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  OutstreamGL.cpp
//
//  Created by Fabian on 5/13/13.

#include "OutstreamGL.hpp"
#include "gl/Material.hpp"
#include <mutex>

using namespace std;

namespace kinski{ namespace gl{
    
    namespace{ std::mutex mutex; }
    
    // This is the streambuffer; its function is to store formatted data and send
    // it to a character output when solicited (sync/overflow methods) . You do not
    // instantiate it by yourself on your application; it will be automatically used
    // by an actual output stream (like the OutstreamGL class defined above)
    class StreamBufferGL : public std::streambuf
    {
    public:
        StreamBufferGL(OutstreamGL *ostreamGL, size_t buff_sz = 1 << 20);
        
    protected:
        
        // flush the characters in the buffer
        int flushBuffer ();
        virtual int overflow ( int c = EOF );
        virtual int sync();
        
    private:
        OutstreamGL* m_outstreamGL;
        std::vector<char> m_buffer;
    };
    
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
    
    void OutstreamGL::add_line(const std::string &line)
    {
        std::unique_lock<std::mutex> scoped_lock(mutex);
        
        while(m_lines.size() >= m_max_lines) m_lines.pop_back();
        m_lines.push_front(line);
    }
    
    void OutstreamGL::draw()
    {
        if(!m_font) return;
        
        glm::vec2 step(0, m_font.line_height() * 1.1f);
        glm::vec2 offset(10, window_dimension().y - step.y);
        
        std::unique_lock<std::mutex> scoped_lock(mutex);
        
        int i = m_lines.size();
        for (const string &line : m_lines)
        {
            gl::Color color = m_color;
            color.a = glm::smoothstep(0.f, 1.f, (float) i / m_lines.size());
            if(line.find("WARNING") != std::string::npos){color = gl::COLOR_ORANGE;}
            else if(line.find("ERROR") != std::string::npos){color = gl::COLOR_RED;}

            gl::draw_text_2D(line.substr(0, line.size() - 1), m_font, color, offset);
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
        m_outstreamGL->add_line(std::string(pbase(), pptr()));
        
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