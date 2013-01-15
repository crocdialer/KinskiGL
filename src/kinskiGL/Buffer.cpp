//
//  Buffer.cpp
//  kinskiGL
//
//  Created by Fabian on 1/2/13.
//
//

#include <vector>
#include "Buffer.h"

namespace kinski{ namespace gl{

struct Buffer::Obj
{
    Obj():buffer_id(0), target(0), usage(0), numBytes(0), doNotDispose(false)
    {
        glGenBuffers(1, &buffer_id);
    };
    
    ~Obj()
    {
        if( ( buffer_id > 0 ) && ( ! doNotDispose ) )
        {
            glDeleteBuffers(1, &buffer_id);
        }
        
    };
    
    GLuint buffer_id;
    GLenum target;
    GLenum usage;
    GLsizei numBytes;
    
    bool doNotDispose;
    
};
    
Buffer::Buffer():
m_Obj(new Obj)
{

}
    
Buffer::Buffer(GLenum target, GLenum usage):
m_Obj(new Obj)
{
    init(target, usage);
}
    
template <class T>
Buffer::Buffer(const std::vector<T> &theVec, GLenum target, GLenum usage):
m_Obj(new Obj)
{
    init(target, usage);
    setData(theVec);
}
    
Buffer::~Buffer()
{
    
}

void Buffer::init(GLenum target, GLenum usage)
{
    //m_Obj = ObjPtr(new Obj);
    m_Obj->target = target;
    m_Obj->usage = usage;
}

GLint Buffer::id() const
{
    return m_Obj->buffer_id;
}

char* Buffer::map()
{
    glBindBuffer(m_Obj->target, m_Obj->buffer_id);
    char *ptr = (char*) GL_SUFFIX(glMapBuffer)(m_Obj->target, GL_ENUM(GL_WRITE_ONLY));
    
    if(!ptr) throw Exception("Could not map gl::Buffer");
    
    glBindBuffer(m_Obj->target, 0);
    return ptr;
}

void Buffer::unmap()
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

void Buffer::setData(char *theData, GLsizei numBytes)
{
    if(!m_Obj->target) init();
    
    m_Obj->numBytes = numBytes;
    glBindBuffer(m_Obj->target, m_Obj->buffer_id);
    glBufferData(m_Obj->target, numBytes, theData, m_Obj->usage);
    glBindBuffer(m_Obj->target, 0);
}
    
}}//namespace
