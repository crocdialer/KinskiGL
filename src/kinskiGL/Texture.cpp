// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Texture.h"

using namespace std;

namespace kinski{
namespace gl {
    
/////////////////////////////////////////////////////////////////////////////////
// Texture::Format
    
Texture::Format::Format()
{
	m_Target = GL_TEXTURE_2D;
	m_WrapS = GL_REPEAT;//GL_CLAMP_TO_EDGE;
	m_WrapT = GL_REPEAT;//GL_CLAMP_TO_EDGE;
	m_MinFilter = GL_LINEAR;
	m_MagFilter = GL_LINEAR;
	m_Mipmapping = false;
	m_InternalFormat = -1;
}

/////////////////////////////////////////////////////////////////////////////////
// Texture::Obj
    
struct Texture::Obj
    {
    Obj() : m_Width( -1 ), m_Height( -1 ), m_InternalFormat( -1 ), m_dataType(-1),
    m_TextureID( 0 ), m_Flipped( false ), m_DeallocatorFunc( 0 ) {};
    
    Obj( int aWidth, int aHeight ) : m_Width( aWidth ), m_Height( aHeight ),
    m_InternalFormat( -1 ),
    m_dataType(-1),
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

    
    mutable GLint	m_Width, m_Height;
    mutable GLint	m_InternalFormat;
    GLenum          m_dataType;
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
Texture::Texture(): m_Obj( ObjPtr( new Obj )){}
    
Texture::Texture( int aWidth, int aHeight, Format format )
	: m_Obj( ObjPtr( new Obj( aWidth, aHeight ) ) )
{
	if( format.m_InternalFormat == -1 )
		format.m_InternalFormat = GL_RGBA;
	m_Obj->m_InternalFormat = format.m_InternalFormat;
	m_Obj->m_Target = format.m_Target;
    m_Obj->m_dataType = GL_UNSIGNED_BYTE;
	init( (unsigned char*)0, GL_RGBA, format, 0);
}

Texture::Texture( const unsigned char *data, int dataFormat, int aWidth, int aHeight, Format format )
	: m_Obj( ObjPtr( new Obj( aWidth, aHeight ) ) )
{
	if( format.m_InternalFormat == -1 )
		format.m_InternalFormat = GL_RGBA;
	m_Obj->m_InternalFormat = format.m_InternalFormat;
	m_Obj->m_Target = format.m_Target;
    m_Obj->m_dataType = GL_UNSIGNED_BYTE;
	init( data, dataFormat, format );
}	

Texture::Texture( GLenum aTarget, GLuint aTextureID, int aWidth, int aHeight, bool aDoNotDispose )
	: m_Obj( ObjPtr( new Obj ) )
{
	m_Obj->m_Target = aTarget;
	m_Obj->m_TextureID = aTextureID;
	m_Obj->m_DoNotDispose = aDoNotDispose;
	m_Obj->m_Width = aWidth;
	m_Obj->m_Height = aHeight;

}

void Texture::init(const unsigned char *data, GLenum dataFormat,
                   const Format &format, int unpackRowLength)
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
        
	}
    else 
    {
        glBindTexture( m_Obj->m_Target, m_Obj->m_TextureID );
    }
    
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    
#if ! defined( KINSKI_GLES )
	glPixelStorei( GL_UNPACK_ROW_LENGTH, unpackRowLength );
#endif
    
	glTexImage2D(m_Obj->m_Target, 0, m_Obj->m_InternalFormat,
                 m_Obj->m_Width, m_Obj->m_Height, 0, dataFormat,
                 GL_UNSIGNED_BYTE, data );
    
#if ! defined( KINSKI_GLES )
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
	
	if( data ) 
    {
		glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
		glTexImage2D( m_Obj->m_Target, 0, m_Obj->m_InternalFormat, m_Obj->m_Width, m_Obj->m_Height, 0, dataFormat, GL_FLOAT, data );
	}
	else
    {
        GLuint mode;
    
#ifdef KINSKI_GLES
        mode = GL_LUMINANCE;
#else 
        mode = GL_RED;
#endif
        // init to black...
		glTexImage2D( m_Obj->m_Target, 0, m_Obj->m_InternalFormat, m_Obj->m_Width, m_Obj->m_Height, 0,
                     mode,
                     GL_FLOAT, 0 );
    }
    
