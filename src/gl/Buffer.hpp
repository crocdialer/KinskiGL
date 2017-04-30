// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef KINSKI_GL_BUFFER_H_
#define KINSKI_GL_BUFFER_H_

#include "gl/gl.hpp"

namespace kinski{ namespace gl{

class KINSKI_API Buffer
{
 public:
    
    Buffer(){};
    Buffer(GLenum target, GLenum usage);
    
    template <class T> Buffer(const std::vector<T> &the_vec, GLenum target, GLenum usage)
    {
        init(target, usage);
        set_data(the_vec);
    }
    
    ~Buffer();
    
    //! Emulates shared_ptr-like behavior
    operator bool() const { return m_impl.get(); }
    void reset() { m_impl.reset(); }
    
    // map and unmap the buffer to local memory
    uint8_t* map(GLenum access = 0);
    const uint8_t* map(GLenum access = 0) const;
    void unmap() const;
    
    void bind(GLenum the_target = 0) const;
    void unbind(GLenum the_target = 0) const;

    uint32_t id() const;
    GLenum target() const;
    GLenum usage() const;
    size_t num_bytes() const;
    size_t stride() const;
    
    void set_target(GLenum theTarget);
    void set_usage(GLenum theUsage);
    void set_stride(size_t theStride);
    void set_data(const void *theData, size_t num_bytes);
    
    template <typename T>
    inline void set_data(const std::vector<T> &the_vec)
    {
        GLsizei num_bytes = the_vec.size() * sizeof(T);
        set_data((void*)&the_vec[0], num_bytes);
    };
    
private:
    std::shared_ptr<struct BufferImpl> m_impl;
    
    void init(GLenum target = GL_ARRAY_BUFFER, GLenum usage = GL_STATIC_DRAW);
};
    
}}

#endif /* defined(__gl__Buffer__) */
