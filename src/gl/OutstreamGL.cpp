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
        StreamBufferGL(OutstreamGL *ostreamGL);
        
    protected:
        
        // flush the characters in the buffer
        int flush_buffer();
        virtual int overflow(int c = EOF) override;
        virtual int sync() override;
        
    private:
        OutstreamGL* m_outstreamGL;
        std::array<char, 1 << 16> m_buffer;
    };
    
    OutstreamGL::OutstreamGL(uint32_t max_lines):
    ios(0),
    ostream(new StreamBufferGL(this)),
    m_lines(max_lines),
    m_gui_scene(gl::Scene::create()),
    m_dirty(true)
    {
        
    }
    
    OutstreamGL::OutstreamGL(const gl::Font &the_font, uint32_t max_lines):
    ios(0),
    ostream(new StreamBufferGL(this)),
    m_font(the_font),
    m_lines(max_lines),
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
        m_lines.push_back(line);
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
            std::unique_lock<std::mutex> scoped_lock(mutex);
            m_gui_scene->clear();
            auto text_obj = m_font.create_text_object(m_lines,
                                                      gl::Font::Align::LEFT,
                                                      window_dimension().x - 2 * margin,
                                                      m_font.line_height() * 1.1f);
            
            auto obj_it = text_obj->children().begin();
            
            for(uint32_t i = 0; i < m_lines.size(); ++i)
            {
                auto &line = m_lines[i];
                gl::SelectVisitor<gl::Mesh> visitor;
                (*obj_it)->accept(visitor);
                auto col = gl::COLOR_WHITE;

                if(line.find("WARNING") != std::string::npos)
                { col = gl::COLOR_ORANGE; }
                else if(line.find("ERROR") != std::string::npos)
                { col = gl::COLOR_RED; }

//                col.a = (float) i / m_lines.size();
                for(auto m : visitor.get_objects()){ m->material()->set_diffuse(col); }
                ++obj_it;
            }
            
            auto aabb = text_obj->aabb();
            text_obj->set_position(gl::vec3(margin, aabb.height() + margin, 0.f));
            m_gui_scene->add_object(text_obj);
            
            if(use_caching)
            {
                gl::vec2 fbo_sz(window_dimension().x, aabb.height() + margin);
                fbo_sz = glm::min(fbo_sz, gl::window_dimension());
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
#if !defined(KINSKI_GLES_2)
                    m_blend_material->set_blend_factors(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);
#else
                    m_blend_material->set_blend_factors(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
#endif
                }
                m_blend_material->set_textures({m_fbo.texture()});
            }
            m_gui_scene->clear();
            m_dirty = false;
        }
        if(use_caching)
        {
            gl::draw_quad(m_fbo.size(), m_blend_material,
                          gl::vec2(0.f, gl::window_dimension().y - m_fbo.size().y));
        }
        else{ m_gui_scene->render(gl::OrthoCamera::create_for_window()); }
    }
    
    StreamBufferGL::StreamBufferGL(OutstreamGL *ostreamGL):
    m_outstreamGL(ostreamGL),
    m_buffer()
    {
        //set putbase pointer and endput pointer
        setp(m_buffer.data(), m_buffer.data() + m_buffer.size());
    }

    
    // flush the characters in the buffer
    int StreamBufferGL::flush_buffer()
    {
        int num = pptr() - pbase();
        
        // pass the flushed char sequence
        m_outstreamGL->add_line(std::string(pbase(), pptr()));

        // reset put pointer accordingly
        pbump(-num);

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
