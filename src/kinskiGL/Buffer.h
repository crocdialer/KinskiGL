//
//  Buffer.h
//  kinskiGL
//
//  Created by Fabian on 1/2/13.
//
//

#ifndef __kinskiGL__Buffer__
#define __kinskiGL__Buffer__

#include "KinskiGL.h"

namespace kinski{ namespace gl{

class Buffer
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
