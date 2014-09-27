// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#if ! defined( KINSKI_GLES )

#include "Texture.h"
#include "ArrayTexture.h"

using namespace std;

namespace kinski{
namespace gl {
    
/////////////////////////////////////////////////////////////////////////////////
// Texture::Format
    
ArrayTexture::Format::Format()
{
	m_Target = GL_TEXTURE_2D_ARRAY;//GL_TEXTURE_3D
	m_WrapS = GL_CLAMP_TO_EDGE;//GL_REPEAT not working in ios
	m_WrapT = GL_CLAMP_TO_EDGE;
	m_MinFilter = GL_LINEAR;
	m_MagFilter = GL_LINEAR;
	m_Mipmapping = false;
	m_InternalFormat = -1;
}

/////////////////////////////////////////////////////////////////////////////////
// Texture::Obj
    
struct ArrayTexture::Obj
{
    Obj() :
    m_Width( -1 ),
    m_Height( -1 ),
    m_Depth( -1 ),
    m_InternalFormat( -1 ),
    m_dataType(-1),
    m_Target(GL_TEXTURE_2D_ARRAY),
    m_TextureID( 0 ),
    m_Flipped( false ),
    m_mip_map(false),
    m_DeallocatorFunc( 0 ) {};
    
    Obj( int aWidth, int aHeight, int aDepth ) :
    m_Width( aWidth ),
    m_Height( aHeight ),
    m_Depth( aDepth ),
    m_InternalFormat( -1 ),
    m_dataType(-1),
    m_Target(GL_TEXTURE_2D_ARRAY),
    m_TextureID( 0 ), m_Flipped( false ), m_mip_map(false), m_boundTextureUnit(-1),
    m_DeallocatorFunc( 0 )  {};
    
    ~Obj()
    {
        if( m_DeallocatorFunc )
            (*m_DeallocatorFunc)( m_DeallocatorRefcon );
        
        if( ( m_TextureID > 0 ) && ( ! m_DoNotDispose ) ) {
            glDeleteTextures( 1, &m_TextureID );
        }
    }

    
    mutable GLint	m_Width, m_Height, m_Depth;
    mutable GLint	m_InternalFormat;
    GLenum          m_dataType;
    GLenum			m_Target;
    GLuint			m_TextureID;
    
