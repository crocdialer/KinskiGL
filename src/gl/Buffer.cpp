// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Buffer.hpp"

namespace kinski{ namespace gl{

struct BufferImpl
{
    BufferImpl():buffer_id(0), target(0), usage(0), num_bytes(0), stride(0), do_not_dispose(false)
    {
        glGenBuffers(1, &buffer_id);
    };
    
    ~BufferImpl()
    {
        if(buffer_id && (!do_not_dispose))
        {
            glDeleteBuffers(1, &buffer_id);
        }
    };

    uint32_t buffer_id;
    GLenum target;
    GLenum usage;
    size_t num_bytes;
    size_t stride;
    bool do_not_dispose;
};
    
Buffer::Buffer(GLenum target, GLenum usage)
{
    init(target, usage);
}
    
Buffer::~Buffer(){}

void Buffer::init(GLenum target, GLenum usage)
{
    m_impl = std::make_shared<BufferImpl>();
    m_impl->target = target;
    m_impl->usage = usage;
}

uint32_t Buffer::id() const
{
    return m_impl->buffer_id;
}

uint8_t* Buffer::map(GLenum mode)
{
    glBindBuffer(m_impl->target, m_impl->buffer_id);

#if defined(KINSKI_GLES_3)
    mode = mode ? mode : GL_MAP_WRITE_BIT;
    uint8_t *ptr = (uint8_t*) glMapBufferRange(m_impl->target, 0, m_impl->num_bytes, mode);
#else
    mode = mode ? mode : GL_ENUM(GL_WRITE_ONLY);
    uint8_t *ptr = (uint8_t*) GL_SUFFIX(glMapBuffer)(m_impl->target, mode);
#endif
    
    if(!ptr) throw Exception("Could not map gl::Buffer");
    
    glBindBuffer(m_impl->target, 0);
    return ptr;
}
    
const uint8_t* Buffer::map(GLenum mode) const
{
    glBindBuffer(m_impl->target, m_impl->buffer_id);

#if defined(KINSKI_GLES_3)
    mode = mode ? mode : GL_MAP_WRITE_BIT;
    const uint8_t *ptr = (uint8_t*) glMapBufferRange(m_impl->target, 0, m_impl->num_bytes, mode);
#else
    mode = mode ? mode : GL_ENUM(GL_WRITE_ONLY);
    const uint8_t *ptr = (uint8_t*) GL_SUFFIX(glMapBuffer)(m_impl->target, mode);
#endif
    
    if(!ptr) throw Exception("Could not map gl::Buffer");
    
    glBindBuffer(m_impl->target, 0);
    return ptr;
}

void Buffer::unmap() const
{
    glBindBuffer(m_impl->target, m_impl->buffer_id);
    GL_SUFFIX(glUnmapBuffer)(m_impl->target);
    glBindBuffer(m_impl->target, 0);
}

GLenum Buffer::target() const
{
    return m_impl->target;
}

GLenum Buffer::usage() const
{
    return m_impl->usage;
}

size_t Buffer::num_bytes() const
{
    return m_impl->num_bytes;
}

size_t Buffer::stride() const
{
    return m_impl->stride;
}

void Buffer::bind(GLenum the_target) const
{
    glBindBuffer(the_target ? the_target : m_impl->target, m_impl->buffer_id);
}

void Buffer::unbind(GLenum the_target) const
{
    glBindBuffer(the_target ? the_target : m_impl->target, 0);
}
    
    
void Buffer::set_target(GLenum theTarget)
{
    if(m_impl) m_impl->target = theTarget;
}
    
void Buffer::set_usage(GLenum theUsage)
{
    if(m_impl) m_impl->usage = theUsage;
}
    
void Buffer::set_stride(size_t theStride)
{
    m_impl->stride = theStride;
}
    
void Buffer::set_data(const void *the_data, size_t num_bytes)
{
    if(!m_impl)
    {
        init();
        glBindBuffer(m_impl->target, m_impl->buffer_id);
    }
    else
    {
        //orphan buffer
        glBindBuffer(m_impl->target, m_impl->buffer_id);
        glBufferData(m_impl->target, num_bytes, nullptr, m_impl->usage);
    }
    
    m_impl->num_bytes = num_bytes;
    glBufferData(m_impl->target, num_bytes, the_data, m_impl->usage);
    glBindBuffer(m_impl->target, 0);
}
    
}}//namespace
