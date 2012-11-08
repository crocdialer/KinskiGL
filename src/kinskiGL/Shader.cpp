/*
 Copyright (c) 2010, The Barbarian Group
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#include "Shader.h"

#include <fstream>

using namespace std;

namespace kinski
{
namespace gl
{

//////////////////////////////////////////////////////////////////////////
// Shader::Obj
struct Shader::Obj 
{
    Obj() : m_Handle( 0 ) {}
    ~Obj();
    
    GLuint						m_Handle;
    std::map<std::string,int>	m_UniformLocs;

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
	m_Obj->m_Handle = glCreateProgram();
	
	if ( vertexShader )
		loadShader( vertexShader, GL_VERTEX_SHADER );
    
	if( fragmentShader )
		loadShader( fragmentShader, GL_FRAGMENT_SHADER );
    
	if( geometryShader )
		loadShader( geometryShader, GL_GEOMETRY_SHADER );
    
	link();
}

void Shader::loadFromData(const char *vertSrc,
                          const char *fragSrc,
                          const char *geomSrc)
{
    if(!m_Obj)
    {
        m_Obj = ObjPtr(new Obj);
        m_Obj->m_Handle = glCreateProgram();
    }
    
    loadShader( vertSrc, GL_VERTEX_SHADER );
	loadShader( fragSrc, GL_FRAGMENT_SHADER );

    if(geomSrc) 
        loadShader(geomSrc, GL_GEOMETRY_SHADER);
    
    link();
}
    
void Shader::loadFromFile(const std::string &vertPath,
                          const std::string &fragPath,
                          const std::string &geomPath)
{
    string vertSrc, fragSrc, geomSrc;
    vertSrc = readFile(vertPath);
    fragSrc = readFile(fragPath);

    if (!geomPath.empty()) geomSrc = readFile(geomPath);
    
    loadFromData(vertSrc.c_str(), fragSrc.c_str(), geomSrc.empty() ? NULL : geomSrc.c_str());
}
    
const string Shader::readFile(const std::string &path)
{
    
    ifstream inStream(path.c_str());
    if(!inStream.good())
    {
        string err = "file not found: " + path;
        throw Exception(err);
    }
    
    return string ((istreambuf_iterator<char>(inStream)),
                    istreambuf_iterator<char>());
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
}

void Shader::link()
{
    glLinkProgram( m_Obj->m_Handle );
    
    GLint status;
	glGetProgramiv( m_Obj->m_Handle, GL_LINK_STATUS, &status );
	if( status != GL_TRUE )
    {
		std::string log = getProgramLog();
		throw ShaderLinkException();
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

void Shader::uniform( const std::string &name, GLint data )
{
	GLint loc = getUniformLocation( name );
	glUniform1i( loc, data );
}

void Shader::uniform( const std::string &name, const glm::vec2 &data )
{
	GLint loc = getUniformLocation( name );
	glUniform2i( loc, data.x, data.y );
}

void Shader::uniform( const std::string &name, const GLint *data, int count )
{
	GLint loc = getUniformLocation( name );
	glUniform1iv( loc, count, data );
}

void Shader::uniform( const std::string &name, const glm::ivec2 *data, int count )
{
	GLint loc = getUniformLocation( name );
	glUniform2iv( loc, count, &data[0].x );
}

void Shader::uniform( const std::string &name, GLfloat data )
{
	GLint loc = getUniformLocation( name );
	glUniform1f( loc, data );
}

void Shader::uniform( const std::string &name, const glm::vec3 &data )
{
	GLint loc = getUniformLocation( name );
	glUniform3f( loc, data.x, data.y, data.z );
}

void Shader::uniform( const std::string &name, const glm::vec4 &data )
{
	GLint loc = getUniformLocation( name );
	glUniform4f( loc, data.x, data.y, data.z, data.w );
}

void Shader::uniform( const std::string &name, const GLfloat *data, int count )
{
	GLint loc = getUniformLocation( name );
	glUniform1fv( loc, count, data );
}

    void Shader::uniform( const std::string &name, const glm::vec2 *theArray, int count )
{
	GLint loc = getUniformLocation( name );
	glUniform2fv( loc, count, &theArray[0].x );
}

void Shader::uniform( const std::string &name, const glm::vec3 *theArray, int count )
{
	GLint loc = getUniformLocation( name );
	glUniform3fv( loc, count, &theArray[0].x );
}

void Shader::uniform( const std::string &name, const glm::vec4 *theArray, int count )
{
	GLint loc = getUniformLocation( name );
	glUniform4fv( loc, count, &theArray[0].x );
}

void Shader::uniform( const std::string &name, const glm::mat3 &theMat, bool transpose )
{
	GLint loc = getUniformLocation( name );
	glUniformMatrix3fv( loc, 1, ( transpose ) ? GL_TRUE : GL_FALSE, glm::value_ptr(theMat) );
}

void Shader::uniform( const std::string &name, const glm::mat4 &theMat, bool transpose )
{
	GLint loc = getUniformLocation( name );
	glUniformMatrix4fv( loc, 1, ( transpose ) ? GL_TRUE : GL_FALSE, glm::value_ptr(theMat) );
}

void Shader::uniform( const std::string &name, const glm::mat3 *theArray, int count, bool transpose )
{
    GLint loc = getUniformLocation( name );
    glUniformMatrix3fv( loc, count, ( transpose ) ? GL_TRUE : GL_FALSE, glm::value_ptr(theArray[0]) );
}

void Shader::uniform( const std::string &name, const glm::mat4 *theArray, int count, bool transpose )
{
    GLint loc = getUniformLocation( name );
    glUniformMatrix4fv( loc, count, ( transpose ) ? GL_TRUE : GL_FALSE, glm::value_ptr(theArray[0]) );
}
    
void Shader::uniform( const std::string &name, const std::vector<GLint> &theArray )
{
    GLint loc = getUniformLocation( name );
    glUniform1iv( loc, theArray.size(), &theArray[0] );
}
    
void Shader::uniform( const std::string &name, const std::vector<GLfloat> &theArray )
{
    GLint loc = getUniformLocation( name );
	glUniform1fv( loc, theArray.size(), &theArray[0] );
}

void Shader::uniform( const std::string &name, const std::vector<glm::vec2> &theArray )
{
    GLint loc = getUniformLocation( name );
	glUniform2fv( loc, theArray.size(), &theArray[0].x );
}

void Shader::uniform( const std::string &name, const std::vector<glm::vec3> &theArray )
{
    GLint loc = getUniformLocation( name );
	glUniform3fv( loc, theArray.size(), &theArray[0].x );
}

void Shader::uniform( const std::string &name, const std::vector<glm::vec4> &theArray )
{
    GLint loc = getUniformLocation( name );
	glUniform4fv( loc, theArray.size(), &theArray[0].x );
}

void Shader::uniform( const std::string &name, const std::vector<glm::mat3> &theArray, bool transpose )
{
    GLint loc = getUniformLocation( name );
    glUniformMatrix3fv( loc, theArray.size(), ( transpose ) ? GL_TRUE : GL_FALSE, glm::value_ptr(theArray[0]) );
}

void Shader::uniform( const std::string &name, const std::vector<glm::mat4> &theArray, bool transpose )
{
    GLint loc = getUniformLocation( name );
    glUniformMatrix4fv( loc, theArray.size(), ( transpose ) ? GL_TRUE : GL_FALSE, glm::value_ptr(theArray[0]) );
}

void Shader::bindFragDataLocation(const std::string &fragLoc)
{
    glBindFragDataLocation(m_Obj->m_Handle, 0, fragLoc.c_str());
}
    
GLint Shader::getUniformLocation( const std::string &name )
{
	map<string,int>::const_iterator uniformIt = m_Obj->m_UniformLocs.find( name );
	if( uniformIt == m_Obj->m_UniformLocs.end() ) {
		GLint loc = glGetUniformLocation( m_Obj->m_Handle, name.c_str() );
		m_Obj->m_UniformLocs[name] = loc;
		return loc;
	}
	else
		return uniformIt->second;
}

GLint Shader::getAttribLocation( const std::string &name )
{
	return glGetAttribLocation( m_Obj->m_Handle, name.c_str() );
}

//////////////////////////////////////////////////////////////////////////
// ShaderCompileExc
ShaderCompileExc::ShaderCompileExc( const std::string &log, GLint aShaderType ):
    Exception(""),
    m_ShaderType( aShaderType )
{
	if( m_ShaderType == GL_VERTEX_SHADER )
		strncpy( m_Message, "VERTEX: ", 1000 );
	else if( m_ShaderType == GL_FRAGMENT_SHADER )
		strncpy( m_Message, "FRAGMENT: ", 1000 );
	else if( m_ShaderType == GL_GEOMETRY_SHADER )
		strncpy( m_Message, "GEOMETRY: ", 1000 );
	else
		strncpy( m_Message, "UNKNOWN: ", 1000 );
	strncat( m_Message, log.c_str(), 15000 );
}
	
}}
