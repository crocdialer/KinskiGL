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
	m_WrapS = GL_CLAMP_TO_EDGE;//GL_REPEAT not working in ios
	m_WrapT = GL_CLAMP_TO_EDGE;
	m_MinFilter = GL_LINEAR;
	m_MagFilter = GL_LINEAR;
	m_Mipmapping = false;
	m_InternalFormat = -1;
}

/////////////////////////////////////////////////////////////////////////////////
// Texture::Obj
    
struct Texture::Obj
{
    Obj() : m_Width( -1 ), m_Height( -1 ), m_depth(-1), m_InternalFormat( -1 ), m_dataType(-1),
    m_Target(GL_TEXTURE_2D), m_TextureID(0), m_Flipped(false), m_mip_map(false),
    m_DeallocatorFunc(0) {};
    
    Obj( int aWidth, int aHeight, int depth = 1 ) : m_Width(aWidth), m_Height(aHeight),
    m_depth(depth), m_InternalFormat( -1 ), m_dataType(-1), m_Target(GL_TEXTURE_2D), m_TextureID(0),
    m_Flipped(false), m_mip_map(false), m_boundTextureUnit(-1), m_DeallocatorFunc(0){};
    
    ~Obj()
    {
        if( m_DeallocatorFunc )
            (*m_DeallocatorFunc)( m_DeallocatorRefcon );
        
        if( (m_TextureID > 0) && (!m_DoNotDispose))
        {
            glDeleteTextures(1, &m_TextureID);
        }
    }

    
    GLint m_Width, m_Height, m_depth;
    GLint m_InternalFormat;
    GLenum m_dataType;
    GLenum m_Target;
    GLuint m_TextureID;
    
    bool m_DoNotDispose;
    bool m_Flipped;
    bool m_mip_map;
    GLint m_boundTextureUnit;
    void (*m_DeallocatorFunc)(void *refcon);
    void *m_DeallocatorRefcon;
};

/////////////////////////////////////////////////////////////////////////////////
// Texture
    
Texture::Texture( int aWidth, int aHeight, Format format ):
Texture(aWidth, aHeight, 1, format){}
    
Texture::Texture(int aWidth, int aHeight, int aDepth, Format format):
m_Obj(new Obj(aWidth, aHeight, aDepth))
{
    if( format.m_InternalFormat == -1 ){ format.m_InternalFormat = GL_RGBA; }
    m_Obj->m_InternalFormat = format.m_InternalFormat;
    m_Obj->m_Target = format.m_Target;
//    m_Obj->m_dataType = GL_UNSIGNED_BYTE;
    init(nullptr, GL_UNSIGNED_BYTE, GL_RGBA, format);
}

Texture::Texture(const unsigned char *data, int dataFormat, int aWidth, int aHeight, Format format)
    : Texture(data, dataFormat, aWidth, aHeight, 1, format){}
    
Texture::Texture(const unsigned char *data, int dataFormat, int aWidth, int aHeight, int aDepth,
                 Format format)
: m_Obj(new Obj(aWidth, aHeight, aDepth))
{
    if( format.m_InternalFormat == -1 ){ format.m_InternalFormat = GL_RGBA; }
    m_Obj->m_InternalFormat = format.m_InternalFormat;
    m_Obj->m_Target = format.m_Target;
    m_Obj->m_dataType = GL_UNSIGNED_BYTE;
    init(data, GL_UNSIGNED_BYTE, dataFormat, format);
}

Texture::Texture(GLenum aTarget, GLuint aTextureID, int aWidth, int aHeight, bool aDoNotDispose)
    : Texture(aTarget, aTextureID, aWidth, aHeight, 1, aDoNotDispose){}
    
Texture::Texture(GLenum aTarget, GLuint aTextureID, int aWidth, int aHeight, int aDepth,
                 bool aDoNotDispose):
m_Obj(new Obj(aWidth, aHeight, aDepth))
{
    m_Obj->m_Target = aTarget;
    m_Obj->m_TextureID = aTextureID;
    m_Obj->m_InternalFormat = GL_RGBA;
    m_Obj->m_dataType = GL_UNSIGNED_BYTE;
    m_Obj->m_DoNotDispose = aDoNotDispose;
}
    
