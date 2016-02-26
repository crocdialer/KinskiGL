// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
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
    
    template <typename T>
    Buffer(const std::vector<T> &theVec, GLenum target, GLenum usage);
    
    ~Buffer();
    
    //! Emulates shared_ptr-like behavior
    operator bool() const { return m_Obj.get() != nullptr; }
    void reset() { m_Obj.reset(); }
    
    // map and unmap the buffer to local memory
    uint8_t* map(GLenum access = 0);
    const uint8_t* map(GLenum access = 0) const;
    void unmap() const;
    
    void bind(GLenum the_target = 0) const;
    void unbind(GLenum the_target = 0) const;
    
    GLint id() const;
    GLenum target() const;
    GLenum usage() const;
    GLsizei numBytes() const;
    GLsizei stride() const;
    
    void setTarget(GLenum theTarget);
    void setUsage(GLenum theUsage);
    void setStride(GLsizei theStride);
    void setData(const void *theData, GLsizei numBytes);
    
    template <typename T>
    inline void setData(const std::vector<T> &theVec)
    {
        GLsizei numBytes = theVec.size() * sizeof(T);
        setData((void*)&theVec[0], numBytes);
    };
    
private:
    struct Obj;
    typedef std::shared_ptr<Obj> ObjPtr;
    ObjPtr m_Obj;
    
    void init(GLenum target = GL_ARRAY_BUFFER, GLenum usage = GL_STATIC_DRAW);
};
    
}}

#endif /* defined(__gl__Buffer__) */
