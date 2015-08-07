// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Buffer.h"

namespace kinski{ namespace gl{

struct Buffer::Obj
{
    Obj():buffer_id(0), target(0), usage(0), numBytes(0), stride(0), doNotDispose(false)
    {
        glGenBuffers(1, &buffer_id);
    };
    
    ~Obj()
    {
        if(buffer_id && (!doNotDispose))
        {
            glDeleteBuffers(1, &buffer_id);
        }
    };
    
    GLuint buffer_id;
    GLenum target;
    GLenum usage;
    GLsizei numBytes;
    GLsizei stride;
    bool doNotDispose;
};
    
Buffer::Buffer(GLenum target, GLenum usage)
{
    init(target, usage);
}
    
template <class T>
Buffer::Buffer(const std::vector<T> &theVec, GLenum target, GLenum usage)
{
    init(target, usage);
    setData(theVec);
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
    uint8_t *ptr = (uint8_t*) glMapBufferRange(m_Obj->target, 0, m_Obj->numBytes, mode);
#else
    mode = mode ? mode : GL_ENUM(GL_READ_WRITE);
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
    const uint8_t *ptr = (uint8_t*) glMapBufferRange(m_Obj->target, 0, m_Obj->numBytes, mode);
#else
    mode = mode ? mode : GL_ENUM(GL_READ_WRITE);
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

GLsizei Buffer::numBytes() const
{
    return m_Obj->numBytes;
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
    
    
void Buffer::setTarget(GLenum theTarget)
{
    if(m_Obj) m_Obj->target = theTarget;
}
    
void Buffer::setUsage(GLenum theUsage)
{
    if(m_Obj) m_Obj->usage = theUsage;
}
    
void Buffer::setStride(GLsizei theStride)
{
    m_Obj->stride = theStride;
}
    
void Buffer::setData(const void *theData, GLsizei numBytes)
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
        glBufferData(m_Obj->target, numBytes, nullptr, m_Obj->usage);
    }
    
    m_Obj->numBytes = numBytes;
    glBufferData(m_Obj->target, numBytes, theData, m_Obj->usage);
    glBindBuffer(m_Obj->target, 0);
}
    
}}//namespace