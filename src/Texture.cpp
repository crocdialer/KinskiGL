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

#include "Texture.h"

using namespace std;
//using namespace glm;

namespace gl {
    
TextureDataExc::TextureDataExc( const std::string &log ) throw()
{ strncpy( mMessage, log.c_str(), 16000 ); }

/////////////////////////////////////////////////////////////////////////////////
// Texture::Format
    
Texture::Format::Format()
{
	m_Target = GL_TEXTURE_2D;
	m_WrapS = GL_CLAMP_TO_EDGE;
	m_WrapT = GL_CLAMP_TO_EDGE;
	m_MinFilter = GL_LINEAR;
	m_MagFilter = GL_LINEAR;
	m_Mipmapping = false;
	m_InternalFormat = -1;
}

/////////////////////////////////////////////////////////////////////////////////
// Texture::Obj
    
struct Texture::Obj {
    Obj() : m_Width( -1 ), m_Height( -1 ), m_CleanWidth( -1 ),
    m_CleanHeight( -1 ), m_InternalFormat( -1 ), m_TextureID( 0 ),
    m_Flipped( false ), m_DeallocatorFunc( 0 ) {};
    
    Obj( int aWidth, int aHeight ) : m_Width( aWidth ), m_Height( aHeight ),
    m_CleanWidth( aWidth ), m_CleanHeight( aHeight ), m_InternalFormat( -1 ),
    m_TextureID( 0 ), m_Flipped( false ), m_boundTextureUnit(-1), 
    m_DeallocatorFunc( 0 )  {};
    
    ~Obj()
    {
        if( m_DeallocatorFunc )
            (*m_DeallocatorFunc)( m_DeallocatorRefcon );
        
        if( ( m_TextureID > 0 ) && ( ! m_DoNotDispose ) ) {
            glDeleteTextures( 1, &m_TextureID );
        }
    }

    
    mutable GLint	m_Width, m_Height, m_CleanWidth, m_CleanHeight;
    float			mMaxU, mMaxV;
    mutable GLint	m_InternalFormat;
    GLenum			m_Target;
    GLuint			m_TextureID;
    glm::mat4       m_textureMatrix;
    