    bool			m_DoNotDispose;
    bool			m_Flipped;
    bool            m_mip_map;
    mutable GLint   m_boundTextureUnit;
    void			(*m_DeallocatorFunc)(void *refcon);
    void			*m_DeallocatorRefcon;			
};

/////////////////////////////////////////////////////////////////////////////////
// Texture
    
ArrayTexture::ArrayTexture( int aWidth, int aHeight, int aDepth, Format format )
	: m_Obj(new Obj( aWidth, aHeight, aDepth))
{
	if( format.m_InternalFormat == -1 )
		format.m_InternalFormat = GL_RGBA;
	m_Obj->m_InternalFormat = format.m_InternalFormat;
	m_Obj->m_Target = format.m_Target;
    m_Obj->m_dataType = GL_UNSIGNED_BYTE;
	init( (unsigned char*)0, GL_RGBA, format, 0);
}

ArrayTexture::ArrayTexture(const unsigned char *data, int dataFormat, int aWidth, int aHeight,
                           int aDepth, Format format )
	: m_Obj(new Obj( aWidth, aHeight, aDepth))
{
	if( format.m_InternalFormat == -1 )
		format.m_InternalFormat = GL_RGBA;
	m_Obj->m_InternalFormat = format.m_InternalFormat;
	m_Obj->m_Target = format.m_Target;
    m_Obj->m_dataType = GL_UNSIGNED_BYTE;
	init( data, dataFormat, format );
}	

ArrayTexture::ArrayTexture(GLenum aTarget, GLuint aTextureID, int aWidth, int aHeight,
                           int aDepth, bool aDoNotDispose )
	: m_Obj( ObjPtr( new Obj() ) )
{
	m_Obj->m_Target = aTarget;
	m_Obj->m_TextureID = aTextureID;
	m_Obj->m_DoNotDispose = aDoNotDispose;
	m_Obj->m_Width = aWidth;
	m_Obj->m_Height = aHeight;

}

void ArrayTexture::init(const unsigned char *data, GLenum dataFormat,
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
    else{ glBindTexture( m_Obj->m_Target, m_Obj->m_TextureID ); }
    
//	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
//    
//#if ! defined( KINSKI_GLES )
//	glPixelStorei( GL_UNPACK_ROW_LENGTH, unpackRowLength );
//#endif
    
//	glTexImage2D(m_Obj->m_Target, 0, m_Obj->m_InternalFormat,
//                 m_Obj->m_Width, m_Obj->m_Height, 0, dataFormat,
//                 GL_UNSIGNED_BYTE, data );
    
    glTexImage3D(m_Obj->m_Target, 0, m_Obj->m_InternalFormat, m_Obj->m_Width, m_Obj->m_Height,
                 m_Obj->m_Depth, 0, dataFormat, GL_UNSIGNED_BYTE, data);
    
//#if ! defined( KINSKI_GLES )
//	glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
//#endif	
    
//    if( format.m_Mipmapping ){ glGenerateMipmap(m_Obj->m_Target); }
//    
//    if(dataFormat != GL_RGB && dataFormat != GL_RGBA)
//    {
//        #ifndef KINSKI_GLES
//            GLint swizzleMask[] = {(GLint)(dataFormat), (GLint)(dataFormat), (GLint)(dataFormat),
//                GL_ONE};
//            glTexParameteriv(m_Obj->m_Target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
//        #endif
//    }
    
    KINSKI_CHECK_GL_ERRORS();
}
    
bool ArrayTexture::dataFormatHasAlpha( GLint dataFormat )
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

bool ArrayTexture::dataFormatHasColor( GLint dataFormat )
{
	switch( dataFormat )
    {
		case GL_ALPHA:
			return false;
		break;
	}
	
	return true;
}

ArrayTexture ArrayTexture::weakClone() const
{
	gl::ArrayTexture result = ArrayTexture(m_Obj->m_Target, m_Obj->m_TextureID, m_Obj->m_Width,
                                           m_Obj->m_Height, m_Obj->m_Depth, true );
	result.m_Obj->m_InternalFormat = m_Obj->m_InternalFormat;
	result.m_Obj->m_Flipped = m_Obj->m_Flipped;	
	return result;
}

void ArrayTexture::setDeallocator( void(*aDeallocatorFunc)( void * ), void *aDeallocatorRefcon )
{
	m_Obj->m_DeallocatorFunc = aDeallocatorFunc;
	m_Obj->m_DeallocatorRefcon = aDeallocatorRefcon;
}

void ArrayTexture::setDoNotDispose( bool aDoNotDispose )
{ 
    m_Obj->m_DoNotDispose = aDoNotDispose; 
}

void ArrayTexture::setTextureMatrix( const glm::mat4 &theMatrix )
{
    m_textureMatrix = theMatrix;
}
    
glm::mat4 ArrayTexture::getTextureMatrix() const
{
    glm::mat4 ret = m_textureMatrix;
    
    if(m_Obj->m_Flipped)
    {
        static glm::mat4 flipY = glm::mat4(glm::vec4(1, 0, 0, 1),
                                           glm::vec4(0, -1, 0, 1),// invert y-coords
                                           glm::vec4(0, 0, 1, 1),
                                           glm::vec4(0, 1, 0, 1));// [-1,0] -> [0,1]
        ret *= flipY;
    }
    return ret;
}

const bool ArrayTexture::isBound() const
{
    return m_Obj->m_boundTextureUnit >= 0;
}

const GLint ArrayTexture::getBoundTextureUnit() const
{
    return m_Obj->m_boundTextureUnit;
}
    
GLuint ArrayTexture::getId() const
{ 
    return m_Obj->m_TextureID; 
}

GLenum ArrayTexture::getTarget() const
{ 
    return m_Obj->m_Target; 
}

//!	whether the texture is flipped vertically
bool ArrayTexture::isFlipped() const
{
    return m_Obj->m_Flipped; 
}

//!	Marks the texture as being flipped vertically or not
void ArrayTexture::setFlipped( bool aFlipped )
{
    m_Obj->m_Flipped = aFlipped;
}
    
void ArrayTexture::setWrapS( GLenum wrapS )
{
	glTexParameteri( m_Obj->m_Target, GL_TEXTURE_WRAP_S, wrapS );
}

void ArrayTexture::setWrapT( GLenum wrapT )
{
	glTexParameteri( m_Obj->m_Target, GL_TEXTURE_WRAP_T, wrapT );
}

void ArrayTexture::setMinFilter( GLenum minFilter )
{
    glTexParameteri( m_Obj->m_Target, GL_TEXTURE_MIN_FILTER, minFilter );
}

void ArrayTexture::setMagFilter( GLenum magFilter )
{
    glTexParameteri( m_Obj->m_Target, GL_TEXTURE_MAG_FILTER, magFilter );
}

void ArrayTexture::set_mipmapping(bool b)
{
    if(b && !m_Obj->m_mip_map)
    {
        setMinFilter(GL_LINEAR_MIPMAP_NEAREST);
        glGenerateMipmap(m_Obj->m_Target);
    }
    else
    {
        setMinFilter(GL_LINEAR);
    }
    m_Obj->m_mip_map = b;
}

void ArrayTexture::set_anisotropic_filter(float f)
{
    if(gl::isExtensionSupported("GL_EXT_texture_filter_anisotropic"))
    {
        GLfloat fLargest;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
        bind();
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min(fLargest, f));
        unbind();
    }
}
    
bool ArrayTexture::hasAlpha() const
{
    if(!m_Obj) throw TextureDataExc("Texture not initialized ...");
    
	switch( m_Obj->m_InternalFormat )
    {
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
	
float ArrayTexture::getLeft() const
{
	return (m_textureMatrix * glm::vec4(0, 0, 0, 1)).x;
}

float ArrayTexture::getRight() const
{
	return (m_textureMatrix * glm::vec4(1, 0, 0, 1)).x;
}

float ArrayTexture::getTop() const
{
	return (m_textureMatrix * glm::vec4(0, 1, 0, 1)).y;
}
    
float ArrayTexture::getBottom() const
{
    return (m_textureMatrix * glm::vec4(0, 0, 0, 1)).y;
}

GLint ArrayTexture::getInternalFormat() const
{
    if(!m_Obj) throw TextureDataExc("Texture not initialized ...");
#if ! defined( KINSKI_GLES )
	if( m_Obj->m_InternalFormat == -1 )
    {
		bind();
		glGetTexLevelParameteriv( m_Obj->m_Target, 0, GL_TEXTURE_INTERNAL_FORMAT, &m_Obj->m_InternalFormat );
	}
#endif // ! defined( KINSKI_GLES )
	
	return m_Obj->m_InternalFormat;
}

GLint ArrayTexture::getWidth() const
{
    if(!m_Obj) throw TextureDataExc("Texture not initialized ...");
#if ! defined( KINSKI_GLES )
	if( m_Obj->m_Width == -1 )
    {
		bind();
		glGetTexLevelParameteriv( m_Obj->m_Target, 0, GL_TEXTURE_WIDTH, &m_Obj->m_Width );
	}
#endif

	return m_Obj->m_Width;
}

GLint ArrayTexture::getHeight() const
{
    if(!m_Obj) throw TextureDataExc("Texture not initialized ...");
#if ! defined( KINSKI_GLES )
	if( m_Obj->m_Height == -1 )
    {
		bind();
		glGetTexLevelParameteriv( m_Obj->m_Target, 0, GL_TEXTURE_HEIGHT, &m_Obj->m_Height );	
	}
#endif
	return m_Obj->m_Height;
}

GLint ArrayTexture::getDepth() const
{
    if(!m_Obj) throw TextureDataExc("Texture not initialized ...");
#if ! defined( KINSKI_GLES )
    if( m_Obj->m_Height == -1 )
    {
        bind();
        glGetTexLevelParameteriv( m_Obj->m_Target, 0, GL_TEXTURE_DEPTH, &m_Obj->m_Depth );
    }
#endif
    return m_Obj->m_Depth;
}
    
void ArrayTexture::bind( GLuint textureUnit ) const
{
    if(!m_Obj) throw TextureDataExc("Texture not initialized ...");
    m_Obj->m_boundTextureUnit = textureUnit;
	glActiveTexture( GL_TEXTURE0 + textureUnit );
	glBindTexture( m_Obj->m_Target, m_Obj->m_TextureID );
	glActiveTexture( GL_TEXTURE0 );
}

void ArrayTexture::unbind( GLuint textureUnit ) const
{
    if(!m_Obj) throw TextureDataExc("Texture not initialized ...");
    m_Obj->m_boundTextureUnit = -1;
	glActiveTexture( GL_TEXTURE0 + textureUnit );
	glBindTexture( m_Obj->m_Target, 0 );
	glActiveTexture( GL_TEXTURE0 );
}

void ArrayTexture::enableAndBind() const
{
	glEnable( m_Obj->m_Target );
	glBindTexture( m_Obj->m_Target, m_Obj->m_TextureID );
}

void ArrayTexture::disable() const
{
	glDisable( m_Obj->m_Target );
}

/////////////////////////////////////////////////////////////////////////////////

} // namespace gl
}
#endif