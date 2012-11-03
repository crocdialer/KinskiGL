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

#ifndef _KINSKI_SHADER_INCLUDED_
#define _KINSKI_SHADER_INCLUDED_

#include <exception>
#include "KinskiGL.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

namespace kinski{
namespace gl {

//! Represents an OpenGL GLSL program. \ImplShared
class Shader {
  public: 
	Shader() {}
    
	Shader( const char *vertexShader, const char *fragmentShader,
            const char *geometryShader = 0, GLint geometryInputType = GL_POINTS,
            GLint geometryOutputType = GL_TRIANGLES,
            GLint geometryOutputVertices = 0);

	void			bind() const;
	static void		unbind();

	GLuint			getHandle() const ;

	void uniform( const std::string &name, GLint data );
	void uniform( const std::string &name, GLfloat data );
    void uniform( const std::string &name, GLdouble data ){ uniform(name, (float) data); };
    
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
	GLint getAttribLocation( const std::string &name );

	std::string getShaderLog( GLuint handle ) const;
    
    void loadFromData(const char *vertSrc, const char *fragSrc,
                      const char *geomSrc = NULL);
    void loadFromFile(const std::string &vertPath, const std::string &fragPath,
                      const std::string &geomPath="");
    
  private:
	void loadShader( const char *shaderSource, GLint shaderType );
	void attachShaders();
	void link();
    
    const std::string readFile(const std::string &path);

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

class ShaderCompileExc : public std::exception
{
 public:	
	ShaderCompileExc( const std::string &log, GLint aShaderType ) throw();
	virtual const char* what() const throw()
	{
		return m_Message;
	}

 private:
	char	m_Message[16001];
	GLint	m_ShaderType;
};

class ShaderNullProgramExc : public std::exception {
 public:	
	virtual const char* what() const throw()
	{
		return "Glsl: Attempt to use null shader";
	}

};

}
}
#endif // _KINSKI_SHADER_INCLUDED_