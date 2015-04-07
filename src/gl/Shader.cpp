// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "core/file_functions.h"
#include "Shader.h"

using namespace std;

namespace kinski{ namespace gl{
    
//////////////////////////////////////////////////////////////////////////
// Shader::Obj
struct Shader::Obj
{
    Obj() : m_Handle(glCreateProgram()) {}
    ~Obj();
    
    GLuint						m_Handle;
    std::map<std::string,int>	m_UniformLocs;
    std::map<std::string,int>	m_UniformBlockIndices;

};
    
Shader::Obj::~Obj()
{
	if( m_Handle )
		glDeleteProgram( (GLuint)m_Handle );
}

//////////////////////////////////////////////////////////////////////////
// Shader

Shader::Shader(){}
    
Shader::Shader(const char *vertexShader, const char *fragmentShader,
               const char *geometryShader, GLint geometryInputType,
               GLint geometryOutputType, GLint geometryOutputVertices)
: m_Obj( ObjPtr( new Obj ) )
{
	if ( vertexShader )
		loadShader( vertexShader, GL_VERTEX_SHADER );
    
	if( fragmentShader )
		loadShader( fragmentShader, GL_FRAGMENT_SHADER );
    
#ifndef KINSKI_GLES
	if( geometryShader )
		loadShader( geometryShader, GL_GEOMETRY_SHADER );
#endif
    
	link();
}

void Shader::loadFromData(const char *vertSrc,
                          const char *fragSrc,
                          const char *geomSrc)
{
    m_Obj = ObjPtr(new Obj);
    
    loadShader( vertSrc, GL_VERTEX_SHADER );
	loadShader( fragSrc, GL_FRAGMENT_SHADER );

#ifndef KINSKI_GLES
    if(geomSrc) 
        loadShader(geomSrc, GL_GEOMETRY_SHADER);
#endif
    
    link();
}
        
void Shader::loadShader( const char *shaderSource, GLint shaderType )
{
	GLuint handle = glCreateShader( shaderType );
	glShaderSource( handle, 1, reinterpret_cast<const GLchar**>( &shaderSource ), NULL );
	glCompileShader( handle );
	
	GLint status;
	glGetShaderiv( (GLuint) handle, GL_COMPILE_STATUS, &status );
	if( status != GL_TRUE )
    {
		std::string log = getShaderLog( (GLuint)handle );
		throw ShaderCompileExc( log, shaderType );
	}
	glAttachShader( m_Obj->m_Handle, handle );
    glDeleteShader(handle);
}

void Shader::link()
{
    glLinkProgram( m_Obj->m_Handle );
    
    GLint status;
	glGetProgramiv( m_Obj->m_Handle, GL_LINK_STATUS, &status );
	if( status != GL_TRUE )
    {
		std::string log = getProgramLog();
		throw ShaderLinkException(log);
	}
    std::string log = getProgramLog();
}

void Shader::bind() const
{
    if(!m_Obj) throw ShaderNullProgramExc();
    
	glUseProgram( m_Obj->m_Handle );
}

void Shader::unbind()
{
	glUseProgram( 0 );
}

GLuint Shader::getHandle() const
{
    return m_Obj->m_Handle;
}
    
std::string Shader::getShaderLog( GLuint handle ) const
{
	std::string log;
	
	GLchar *debugLog;
	GLint debugLength = 0, charsWritten = 0;
	glGetShaderiv( handle, GL_INFO_LOG_LENGTH, &debugLength );

	if( debugLength > 0 ) {
		debugLog = new GLchar[debugLength];
		glGetShaderInfoLog( handle, debugLength, &charsWritten, debugLog );
		log.append( debugLog, 0, debugLength );
		delete [] debugLog;
	}
	
	return log;
}
    
std::string Shader::getProgramLog() const
{
    std::string log;
    
    GLchar *debugLog;
    GLint debugLength = 0, charsWritten = 0;
    glGetProgramiv( m_Obj->m_Handle, GL_INFO_LOG_LENGTH, &debugLength );
    
    if( debugLength > 0 ) {
        debugLog = new GLchar[debugLength];
        glGetProgramInfoLog( m_Obj->m_Handle, debugLength, &charsWritten, debugLog );
        log.append( debugLog, 0, debugLength );
        delete [] debugLog;
    }
    
    return log;
}

void Shader::uniform(const std::string &name, GLint data)
{
	GLint loc = getUniformLocation( name );
	if(loc != -1) glUniform1i(loc, data);
}

void Shader::uniform(const std::string &name, const glm::vec2 &data)
{
	GLint loc = getUniformLocation( name );
	if(loc != -1) glUniform2f( loc, data.x, data.y );
}

void Shader::uniform(const std::string &name, const GLint *data, int count)
{
	GLint loc = getUniformLocation( name );
	if(loc != -1) glUniform1iv( loc, count, data );
}

void Shader::uniform(const std::string &name, const glm::ivec2 *data, int count)
{
	GLint loc = getUniformLocation(name);
	if(loc != -1) glUniform2iv(loc, count, &data[0].x);
}

void Shader::uniform(const std::string &name, GLfloat data)
{
	GLint loc = getUniformLocation(name);
	if(loc != -1) glUniform1f(loc, data);
}

void Shader::uniform(const std::string &name, const glm::vec3 &data)
{
	GLint loc = getUniformLocation(name);
	if(loc != -1) glUniform3f( loc, data.x, data.y, data.z );
}

void Shader::uniform(const std::string &name, const glm::vec4 &data)
{
	GLint loc = getUniformLocation(name);
	if(loc != -1) glUniform4f( loc, data.x, data.y, data.z, data.w );
}

void Shader::uniform(const std::string &name, const GLfloat *data, int count)
{
	GLint loc = getUniformLocation(name);
	if(loc != -1) glUniform1fv(loc, count, data);
}

