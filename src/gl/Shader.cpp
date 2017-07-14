// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Shader.hpp"

using namespace std;

namespace kinski{ namespace gl{
    
//////////////////////////////////////////////////////////////////////////
// Shader::Obj
struct ShaderImpl
{
    ShaderImpl() : m_Handle(glCreateProgram()) {}
    
    ~ShaderImpl()
    {
        if(m_Handle){ glDeleteProgram(m_Handle);}
    }
    
    GLuint						m_Handle;
    std::map<std::string,int>	m_uniform_locations;
    std::map<std::string, GLuint>	m_UniformBlockIndices;

};

//////////////////////////////////////////////////////////////////////////
// Shader
ShaderPtr Shader::create(const std::string &vertexShader, const std::string &fragmentShader,
						 const std::string &geometryShader)
{
	return ShaderPtr(new Shader(vertexShader, fragmentShader, geometryShader));
}
    
Shader::Shader(const std::string &vertexShader, const std::string &fragmentShader, const std::string &geometryShader):
m_impl(new ShaderImpl)
{
    if(!vertexShader.empty()){ load_shader(vertexShader.c_str(), GL_VERTEX_SHADER); }
    
    if(!fragmentShader.empty()){ load_shader(fragmentShader.c_str(), GL_FRAGMENT_SHADER); }
    
#if !defined(KINSKI_GLES)
    if(!geometryShader.empty()){ load_shader(geometryShader.c_str(), GL_GEOMETRY_SHADER); }
#endif
    
	link();
}

    void Shader::load_from_data(const std::string &vertSrc, const std::string &fragSrc,
								const std::string &geomSrc)
{
    m_impl.reset(new ShaderImpl);

	load_shader(vertSrc.c_str(), GL_VERTEX_SHADER);
	load_shader(fragSrc.c_str(), GL_FRAGMENT_SHADER);

#if !defined(KINSKI_GLES)
    if(!geomSrc.empty()){ load_shader(geomSrc.c_str(), GL_GEOMETRY_SHADER); }
#endif
    
    link();
}
        
void Shader::load_shader(const char *shaderSource, GLint shaderType)
{
	GLuint handle = glCreateShader(shaderType);
	glShaderSource(handle, 1, reinterpret_cast<const GLchar**>(&shaderSource), nullptr);
	glCompileShader(handle);
	
	GLint status;
	glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
	if(status != GL_TRUE)
    {
		std::string log = get_shader_log(handle);
		throw ShaderCompileExc(log, shaderType);
	}
	glAttachShader(m_impl->m_Handle, handle);
    glDeleteShader(handle);
}

void Shader::link()
{
    glLinkProgram(m_impl->m_Handle);
    
    GLint status;
	glGetProgramiv(m_impl->m_Handle, GL_LINK_STATUS, &status);
	if(status != GL_TRUE)
    {
		std::string log = get_program_log();
		throw ShaderLinkException(log);
	}
    std::string log = get_program_log();
}

void Shader::bind() const
{
    if(!m_impl) throw ShaderNullProgramExc();
	glUseProgram(m_impl->m_Handle);
}

void Shader::unbind()
{
	glUseProgram(0);
}

GLuint Shader::handle() const
{
    return m_impl->m_Handle;
}
    
std::string Shader::get_shader_log(GLuint handle) const
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
    
std::string Shader::get_program_log() const
{
    std::string log;
    GLchar *debugLog;
    GLint debugLength = 0, charsWritten = 0;
    glGetProgramiv(m_impl->m_Handle, GL_INFO_LOG_LENGTH, &debugLength);
    
    if(debugLength > 0)
    {
        debugLog = new GLchar[debugLength];
        glGetProgramInfoLog(m_impl->m_Handle, debugLength, &charsWritten, debugLog);
        log.append(debugLog, 0, debugLength);
        delete [] debugLog;
    }
    return log;
}

void Shader::uniform(const std::string &name, GLint data)
{
	GLint loc = uniform_location(name);
	if(loc != -1) glUniform1i(loc, data);
}

void Shader::uniform(const std::string &name, const glm::vec2 &data)
{
	GLint loc = uniform_location(name);
	if(loc != -1) glUniform2f( loc, data.x, data.y );
}

void Shader::uniform(const std::string &name, const GLint *data, int count)
{
	GLint loc = uniform_location(name);
	if(loc != -1) glUniform1iv( loc, count, data );
}

void Shader::uniform(const std::string &name, const glm::ivec2 *data, int count)
{
	GLint loc = uniform_location(name);
	if(loc != -1) glUniform2iv(loc, count, &data[0].x);
}

void Shader::uniform(const std::string &name, GLfloat data)
{
	GLint loc = uniform_location(name);
	if(loc != -1) glUniform1f(loc, data);
}

void Shader::uniform(const std::string &name, const glm::vec3 &data)
{
	GLint loc = uniform_location(name);
	if(loc != -1) glUniform3f( loc, data.x, data.y, data.z );
}

void Shader::uniform(const std::string &name, const glm::vec4 &data)
{
	GLint loc = uniform_location(name);
	if(loc != -1) glUniform4f( loc, data.x, data.y, data.z, data.w );
}

void Shader::uniform(const std::string &name, const GLfloat *data, int count)
{
	GLint loc = uniform_location(name);
	if(loc != -1) glUniform1fv(loc, count, data);
}

