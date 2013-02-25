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

#include "KinskiGL.h"

namespace kinski{ namespace gl{

class KINSKI_API Buffer
{
 private:
    struct Obj;
    typedef std::shared_ptr<Obj> ObjPtr;
    ObjPtr m_Obj;
    
    void init(GLenum target = GL_ARRAY_BUFFER, GLenum usage = GL_STREAM_DRAW);
    
 public:
    
    Buffer();
    
    Buffer(GLenum target, GLenum usage);
    
    template <class T>
    Buffer(const std::vector<T> &theVec, GLenum target, GLenum usage);
    
    ~Buffer();
    
    //! Emulates shared_ptr-like behavior
    typedef ObjPtr Buffer::*unspecified_bool_type;
    operator unspecified_bool_type() const { return ( m_Obj.get() == 0 ) ? 0 : &Buffer::m_Obj; }
    void reset() { m_Obj.reset(); }
    
    // map and unmap the buffer to local memory
    char* map();
    void unmap();
    
    GLint id() const;
    GLenum target() const;
    GLenum usage() const;
    GLsizei numBytes() const;
    
    void setTarget(GLenum theTarget);
    void setUsage(GLenum theUsage);
    
    void setData(char *theData, GLsizei numBytes);
    
    template <class T>
    void setData(const std::vector<T> &theVec)
    {
        GLsizei numBytes = theVec.size() * sizeof(T);
        setData((char*)(&theVec[0]), numBytes);
    };
};
    
}}

#endif /* defined(__kinskiGL__Buffer__) */
