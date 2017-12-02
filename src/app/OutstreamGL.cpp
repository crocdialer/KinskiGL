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
#include "gl/Mesh.hpp"
#include "gl/Scene.hpp"
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
        int flush_buffer();
        virtual int overflow(int c = EOF) override;
        virtual int sync() override;
        
    private:
        OutstreamGL* m_outstreamGL;
        std::vector<char> m_buffer;
    };
    
    OutstreamGL::OutstreamGL(uint32_t max_lines):
    ios(0),
    ostream(new StreamBufferGL(this)),
    m_max_lines(max_lines),
    m_gui_scene(gl::Scene::create()),
    m_dirty(true)
    {
        
    }
    
    OutstreamGL::OutstreamGL(const gl::Font &the_font, uint32_t max_lines):
    ios(0),
    ostream(new StreamBufferGL(this)),
    m_font(the_font),
    m_max_lines(max_lines),
    m_gui_scene(gl::Scene::create()),
    m_dirty(true)
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
        m_dirty = true;
    }
    
    void OutstreamGL::draw()
    {
        if(!m_font) return;

        constexpr uint32_t margin = 10;
        bool use_caching = true;
        
        if(use_caching && (!m_fbo || m_fbo.size().x != window_dimension().x)){ m_dirty = true; }
    
        if(m_dirty)
        {
            std::stringstream stream;
            {
                std::unique_lock<std::mutex> scoped_lock(mutex);
                for(auto it = m_lines.crbegin(); it != m_lines.crend(); ++it){ stream << *it; }
            }
            m_gui_scene->clear();
            auto text_obj = m_font.create_text_obj(stream.str(), window_dimension().x - 2 * margin,
                                                   m_font.line_height() * 1.1f);
            auto aabb = text_obj->aabb();
            text_obj->set_position(gl::vec3(margin, aabb.height() + margin, 0.f));
            m_gui_scene->add_object(text_obj);
            
            if(use_caching)
            {
                gl::vec2 fbo_sz(window_dimension().x, aabb.height() + margin);
                if(!m_fbo || m_fbo.size() != fbo_sz){ m_fbo = gl::Fbo(fbo_sz); }
                gl::render_to_texture(m_fbo, [this]()
                {
                    gl::clear(gl::vec4(0));
                    m_gui_scene->render(gl::OrthoCamera::create_for_window());
                });
                
                if(!m_blend_material)
                {
                    m_blend_material = gl::Material::create();
                    m_blend_material->set_depth_test(false);
                    m_blend_material->set_depth_write(false);
                    m_blend_material->set_blending(true);
                    m_blend_material->set_blend_factors(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                }
                m_blend_material->set_textures({m_fbo.texture()});
            }
            m_gui_scene->clear();
            m_dirty = false;
        }
        if(use_caching)
        {
            gl::draw_quad(m_blend_material, m_fbo.size(),
                          gl::vec2(0.f, gl::window_dimension().y - m_fbo.size().y));
        }
        else{ m_gui_scene->render(gl::OrthoCamera::create_for_window()); }
        
//        std::unique_lock<std::mutex> scoped_lock(mutex);
//        glm::vec2 step(0, m_font.line_height() * 1.1f);
//        glm::vec2 offset(10, window_dimension().y - step.y);
//        int i = m_lines.size();
//        for (const string &line : m_lines)
//        {
//            gl::Color color = m_color;
//            color.a = glm::smoothstep(0.f, 1.f, (float) i / m_lines.size());
//            if(line.find("WARNING") != std::string::npos){color = gl::COLOR_ORANGE;}
//            else if(line.find("ERROR") != std::string::npos){color = gl::COLOR_RED;}
//
//            gl::draw_text_2D(line.substr(0, line.size() - 1), m_font, color, offset);
//            offset -= step;
//            i--;
//        }
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
    int StreamBufferGL::flush_buffer()
    {
        int num = pptr() - pbase();
        
        // pass the flushed char sequence
        m_outstreamGL->add_line(std::string(pbase(), pptr()));
        
        pbump(-num); // reset put pointer accordingly
        return num;
    }
    
    int StreamBufferGL::overflow (int c)
    {
        if(c != EOF)
        {
            *pptr() = c;    // insert character into the buffer
            pbump(1);
        }
        if(flush_buffer() == EOF){ return EOF; }
        return c;
    }
    
    int StreamBufferGL::sync()
    {
        if(flush_buffer() == EOF){ return -1; }
        return 0;
    }
    
}}//namespace