    void Shader::uniform(const std::string &name, const glm::vec2 *theArray, int count)
{
	GLint loc = getUniformLocation(name);
	if(loc != -1) glUniform2fv(loc, count, &theArray[0].x);
}

void Shader::uniform(const std::string &name, const glm::vec3 *theArray, int count)
{
	GLint loc = getUniformLocation(name);
	if(loc != -1) glUniform3fv(loc, count, &theArray[0].x);
}

void Shader::uniform(const std::string &name, const glm::vec4 *theArray, int count)
{
	GLint loc = getUniformLocation(name);
	if(loc != -1) glUniform4fv(loc, count, &theArray[0].x);
}

void Shader::uniform(const std::string &name, const glm::mat3 &theMat, bool transpose)
{
	GLint loc = getUniformLocation(name);
	if(loc != -1) glUniformMatrix3fv(loc, 1, ( transpose ) ? GL_TRUE : GL_FALSE,
                                     glm::value_ptr(theMat));
}

void Shader::uniform(const std::string &name, const glm::mat4 &theMat, bool transpose)
{
	GLint loc = getUniformLocation(name);
	if(loc != -1) glUniformMatrix4fv(loc, 1, ( transpose ) ? GL_TRUE : GL_FALSE,
                                     glm::value_ptr(theMat));
}

void Shader::uniform(const std::string &name, const glm::mat3 *theArray, int count, bool transpose)
{
    GLint loc = getUniformLocation(name);
    if(loc != -1) glUniformMatrix3fv(loc, count, ( transpose ) ? GL_TRUE : GL_FALSE,
                                     glm::value_ptr(theArray[0]));
}

void Shader::uniform(const std::string &name, const glm::mat4 *theArray, int count, bool transpose)
{
    GLint loc = getUniformLocation(name);
    if(loc != -1) glUniformMatrix4fv(loc, count, ( transpose ) ? GL_TRUE : GL_FALSE,
                                     glm::value_ptr(theArray[0]));
}
    
void Shader::uniform(const std::string &name, const std::vector<GLint> &theArray)
{
    GLint loc = getUniformLocation(name);
    if(loc != -1) glUniform1iv(loc, theArray.size(), &theArray[0]);
}
    
void Shader::uniform(const std::string &name, const std::vector<GLfloat> &theArray)
{
    GLint loc = getUniformLocation(name);
	if(loc != -1) glUniform1fv(loc, theArray.size(), &theArray[0]);
}

void Shader::uniform(const std::string &name, const std::vector<glm::vec2> &theArray)
{
    GLint loc = getUniformLocation( name );
	if(loc != -1) glUniform2fv( loc, theArray.size(), &theArray[0].x );
}

void Shader::uniform(const std::string &name, const std::vector<glm::vec3> &theArray)
{
    GLint loc = getUniformLocation( name );
	if(loc != -1) glUniform3fv( loc, theArray.size(), &theArray[0].x );
}

void Shader::uniform(const std::string &name, const std::vector<glm::vec4> &theArray)
{
    GLint loc = getUniformLocation( name );
	if(loc != -1) glUniform4fv( loc, theArray.size(), &theArray[0].x );
}

void Shader::uniform(const std::string &name, const std::vector<glm::mat3> &theArray, bool transpose)
{
    GLint loc = getUniformLocation( name );
    if(loc != -1) glUniformMatrix3fv( loc, theArray.size(), ( transpose ) ? GL_TRUE : GL_FALSE,
                                     glm::value_ptr(theArray[0]) );
}

void Shader::uniform(const std::string &name, const std::vector<glm::mat4> &theArray, bool transpose)
{
    GLint loc = getUniformLocation( name );
    if(loc != -1) glUniformMatrix4fv(loc, theArray.size(), ( transpose ) ? GL_TRUE : GL_FALSE,
                                     glm::value_ptr(theArray[0]));
}

void Shader::bindFragDataLocation(const std::string &fragLoc)
{
#ifndef KINSKI_GLES
    glBindFragDataLocation(m_Obj->m_Handle, 0, fragLoc.c_str());
#endif
}
    
GLint Shader::getUniformLocation(const std::string &name)
{
	map<string,int>::const_iterator uniformIt = m_Obj->m_UniformLocs.find( name );
	if( uniformIt == m_Obj->m_UniformLocs.end() )
    {
		GLint loc = glGetUniformLocation( m_Obj->m_Handle, name.c_str() );
        m_Obj->m_UniformLocs[name] = loc;
		return loc;
	}
	else
		return uniformIt->second;
}
    
GLint Shader::getUniformBlockIndex(const std::string &name)
{
#ifndef KINSKI_GLES
    
    auto it = m_Obj->m_UniformBlockIndices.find(name);
    if(it == m_Obj->m_UniformBlockIndices.end())
    {
        GLint loc = glGetUniformBlockIndex(m_Obj->m_Handle, name.c_str());
        m_Obj->m_UniformBlockIndices[name] = loc;
        return loc;
    }
    else
        return it->second;
#endif
    return GL_INVALID_INDEX;
}
    
GLint Shader::getAttribLocation(const std::string &name) const
{
	return glGetAttribLocation( m_Obj->m_Handle, name.c_str() );
}

//////////////////////////////////////////////////////////////////////////
// ShaderCompileExc
ShaderCompileExc::ShaderCompileExc(const std::string &log, GLint aShaderType):
    Exception(""),
    m_ShaderType( aShaderType )
{
	if( m_ShaderType == GL_VERTEX_SHADER )
		strncpy( m_Message, "VERTEX: ", 1000 );
	else if( m_ShaderType == GL_FRAGMENT_SHADER )
		strncpy( m_Message, "FRAGMENT: ", 1000 );
#ifndef KINSKI_GLES
	else if( m_ShaderType == GL_GEOMETRY_SHADER )
		strncpy( m_Message, "GEOMETRY: ", 1000 );
#endif
	else
		strncpy( m_Message, "UNKNOWN: ", 1000 );
	strncat( m_Message, log.c_str(), 15000 );
}
	
}}
