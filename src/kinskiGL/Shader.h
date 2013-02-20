// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef _KINSKI_SHADER_INCLUDED_
#define _KINSKI_SHADER_INCLUDED_

#include "KinskiGL.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

namespace kinski { namespace gl {

//! Represents an OpenGL GLSL program. \ImplShared
class Shader
{
  public: 
	Shader();
    
	Shader( const char *vertexShader, const char *fragmentShader,
            const char *geometryShader = 0, GLint geometryInputType = GL_POINTS,
            GLint geometryOutputType = GL_TRIANGLES,
            GLint geometryOutputVertices = 0);

	void			bind() const;
	static void		unbind();

	GLuint			getHandle() const ;

	void uniform( const std::string &name, GLint data );
	void uniform( const std::string &name, GLfloat data );
    void uniform( const std::string &name, double data ){ uniform(name, (float) data); };
    
	void uniform( const std::string &name, const GLint *data, int count );
	void uniform( const std::string &name, const glm::ivec2 *theArray, int count );	

	void uniform( const std::string &name, const glm::vec2 &theVec );
	void uniform( const std::string &name, const glm::vec3 &theVec );
    void uniform( const std::string &name, const glm::vec4 &theVec );
	void uniform( const std::string &name, const glm::mat3 &theMat, bool transpose = false );
	void uniform( const std::string &name, const glm::mat4 &theMat, bool transpose = false );
    
    // uniform array
	void uniform( const std::string &name, const GLfloat *theArray, int count );
	void uniform( const std::string &name, const glm::vec2 *theArray, int count );
	void uniform( const std::string &name, const glm::vec3 *theArray, int count );
	void uniform( const std::string &name, const glm::vec4 *theArray, int count );
    void uniform( const std::string &name, const glm::mat3 *theArray, int count,
                  bool transpose = false );
    void uniform( const std::string &name, const glm::mat4 *theArray, int count,
                  bool transpose = false );
    
    // uniform std::vector
    void uniform( const std::string &name, const std::vector<GLint> &theArray );
    void uniform( const std::string &name, const std::vector<GLfloat> &theArray );
	void uniform( const std::string &name, const std::vector<glm::vec2> &theArray );
	void uniform( const std::string &name, const std::vector<glm::vec3> &theArray );
	void uniform( const std::string &name, const std::vector<glm::vec4> &theArray );
    void uniform( const std::string &name, const std::vector<glm::mat3> &theArray,
                  bool transpose = false);
    void uniform( const std::string &name, const std::vector<glm::mat4> &theArray,
                  bool transpose = false);
    
    void bindFragDataLocation(const std::string &fragLoc);
    
	GLint getUniformLocation( const std::string &name );
	GLint getAttribLocation( const std::string &name ) const;

	std::string getShaderLog( GLuint handle ) const;
    std::string getProgramLog() const;
    
    void loadFromData(const char *vertSrc, const char *fragSrc,
                      const char *geomSrc = NULL);
    void loadFromFile(const std::string &vertPath, const std::string &fragPath,
                      const std::string &geomPath="");
    
  private:
	void loadShader( const char *shaderSource, GLint shaderType );
	void attachShaders();
	void link();

	struct Obj;
    typedef std::shared_ptr<Obj> ObjPtr;
	mutable ObjPtr m_Obj;

  public:
	//@{
	//! Emulates shared_ptr-like behavior
	typedef ObjPtr Shader::*unspecified_bool_type;
	operator unspecified_bool_type() const { return ( m_Obj.get() == 0 ) ? 0 : &Shader::m_Obj; }
	void reset() { m_Obj.reset(); }
	//@}  
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
#endif // _KINSKI_SHADER_INCLUDED_