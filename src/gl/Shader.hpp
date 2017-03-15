// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "gl/gl.hpp"

#define BUFFER_OFFSET(i) ((char *)nullptr + (i))

namespace kinski { namespace gl {

//! Represents an OpenGL GLSL program.
class KINSKI_API Shader
{
  public: 
	Shader();
    
	Shader(const char *vertexShader, const char *fragmentShader,
           const char *geometryShader = nullptr);

	void bind() const;
	static void unbind();

	GLuint getHandle() const;

	void uniform(const std::string &name, GLint data);
    inline void uniform(const std::string &name, GLuint data){ uniform(name, GLint(data)); };
	void uniform(const std::string &name, GLfloat data);
    inline void uniform( const std::string &name, double data ){ uniform(name, (float) data); };
    
	void uniform(const std::string &name, const GLint *data, int count);
	void uniform(const std::string &name, const ivec2 *theArray, int count);

	void uniform(const std::string &name, const vec2 &theVec);
	void uniform(const std::string &name, const vec3 &theVec);
    void uniform(const std::string &name, const vec4 &theVec);
	void uniform(const std::string &name, const mat3 &theMat, bool transpose = false);
	void uniform(const std::string &name, const mat4 &theMat, bool transpose = false);
    
    // uniform array
	void uniform(const std::string &name, const GLfloat *theArray, int count);
	void uniform(const std::string &name, const vec2 *theArray, int count);
	void uniform(const std::string &name, const vec3 *theArray, int count);
	void uniform(const std::string &name, const vec4 *theArray, int count);
    void uniform(const std::string &name, const mat3 *theArray, int count,
                 bool transpose = false);
    void uniform(const std::string &name, const mat4 *theArray, int count,
                 bool transpose = false);
    
    // uniform std::vector
    void uniform(const std::string &name, const std::vector<GLint> &theArray);
    void uniform(const std::string &name, const std::vector<GLfloat> &theArray);
	void uniform(const std::string &name, const std::vector<vec2> &theArray);
	void uniform(const std::string &name, const std::vector<vec3> &theArray);
	void uniform(const std::string &name, const std::vector<vec4> &theArray);
    void uniform(const std::string &name, const std::vector<mat3> &theArray,
                 bool transpose = false);
    void uniform(const std::string &name, const std::vector<mat4> &theArray,
                 bool transpose = false);
    
    void bindFragDataLocation(const std::string &fragLoc);
    
	GLint uniform_location(const std::string &name);
    GLint uniform_block_index(const std::string &name);
	GLint attrib_location(const std::string &name) const;

	std::string get_shader_log(GLuint handle) const;
    std::string get_program_log() const;
    
    void load_from_data(const std::string &vertSrc, const std::string &fragSrc,
						const std::string &geomSrc = "");
    
  private:
	void load_shader(const char *shaderSource, GLint shaderType);
	void link();

    std::shared_ptr<struct ShaderImpl> m_impl;

  public:
	//! Emulates shared_ptr-like behavior
	operator bool() const { return m_impl.get() != nullptr; }
	void reset() { m_impl.reset(); }
    
    bool operator==(const Shader &other) const { return m_impl.get() == other.m_impl.get();}
    bool operator!=(const Shader &other) const { return m_impl.get() != other.m_impl.get();}
};
    

class ShaderCompileExc : public Exception
{
 public:	
	ShaderCompileExc( const std::string &log, GLint aShaderType );
	virtual const char* what() const throw()
	{
		return m_Message;
	}

 private:
	char	m_Message[16001];
	GLint	m_ShaderType;
};
    
class ShaderLinkException : public Exception
{
public:
    ShaderLinkException(const std::string &linkLog):
        Exception("GLSL: Shader did not link correctly: " + linkLog){};
};

class ShaderNullProgramExc : public Exception
{
 public:	
    ShaderNullProgramExc(): Exception("GLSL: Attempt to use null shader"){};
};

}}//namespace