    bool			m_DoNotDispose;
    bool			m_Flipped;
    mutable GLint   m_boundTextureUnit;
    void			(*m_DeallocatorFunc)(void *refcon);
    void			*m_DeallocatorRefcon;			
};

/////////////////////////////////////////////////////////////////////////////////
// Texture
    
Texture::Texture( int aWidth, int aHeight, Format format )
	: m_Obj( ObjPtr( new Obj( aWidth, aHeight ) ) )
{
	if( format.m_InternalFormat == -1 )
		format.m_InternalFormat = GL_RGBA;
	m_Obj->m_InternalFormat = format.m_InternalFormat;
	m_Obj->m_Target = format.m_Target;
	init( (unsigned char*)0, 0, GL_RGBA, GL_UNSIGNED_BYTE, format );
}

Texture::Texture( const unsigned char *data, int dataFormat, int aWidth, int aHeight, Format format )
	: m_Obj( ObjPtr( new Obj( aWidth, aHeight ) ) )
{
	if( format.m_InternalFormat == -1 )
		format.m_InternalFormat = GL_RGBA;
	m_Obj->m_InternalFormat = format.m_InternalFormat;
	m_Obj->m_Target = format.m_Target;
	init( data, 0, dataFormat, GL_UNSIGNED_BYTE, format );
}	

Texture::Texture( GLenum aTarget, GLuint aTextureID, int aWidth, int aHeight, bool aDoNotDispose )
	: m_Obj( ObjPtr( new Obj ) )
{
	m_Obj->m_Target = aTarget;
	m_Obj->m_TextureID = aTextureID;
	m_Obj->m_DoNotDispose = aDoNotDispose;
	m_Obj->m_Width = m_Obj->m_CleanWidth = aWidth;
	m_Obj->m_Height = m_Obj->m_CleanHeight = aHeight;

	if( m_Obj->m_Target == GL_TEXTURE_2D ) {
		m_Obj->mMaxU = m_Obj->mMaxV = 1.0f;
	}
	else {
		m_Obj->mMaxU = (float)m_Obj->m_Width;
		m_Obj->mMaxV = (float)m_Obj->m_Height;
	}
}

void Texture::init( const unsigned char *data, int unpackRowLength, GLenum dataFormat, GLenum type, const Format &format )
{
	m_Obj->m_DoNotDispose = false;
    
    if(!m_Obj->m_TextureID)
    {
        glGenTextures( 1, &m_Obj->m_TextureID );

        glBindTexture( m_Obj->m_Target, m_Obj->m_TextureID );
        glTexParameteri( m_Obj->m_Target, GL_TEXTURE_WRAP_S, format.m_WrapS );
        glTexParameteri( m_Obj->m_Target, GL_TEXTURE_WRAP_T, format.m_WrapT );
        glTexParameteri( m_Obj->m_Target, GL_TEXTURE_MIN_FILTER, format.m_MinFilter );	
        glTexParameteri( m_Obj->m_Target, GL_TEXTURE_MAG_FILTER, format.m_MagFilter );
        
        if( m_Obj->m_Target == GL_TEXTURE_2D ) 
        {
            m_Obj->mMaxU = m_Obj->mMaxV = 1.0f;
        }
        else 
        {
            m_Obj->mMaxU = (float)m_Obj->m_Width;
            m_Obj->mMaxV = (float)m_Obj->m_Height;
        }
	}
    else 
    {
        glBindTexture( m_Obj->m_Target, m_Obj->m_TextureID );
    }
    
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    
#if ! defined( CINDER_GLES )
	glPixelStorei( GL_UNPACK_ROW_LENGTH, unpackRowLength );
#endif
    
	glTexImage2D( m_Obj->m_Target, 0, m_Obj->m_InternalFormat, m_Obj->m_Width, m_Obj->m_Height, 0, dataFormat, type, data );
    
#if ! defined( CINDER_GLES )
	glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
#endif	
    
    if( format.m_Mipmapping )
        glGenerateMipmap(m_Obj->m_Target);
}

void Texture::init( const float *data, GLint dataFormat, const Format &format )
{
	m_Obj->m_DoNotDispose = false;

	glGenTextures( 1, &m_Obj->m_TextureID );

	glBindTexture( m_Obj->m_Target, m_Obj->m_TextureID );
	glTexParameteri( m_Obj->m_Target, GL_TEXTURE_WRAP_S, format.m_WrapS );
	glTexParameteri( m_Obj->m_Target, GL_TEXTURE_WRAP_T, format.m_WrapT );
	glTexParameteri( m_Obj->m_Target, GL_TEXTURE_MIN_FILTER, format.m_MinFilter );	
	glTexParameteri( m_Obj->m_Target, GL_TEXTURE_MAG_FILTER, format.m_MagFilter );
	
    if( m_Obj->m_Target == GL_TEXTURE_2D ) 
    {
		m_Obj->mMaxU = m_Obj->mMaxV = 1.0f;
	}
	else 
    {
		m_Obj->mMaxU = (float)m_Obj->m_Width;
		m_Obj->mMaxV = (float)m_Obj->m_Height;
	}
	
	if( data ) 
    {
		glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
		glTexImage2D( m_Obj->m_Target, 0, m_Obj->m_InternalFormat, m_Obj->m_Width, m_Obj->m_Height, 0, dataFormat, GL_FLOAT, data );
	}
	else
		glTexImage2D( m_Obj->m_Target, 0, m_Obj->m_InternalFormat, m_Obj->m_Width, m_Obj->m_Height, 0, GL_LUMINANCE, GL_FLOAT, 0 );  // init to black...
    
    if( format.m_Mipmapping )
        glGenerateMipmap(m_Obj->m_Target);
}

void Texture::update( const unsigned char *data,GLenum format, int theWidth, int theHeight, bool flipped )
{   
    if(!m_Obj)
    {
        m_Obj = ObjPtr(new Obj(theWidth, theHeight));
        m_Obj->m_Target = GL_TEXTURE_2D;
        m_Obj->m_InternalFormat = GL_RGBA;
    }
    
    if(m_Obj->m_Width == theWidth && m_Obj->m_Height == theHeight)
    {
        glBindTexture( m_Obj->m_Target, m_Obj->m_TextureID );
        glTexSubImage2D( m_Obj->m_Target, 0, 0, 0, m_Obj->m_Width, m_Obj->m_Height, format, GL_UNSIGNED_BYTE, data );
    }
    else 
    {
        m_Obj->m_CleanWidth = m_Obj->m_Width = theWidth;
        m_Obj->m_CleanHeight = m_Obj->m_Height = theHeight;
        setFlipped(flipped);
        
        init(data, 0, format, GL_UNSIGNED_BYTE, Format());
    }
}
    
bool Texture::dataFormatHasAlpha( GLint dataFormat )
{
	switch( dataFormat ) {
		case GL_RGBA:
		case GL_ALPHA:
		case GL_LUMINANCE_ALPHA:
#if ! defined( CINDER_GLES )
		case GL_BGRA:
#endif
			return true;
		break;
		default:
			return false;
	}
}

bool Texture::dataFormatHasColor( GLint dataFormat )
{
	switch( dataFormat ) {
		case GL_ALPHA:
		case GL_LUMINANCE:
		case GL_LUMINANCE_ALPHA:
			return false;
		break;
	}
	
	return true;
}

Texture	Texture::weakClone() const
{
	gl::Texture result = Texture( m_Obj->m_Target, m_Obj->m_TextureID, m_Obj->m_Width, m_Obj->m_Height, true );
	result.m_Obj->m_InternalFormat = m_Obj->m_InternalFormat;
	result.m_Obj->m_Flipped = m_Obj->m_Flipped;	
	return result;
}

void Texture::setDeallocator( void(*aDeallocatorFunc)( void * ), void *aDeallocatorRefcon )
{
	m_Obj->m_DeallocatorFunc = aDeallocatorFunc;
	m_Obj->m_DeallocatorRefcon = aDeallocatorRefcon;
}

void Texture::setDoNotDispose( bool aDoNotDispose ) 
{ 
    m_Obj->m_DoNotDispose = aDoNotDispose; 
}

