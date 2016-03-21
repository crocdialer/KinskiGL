// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Buffer.hpp"

namespace kinski{ namespace gl{

struct Buffer::Obj
{
    Obj():buffer_id(0), target(0), usage(0), num_bytes(0), stride(0), do_not_dispose(false)
    {
        glGenBuffers(1, &buffer_id);
    };
    
    ~Obj()
    {
        if(buffer_id && (!do_not_dispose))
        {
            glDeleteBuffers(1, &buffer_id);
        }
    };
    
    GLuint buffer_id;
    GLenum target;
    GLenum usage;
    GLsizei num_bytes;
    GLsizei stride;
    bool do_not_dispose;
};
    
Buffer::Buffer(GLenum target, GLenum usage)
{
    init(target, usage);
}
    
Buffer::~Buffer(){}

void Buffer::init(GLenum target, GLenum usage)
{
    m_Obj = ObjPtr(new Obj);
    m_Obj->target = target;
    m_Obj->usage = usage;
}

GLint Buffer::id() const
{
    return m_Obj->buffer_id;
}

uint8_t* Buffer::map(GLenum mode)
{
    glBindBuffer(m_Obj->target, m_Obj->buffer_id);

#if defined(KINSKI_GLES_3)
    mode = mode ? mode : GL_MAP_WRITE_BIT;
    uint8_t *ptr = (uint8_t*) glMapBufferRange(m_Obj->target, 0, m_Obj->num_bytes, mode);
#else
    mode = mode ? mode : GL_ENUM(GL_WRITE_ONLY);
    uint8_t *ptr = (uint8_t*) GL_SUFFIX(glMapBuffer)(m_Obj->target, mode);
#endif
    
    if(!ptr) throw Exception("Could not map gl::Buffer");
    
    glBindBuffer(m_Obj->target, 0);
    return ptr;
}
    
const uint8_t* Buffer::map(GLenum mode) const
{
    glBindBuffer(m_Obj->target, m_Obj->buffer_id);

#if defined(KINSKI_GLES_3)
    mode = mode ? mode : GL_MAP_WRITE_BIT;
    const uint8_t *ptr = (uint8_t*) glMapBufferRange(m_Obj->target, 0, m_Obj->num_bytes, mode);
#else
    mode = mode ? mode : GL_ENUM(GL_WRITE_ONLY);
    const uint8_t *ptr = (uint8_t*) GL_SUFFIX(glMapBuffer)(m_Obj->target, mode);
#endif
    
    if(!ptr) throw Exception("Could not map gl::Buffer");
    
    glBindBuffer(m_Obj->target, 0);
    return ptr;
}

void Buffer::unmap() const
{
    glBindBuffer(m_Obj->target, m_Obj->buffer_id);
    GL_SUFFIX(glUnmapBuffer)(m_Obj->target);
    glBindBuffer(m_Obj->target, 0);
}

GLenum Buffer::target() const
{
    return m_Obj->target;
}

GLenum Buffer::usage() const
{
    return m_Obj->usage;
}

GLsizei Buffer::num_bytes() const
{
    return m_Obj->num_bytes;
}

GLsizei Buffer::stride() const
{
    return m_Obj->stride;
}

void Buffer::bind(GLenum the_target) const
{
    glBindBuffer(the_target ? the_target : m_Obj->target, m_Obj->buffer_id);
}

void Buffer::unbind(GLenum the_target) const
{
    glBindBuffer(the_target ? the_target : m_Obj->target, 0);
}
    
    
void Buffer::set_target(GLenum theTarget)
{
    if(m_Obj) m_Obj->target = theTarget;
}
    
void Buffer::set_usage(GLenum theUsage)
{
    if(m_Obj) m_Obj->usage = theUsage;
}
    
void Buffer::set_stride(GLsizei theStride)
{
    m_Obj->stride = theStride;
}
    
void Buffer::set_data(const void *the_data, GLsizei num_bytes)
{
    if(!m_Obj)
    {
        init();
        glBindBuffer(m_Obj->target, m_Obj->buffer_id);
    }
    else
    {
        //orphan buffer
        glBindBuffer(m_Obj->target, m_Obj->buffer_id);
        glBufferData(m_Obj->target, num_bytes, nullptr, m_Obj->usage);
    }
    
    m_Obj->num_bytes = num_bytes;
    glBufferData(m_Obj->target, num_bytes, the_data, m_Obj->usage);
    glBindBuffer(m_Obj->target, 0);
}
    
}}//namespace