void Texture::init(const void *data, GLenum dataType, GLint dataFormat, const Format &format)
{
    m_Obj->m_DoNotDispose = false;
    m_Obj->m_dataType = dataType;
    
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
    
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    
//#if ! defined( KINSKI_GLES )
//    glPixelStorei( GL_UNPACK_ROW_LENGTH, unpackRowLength );
//#endif
    
    if(m_Obj->m_Target == GL_TEXTURE_2D)
    {
        glTexImage2D(m_Obj->m_Target, 0, m_Obj->m_InternalFormat,
                     m_Obj->m_Width, m_Obj->m_Height, 0, dataFormat,
                     dataType, data );
    }
#if !defined(KINSKI_GLES)
    else if (m_Obj->m_Target == GL_TEXTURE_3D ||
             m_Obj->m_Target == GL_TEXTURE_2D_ARRAY)
    {
        glTexImage3D(m_Obj->m_Target, 0, m_Obj->m_InternalFormat, m_Obj->m_Width, m_Obj->m_Height,
                     m_Obj->m_depth, 0, dataFormat, dataType, data);
    }
#endif
    
#if ! defined( KINSKI_GLES )
    glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
#endif
    
    if( format.m_Mipmapping ){ glGenerateMipmap(m_Obj->m_Target); }
    
#ifndef KINSKI_GLES
    if(dataFormat != GL_RGB && dataFormat != GL_RGBA && dataFormat != GL_BGRA)
    {
        GLint swizzleMask[] = {(GLint)(dataFormat), (GLint)(dataFormat), (GLint)(dataFormat),
            GL_ONE};
        glTexParameteriv(m_Obj->m_Target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    }
#endif
    
    KINSKI_CHECK_GL_ERRORS();
}

void Texture::update(const uint8_t *data,GLenum format, int theWidth, int theHeight, bool flipped)
{   
    update(data, GL_UNSIGNED_BYTE, format, theWidth, theHeight, flipped);
}
    
void Texture::update(const float *data,GLenum format, int theWidth, int theHeight, bool flipped)
{
    update(data, GL_FLOAT, format, theWidth, theHeight, flipped);
}
    
void Texture::update(const void *data, GLenum dataType, GLenum format, int theWidth, int theHeight,
                     bool flipped)
{
    if(!m_Obj){ m_Obj = ObjPtr(new Obj()); }
    setFlipped(flipped);
    
    if(m_Obj->m_Width == theWidth && 
       m_Obj->m_Height == theHeight &&
       m_Obj->m_dataType == dataType)
    {
        glBindTexture( m_Obj->m_Target, m_Obj->m_TextureID );
        glTexSubImage2D( m_Obj->m_Target, 0, 0, 0, m_Obj->m_Width, m_Obj->m_Height, format, dataType, data );
    }
    else 
    {
        m_Obj->m_dataType = dataType;
        m_Obj->m_InternalFormat = GL_RGBA;
        m_Obj->m_Width = theWidth;
        m_Obj->m_Height = theHeight;
        Format f;
        init(data, dataType, format, f);
    }
}
    
bool Texture::dataFormatHasAlpha( GLint dataFormat )
{
	switch( dataFormat )
    {
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

void Texture::setTextureMatrix( const mat4 &theMatrix )
{
    m_textureMatrix = theMatrix;
}
    
void Texture::set_swizzle(GLint red, GLint green, GLint blue, GLint alpha)
{
#if !defined(KINSKI_GLES)
    GLint swizzleMask[] = {red, green, blue, alpha};
    glTexParameteriv(getTarget(), GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
#endif
}

void Texture::set_roi(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    // clamp values
    x = clamp<int>(x, 0, getWidth() - 1);
    y = clamp<int>(y, 0, getHeight() - 1);
    width = clamp<int>(width, 0, getWidth() - x);
    height = clamp<int>(height, 0, getHeight() - y);
    
    vec3 offset = vec3((float)x / getWidth(),
                                 (float)y / getHeight(), 0);
    vec3 sc = vec3((float)width / getWidth(),
                                (float)height / getHeight(), 1);
    
    m_textureMatrix = translate(mat4(), offset) * scale(mat4(), sc);
}
    
void Texture::set_roi(const Area<uint32_t> &the_roi)
{
    set_roi(the_roi.x1, the_roi.y1, the_roi.width(), the_roi.height());
}
    
mat4 Texture::getTextureMatrix() const 
{
    mat4 ret = m_textureMatrix;
    
    if(m_Obj->m_Flipped)
    {
        static mat4 flipY = mat4(vec4(1, 0, 0, 1),
                                 vec4(0, -1, 0, 1),// invert y-coords
                                 vec4(0, 0, 1, 1),
                                 vec4(0, 1, 0, 1));// [0, -1] -> [1, 0]
        ret = flipY * ret;
    }
    return ret;
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

void Texture::set_mipmapping(bool b)
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

void Texture::set_anisotropic_filter(float f)
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
    
bool Texture::hasAlpha() const
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
	
float Texture::getLeft() const
{
	return (m_textureMatrix * vec4(0, 0, 0, 1)).x;
}

float Texture::getRight() const
{
	return (m_textureMatrix * vec4(1, 0, 0, 1)).x;
}

float Texture::getTop() const
{
	return (m_textureMatrix * vec4(0, 1, 0, 1)).y;
}
    
float Texture::getBottom() const
{
    return (m_textureMatrix * vec4(0, 0, 0, 1)).y;
}

GLint Texture::getInternalFormat() const
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

GLint Texture::getWidth() const
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

GLint Texture::getHeight() const
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
    
GLint Texture::getDepth() const
{
    if(!m_Obj) throw TextureDataExc("Texture not initialized ...");
#if ! defined( KINSKI_GLES )
    if( m_Obj->m_depth == -1 )
    {
        bind();
        glGetTexLevelParameteriv( m_Obj->m_Target, 0, GL_TEXTURE_DEPTH, &m_Obj->m_depth );
    }
#endif
    return m_Obj->m_depth;
}

void Texture::bind( GLuint textureUnit ) const
{
    if(!m_Obj) throw TextureDataExc("Texture not initialized ...");
    m_Obj->m_boundTextureUnit = textureUnit;
	glActiveTexture( GL_TEXTURE0 + textureUnit );
	glBindTexture( m_Obj->m_Target, m_Obj->m_TextureID );
	glActiveTexture( GL_TEXTURE0 );
}

void Texture::unbind( GLuint textureUnit ) const
{
    if(!m_Obj) throw TextureDataExc("Texture not initialized ...");
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