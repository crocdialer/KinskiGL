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

namespace kinski { namespace gl {

//! Represents an OpenGL GLSL program.
class Shader
{
  public:

	static ShaderPtr create(const std::string &vertexShader, const std::string &fragmentShader,
							const std::string &geometryShader = "");
	void bind() const;
	static void unbind();

	GLuint handle() const;

	void uniform(const std::string &name, GLint data);
    inline void uniform(const std::string &name, GLuint data){ uniform(name, GLint(data)); };
	void uniform(const std::string &name, GLfloat data);
    inline void uniform( const std::string &name, double data){ uniform(name, (float) data); };
    
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
	void uniform(const std::string &name, const std::vector<GLuint> &theArray);
    void uniform(const std::string &name, const std::vector<GLfloat> &theArray);
//	void uniform(const std::string &name, const std::vector<GLdouble> &theArray);
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
	bool uniform_block_binding(const std::string &name, int the_value);
	GLint attrib_location(const std::string &name) const;

	std::string get_shader_log(GLuint handle) const;
    std::string get_program_log() const;
    
    void load_from_data(const std::string &vertSrc, const std::string &fragSrc,
						const std::string &geomSrc = "");
    
  private:
	Shader(const std::string &vertexShader, const std::string &fragmentShader,
		   const std::string &geometryShader);
	void load_shader(const char *shaderSource, GLint shaderType);
	void link();

    std::unique_ptr<struct ShaderImpl> m_impl;
};
    

class ShaderCompileExc : public std::runtime_error
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
    
class ShaderLinkException : public std::runtime_error
{
public:
    ShaderLinkException(const std::string &linkLog):
            std::runtime_error("GLSL: Shader did not link correctly: " + linkLog){};
};

class ShaderNullProgramExc : public std::runtime_error
{
 public:	
    ShaderNullProgramExc(): std::runtime_error("GLSL: Attempt to use null shader"){};
};

}}//namespace