    void Texture::setTextureMatrix( const glm::mat4 &theMatrix )
{
    m_Obj->m_textureMatrix = theMatrix;
}
    
const glm::mat4 &Texture::getTextureMatrix() const 
{ 
    return m_Obj->m_textureMatrix; 
}

const bool Texture::isBound() const
{
    return m_Obj->m_boundTextureUnit >= 0;
}

const GLint Texture::getBoundTextureUnit() const
{
    return m_Obj->m_boundTextureUnit;
}
    
GLuint Texture::getId() const 
{ 
    return m_Obj->m_TextureID; 
}

GLenum Texture::getTarget() const 
{ 
    return m_Obj->m_Target; 
}

//!	whether the texture is flipped vertically
bool Texture::isFlipped() const 
{
    return m_Obj->m_Flipped; 
}

//!	Marks the texture as being flipped vertically or not
void Texture::setFlipped( bool aFlipped ) 
{ 
    m_Obj->m_Flipped = aFlipped;
    m_Obj->m_textureMatrix = glm::mat4();
    
    if(aFlipped) 
    {
        glm::mat4 flipY;
        flipY[1] = glm::vec4(0, -1, 0, 1);// invert y-coords
        flipY[3] = glm::vec4(0, 1, 0, 1); // [-1,0] -> [0,1]
        
        m_Obj->m_textureMatrix *= flipY;
    }
}
    
void Texture::setWrapS( GLenum wrapS )
{
	glTexParameteri( m_Obj->m_Target, GL_TEXTURE_WRAP_S, wrapS );
}

void Texture::setWrapT( GLenum wrapT )
{
	glTexParameteri( m_Obj->m_Target, GL_TEXTURE_WRAP_T, wrapT );
}

void Texture::setMinFilter( GLenum minFilter )
{
	glTexParameteri( m_Obj->m_Target, GL_TEXTURE_MIN_FILTER, minFilter );
}

void Texture::setMagFilter( GLenum magFilter )
{
	glTexParameteri( m_Obj->m_Target, GL_TEXTURE_MAG_FILTER, magFilter );
}

void Texture::setCleanTexCoords( float maxU, float maxV )
{
	m_Obj->mMaxU = maxU;
	m_Obj->mMaxV = maxV;
	
	if( m_Obj->m_Target == GL_TEXTURE_2D ) {
		m_Obj->m_CleanWidth = getWidth() * maxU;
		m_Obj->m_CleanHeight = getHeight() * maxV;
	}
	else {
		m_Obj->m_CleanWidth = (int32_t)maxU;
		m_Obj->m_CleanHeight = (int32_t)maxV;
	}
}

bool Texture::hasAlpha() const
{
	switch( m_Obj->m_InternalFormat ) {
#if ! defined( CINDER_GLES )
		case GL_RGBA8:
		case GL_RGBA16:
		case GL_RGBA32F_ARB:
		case GL_LUMINANCE8_ALPHA8:
		case GL_LUMINANCE16_ALPHA16:
		case GL_LUMINANCE_ALPHA32F_ARB:
#endif
		case GL_RGBA:
		case GL_LUMINANCE_ALPHA:
			return true;
		break;
		default:
			return false;
		break;
	}
}
	
float Texture::getLeft() const
{
	return 0.0f;
}

float Texture::getRight() const
{
	return m_Obj->mMaxU;
}

float Texture::getTop() const
{
	return ( m_Obj->m_Flipped ) ? getMaxV() : 0.0f;
}

GLint Texture::getInternalFormat() const
{
#if ! defined( CINDER_GLES )
	if( m_Obj->m_InternalFormat == -1 ) {
		bind();
		glGetTexLevelParameteriv( m_Obj->m_Target, 0, GL_TEXTURE_INTERNAL_FORMAT, &m_Obj->m_InternalFormat );
	}
#endif // ! defined( CINDER_GLES )
	
	return m_Obj->m_InternalFormat;
}

GLint Texture::getWidth() const
{
#if ! defined( CINDER_GLES )
	if( m_Obj->m_Width == -1 ) {
		bind();
		glGetTexLevelParameteriv( m_Obj->m_Target, 0, GL_TEXTURE_WIDTH, &m_Obj->m_Width );
		m_Obj->m_CleanWidth = m_Obj->m_Width;
	}
#endif // ! defined( CINDER_GLES )

	return m_Obj->m_Width;
}

GLint Texture::getHeight() const
{
#if ! defined( CINDER_GLES )
	if( m_Obj->m_Height == -1 ) {
		//TODO: examine in cinder src
//        gl::SaveTextureBindState( m_Obj->m_Target );
		bind();
		glGetTexLevelParameteriv( m_Obj->m_Target, 0, GL_TEXTURE_HEIGHT, &m_Obj->m_Height );	
		m_Obj->m_CleanHeight = m_Obj->m_Height;		
	}
#endif // ! defined( CINDER_GLES )	
	return m_Obj->m_Height;
}

GLint Texture::getCleanWidth() const
{
#if ! defined( CINDER_GLES )
	if( m_Obj->m_CleanWidth == -1 ) {
		bind();
		glGetTexLevelParameteriv( m_Obj->m_Target, 0, GL_TEXTURE_WIDTH, &m_Obj->m_Width );
		m_Obj->m_CleanWidth = m_Obj->m_Width;
	}

	return m_Obj->m_CleanWidth;
#else
	return m_Obj->m_Width;
#endif // ! defined( CINDER_GLES )	
}

GLint Texture::getCleanHeight() const
{
#if ! defined( CINDER_GLES )
	if( m_Obj->m_CleanHeight == -1 ) {
		bind();
		glGetTexLevelParameteriv( m_Obj->m_Target, 0, GL_TEXTURE_HEIGHT, &m_Obj->m_Height );	
		m_Obj->m_CleanHeight = m_Obj->m_Height;		
	}
	
	return m_Obj->m_CleanHeight;
#else
	return m_Obj->m_Height;
#endif // ! defined( CINDER_GLES )	
}

float Texture::getBottom() const
{
	return ( m_Obj->m_Flipped ) ? 0.0f : getMaxV();
}

float Texture::getMaxU() const
{ 
	return m_Obj->mMaxU;
}

float Texture::getMaxV() const
{
	return m_Obj->mMaxV;
}

void Texture::bind( GLuint textureUnit ) const
{
    if(!m_Obj) return;//throw TextureDataExc("Tried to bind uninitialized texture ...");
    
    m_Obj->m_boundTextureUnit = textureUnit;
    
	glActiveTexture( GL_TEXTURE0 + textureUnit );
	glBindTexture( m_Obj->m_Target, m_Obj->m_TextureID );
	glActiveTexture( GL_TEXTURE0 );
}

void Texture::unbind( GLuint textureUnit ) const
{
    if(!m_Obj) return;//throw TextureDataExc("Tried to unbind uninitialized texture ...");
    
    m_Obj->m_boundTextureUnit = -1;
    
	glActiveTexture( GL_TEXTURE0 + textureUnit );
	glBindTexture( m_Obj->m_Target, 0 );
	glActiveTexture( GL_TEXTURE0 );
}

void Texture::enableAndBind() const
{
	glEnable( m_Obj->m_Target );
	glBindTexture( m_Obj->m_Target, m_Obj->m_TextureID );
}

void Texture::disable() const
{
	glDisable( m_Obj->m_Target );
}

/////////////////////////////////////////////////////////////////////////////////

} // namespace gl