    void Shader::uniform(const std::string &name, const glm::vec2 *theArray, int count)
{
	GLint loc = uniform_location(name);
	if(loc != -1) glUniform2fv(loc, count, &theArray[0].x);
}

void Shader::uniform(const std::string &name, const glm::vec3 *theArray, int count)
{
	GLint loc = uniform_location(name);
	if(loc != -1) glUniform3fv(loc, count, &theArray[0].x);
}

void Shader::uniform(const std::string &name, const glm::vec4 *theArray, int count)
{
	GLint loc = uniform_location(name);
	if(loc != -1) glUniform4fv(loc, count, &theArray[0].x);
}

void Shader::uniform(const std::string &name, const glm::mat3 &theMat, bool transpose)
{
	GLint loc = uniform_location(name);
	if(loc != -1) glUniformMatrix3fv(loc, 1, ( transpose ) ? GL_TRUE : GL_FALSE,
                                     glm::value_ptr(theMat));
}

void Shader::uniform(const std::string &name, const glm::mat4 &theMat, bool transpose)
{
	GLint loc = uniform_location(name);
	if(loc != -1) glUniformMatrix4fv(loc, 1, ( transpose ) ? GL_TRUE : GL_FALSE,
                                     glm::value_ptr(theMat));
}

void Shader::uniform(const std::string &name, const glm::mat3 *theArray, int count, bool transpose)
{
    GLint loc = uniform_location(name);
    if(loc != -1) glUniformMatrix3fv(loc, count, ( transpose ) ? GL_TRUE : GL_FALSE,
                                     glm::value_ptr(theArray[0]));
}

void Shader::uniform(const std::string &name, const glm::mat4 *theArray, int count, bool transpose)
{
    GLint loc = uniform_location(name);
    if(loc != -1) glUniformMatrix4fv(loc, count, ( transpose ) ? GL_TRUE : GL_FALSE,
                                     glm::value_ptr(theArray[0]));
}

void Shader::uniform(const std::string &name, const std::vector<GLint> &theArray)
{
    GLint loc = uniform_location(name);
    if(loc != -1) glUniform1iv(loc, theArray.size(), &theArray[0]);
}

void Shader::uniform(const std::string &name, const std::vector<GLuint> &theArray)
{
    uniform(name, std::vector<GLint>(theArray.begin(), theArray.end()));
}

void Shader::uniform(const std::string &name, const std::vector<GLfloat> &theArray)
{
    GLint loc = uniform_location(name);
	if(loc != -1) glUniform1fv(loc, theArray.size(), &theArray[0]);
}

//void Shader::uniform(const std::string &name, const std::vector<GLdouble> &theArray)
//{
//	GLint loc = uniform_location(name);
//	if(loc != -1) glUniform1dv(loc, theArray.size(), &theArray[0]);
//}

void Shader::uniform(const std::string &name, const std::vector<glm::vec2> &theArray)
{
    GLint loc = uniform_location(name);
	if(loc != -1) glUniform2fv(loc, theArray.size(), &theArray[0].x);
}

void Shader::uniform(const std::string &name, const std::vector<glm::vec3> &theArray)
{
    GLint loc = uniform_location(name);
	if(loc != -1) glUniform3fv(loc, theArray.size(), &theArray[0].x);
}

void Shader::uniform(const std::string &name, const std::vector<glm::vec4> &theArray)
{
    GLint loc = uniform_location(name);
	if(loc != -1) glUniform4fv(loc, theArray.size(), &theArray[0].x);
}

void Shader::uniform(const std::string &name, const std::vector<glm::mat3> &theArray, bool transpose)
{
    GLint loc = uniform_location(name);
    if(loc != -1) glUniformMatrix3fv(loc, theArray.size(), ( transpose ) ? GL_TRUE : GL_FALSE,
                                     glm::value_ptr(theArray[0]));
}

void Shader::uniform(const std::string &name, const std::vector<glm::mat4> &theArray, bool transpose)
{
    GLint loc = uniform_location(name);
    if(loc != -1) glUniformMatrix4fv(loc, theArray.size(), ( transpose ) ? GL_TRUE : GL_FALSE,
                                     glm::value_ptr(theArray[0]));
}

void Shader::bindFragDataLocation(const std::string &fragLoc)
{
#ifndef KINSKI_GLES
    glBindFragDataLocation(m_impl->m_Handle, 0, fragLoc.c_str());
#endif
}
    
GLint Shader::uniform_location(const std::string &name)
{
	map<string,int>::const_iterator uniformIt = m_impl->m_uniform_locations.find(name);
	if( uniformIt == m_impl->m_uniform_locations.end() )
    {
		GLint loc = glGetUniformLocation(m_impl->m_Handle, name.c_str());
        m_impl->m_uniform_locations[name] = loc;
		return loc;
	}
	else
		return uniformIt->second;
}
    
GLint Shader::uniform_block_index(const std::string &name)
{
#ifndef KINSKI_GLES
    
    auto it = m_impl->m_UniformBlockIndices.find(name);
    if(it == m_impl->m_UniformBlockIndices.end())
    {
        GLuint loc = glGetUniformBlockIndex(m_impl->m_Handle, name.c_str());
        m_impl->m_UniformBlockIndices[name] = (loc == GL_INVALID_INDEX) ? -1 : loc;
        return loc;
    }
    else
        return it->second;
#endif
    return -1;
}

bool Shader::uniform_block_binding(const std::string &name, int the_value)
{
#ifndef KINSKI_GLES
	GLint block_index = uniform_block_index(name);

	if(block_index >= 0)
	{
		glUniformBlockBinding(handle(), block_index, the_value);
		KINSKI_CHECK_GL_ERRORS();
		return true;
	}
#endif
    return false;
}

GLint Shader::attrib_location(const std::string &name) const
{
	return glGetAttribLocation( m_impl->m_Handle, name.c_str() );
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