    if( format.m_Mipmapping )
        glGenerateMipmap(m_Obj->m_Target);
}

void Texture::update( const unsigned char *data,GLenum format, int theWidth, int theHeight, bool flipped )
{   
    update(data, GL_UNSIGNED_BYTE, format, theWidth, theHeight, flipped);
}
    
void Texture::update( const float *data,GLenum format, int theWidth, int theHeight, bool flipped )
{
    update(data, GL_FLOAT, format, theWidth, theHeight, flipped);
}
    
void Texture::update(const void *data,
                     GLenum dataType,GLenum format,
                     int theWidth, int theHeight,
                     bool flipped )
{
    if(m_Obj->m_Width == theWidth && 
       m_Obj->m_Height == theHeight &&
       m_Obj->m_dataType == dataType)
    {
        glBindTexture( m_Obj->m_Target, m_Obj->m_TextureID );
        glTexSubImage2D( m_Obj->m_Target, 0, 0, 0, m_Obj->m_Width, m_Obj->m_Height, format, dataType, data );
    }
    else 
    {
        m_Obj->m_Target = GL_TEXTURE_2D;
        m_Obj->m_dataType = dataType;
        m_Obj->m_InternalFormat = GL_RGBA;
        m_Obj->m_Width = theWidth;
        m_Obj->m_Height = theHeight;
        setFlipped(flipped);
        
        if(dataType == GL_UNSIGNED_BYTE)
            init((unsigned char*)data, format, Format());
        else if(dataType == GL_FLOAT)
            init((float*) data, format, Format());
            
    }
}
    
bool Texture::dataFormatHasAlpha( GLint dataFormat )
{
	switch( dataFormat ) {
		case GL_RGBA:
		case GL_ALPHA:
#if ! defined( KINSKI_GLES )
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
        //case GL_LUMINANCE:
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

bool Texture::hasAlpha() const
{
	switch( m_Obj->m_InternalFormat ) {
#if ! defined( KINSKI_GLES )
		case GL_RGBA8:
		case GL_RGBA16:
		case GL_RGBA32F:
#endif
		case GL_RGBA:
			return true;
		break;
		default:
			return false;
		break;
	}
}
	
float Texture::getLeft() const
{
	return (glm::vec4(0, 0, 0, 1) * m_Obj->m_textureMatrix).x;
}

float Texture::getRight() const
{
	return (glm::vec4(1, 0, 0, 1) * m_Obj->m_textureMatrix).x;
}

float Texture::getTop() const
{
	return (glm::vec4(0, 1, 0, 1) * m_Obj->m_textureMatrix).y;
}
    
float Texture::getBottom() const
{
    return (glm::vec4(0, 0, 0, 1) * m_Obj->m_textureMatrix).y;
}

GLint Texture::getInternalFormat() const
{
#if ! defined( KINSKI_GLES )
	if( m_Obj->m_InternalFormat == -1 ) {
		bind();
		glGetTexLevelParameteriv( m_Obj->m_Target, 0, GL_TEXTURE_INTERNAL_FORMAT, &m_Obj->m_InternalFormat );
	}
#endif // ! defined( KINSKI_GLES )
	
	return m_Obj->m_InternalFormat;
}

GLint Texture::getWidth() const
{
#if ! defined( KINSKI_GLES )
	if( m_Obj->m_Width == -1 )
    {
		bind();
		glGetTexLevelParameteriv( m_Obj->m_Target, 0, GL_TEXTURE_WIDTH, &m_Obj->m_Width );
	}
#endif

	return m_Obj->m_Width;
}

GLint Texture::getHeight() const
{
#if ! defined( KINSKI_GLES )
	if( m_Obj->m_Height == -1 )
    {
		bind();
		glGetTexLevelParameteriv( m_Obj->m_Target, 0, GL_TEXTURE_HEIGHT, &m_Obj->m_Height );	
	}
#endif
	return m_Obj->m_Height;
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
}