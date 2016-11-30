// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Buffer.hpp"
#include "Texture.hpp"

using namespace std;

namespace kinski{ namespace gl{
    
Texture create_texture_from_image(const ImagePtr& the_img, bool mipmap,
                                  bool compress, GLfloat anisotropic_filter_lvl)
{
    Texture ret;
    
    if(!the_img){ return ret; }
    GLenum format = 0, internal_format = 0;
    
    switch(the_img->bytes_per_pixel)
    {
#ifdef KINSKI_GLES
        case 1:
            internal_format = format = GL_LUMINANCE;
            break;
        case 2:
            internal_format = format = GL_LUMINANCE_ALPHA;
            break;
        case 3:
            internal_format = format = GL_RGB;
            // needs precompressed image and call to glCompressedTexImage2D
            //                internal_format = compress ? GL_ETC1_RGB8_OES : GL_RGB;
            break;
        case 4:
            internal_format = format = GL_RGBA;
        default:
            break;
#else
        case 1:
            format = GL_RED;
            internal_format = compress? GL_COMPRESSED_RED_RGTC1 : GL_RGBA;
            break;
        case 2:
            format = GL_RG;
            internal_format = compress? GL_COMPRESSED_RG_RGTC2 : GL_RGBA;
            break;
        case 3:
            format = GL_RGB;
            internal_format = compress? GL_COMPRESSED_RGB_S3TC_DXT1_EXT : GL_RGBA;
            break;
        case 4:
            format = GL_RGBA;
            internal_format = compress? GL_COMPRESSED_RGBA_S3TC_DXT5_EXT : GL_RGBA;
        default:
            break;
#endif
    }
    Texture::Format fmt;
    fmt.set_internal_format(internal_format);
    
    if(mipmap)
    {
        fmt.set_mipmapping();
        fmt.set_min_filter(GL_LINEAR_MIPMAP_NEAREST);
    }
    uint8_t *data = the_img->data;
    
#if !defined(KINSKI_GLES)
    gl::Buffer pixel_buf;
    pixel_buf.set_data(the_img->data, the_img->num_bytes());
    pixel_buf.bind(GL_PIXEL_UNPACK_BUFFER);
    data = nullptr;
#endif
    ret = Texture(data, format, the_img->width, the_img->height, fmt);
    ret.set_flipped();
    KINSKI_CHECK_GL_ERRORS();
    
    ret.set_anisotropic_filter(anisotropic_filter_lvl);
    return ret;
}

Texture create_texture_from_data(const std::vector<uint8_t> &the_data, bool mipmap,
                                 bool compress, GLfloat anisotropic_filter_lvl)
{
    ImagePtr img;
    Texture ret;
    try {img = create_image_from_data(the_data);}
    catch (ImageLoadException &e)
    {
        LOG_ERROR << e.what();
        return ret;
    }
    ret = create_texture_from_image(img, mipmap, compress, anisotropic_filter_lvl);
    return ret;
}

Texture create_texture_from_file(const std::string &theFileName, bool mipmap, bool compress,
                                 GLfloat anisotropic_filter_lvl)
{
    ImagePtr img = create_image_from_file(theFileName);
    Texture ret = create_texture_from_image(img);
    return ret;
}

ImagePtr create_image_from_texture(const gl::Texture &the_texture)
{
    ImagePtr ret;
    if(!the_texture){ return ret; }
#if !defined(KINSKI_GLES)
    ret = Image::create(the_texture.width(), the_texture.height(), 4);
    the_texture.bind();
    glGetTexImage(the_texture.target(), 0, GL_RGBA, GL_UNSIGNED_BYTE, ret->data);
    ret->flip();
#endif
    return ret;
}
    
/////////////////////////////////////////////////////////////////////////////////
// Texture::Format
    
Texture::Format::Format()
{
	m_target = GL_TEXTURE_2D;
    m_datatype = GL_UNSIGNED_BYTE;
	m_wrap_s = GL_CLAMP_TO_EDGE;//GL_REPEAT not working in ios
	m_wrap_t = GL_CLAMP_TO_EDGE;
	m_min_filter = GL_LINEAR;
	m_mag_filter = GL_LINEAR;
	m_mipmapping = false;
	m_internal_format = -1;
}

/////////////////////////////////////////////////////////////////////////////////
// Texture::Obj
    
struct TextureImpl
{
    TextureImpl() : m_width(-1), m_height(-1), m_depth(-1), m_internal_format(-1), m_datatype(-1),
    m_target(GL_TEXTURE_2D), m_texture_id(0), m_flipped(false), m_mip_map(false),
    m_deallocator_func(0) {};
    
    TextureImpl(int aWidth, int aHeight, int depth = 1) : m_width(aWidth), m_height(aHeight),
    m_depth(depth), m_internal_format(-1), m_datatype(-1), m_target(GL_TEXTURE_2D), m_texture_id(0),
    m_flipped(false), m_mip_map(false), m_bound_texture_unit(-1), m_deallocator_func(0){};
    
    ~TextureImpl()
    {
        if(m_deallocator_func){ (*m_deallocator_func)(m_deallocator_ref); }
        
        if(m_texture_id && !m_do_not_dispose){ glDeleteTextures(1, &m_texture_id); }
    }

    
    GLint m_width, m_height, m_depth;
    GLint m_internal_format;
    GLenum m_datatype;
    GLenum m_target;
    GLuint m_texture_id;
    
    bool m_do_not_dispose;
    bool m_flipped;
    bool m_mip_map;
    GLint m_bound_texture_unit;
    void (*m_deallocator_func)(void *refcon);
    void *m_deallocator_ref;
};

/////////////////////////////////////////////////////////////////////////////////
// Texture
    
Texture::Texture(int aWidth, int aHeight, Format format):
Texture(aWidth, aHeight, 1, format){}
    
Texture::Texture(int aWidth, int aHeight, int aDepth, Format format):
m_impl(new TextureImpl(aWidth, aHeight, aDepth))
{
    if(format.m_internal_format == -1){ format.m_internal_format = GL_RGBA; }
    m_impl->m_internal_format = format.m_internal_format;
    m_impl->m_target = format.m_target;
    m_impl->m_datatype = format.m_datatype;
    init(nullptr, GL_RGBA, format);
}

Texture::Texture(const unsigned char *data, int dataFormat, int aWidth, int aHeight, Format format)
    : Texture(data, dataFormat, aWidth, aHeight, 1, format){}
    
Texture::Texture(const unsigned char *data, int dataFormat, int aWidth, int aHeight, int aDepth,
                 Format format)
: m_impl(new TextureImpl(aWidth, aHeight, aDepth))
{
    if(format.m_internal_format == -1){ format.m_internal_format = GL_RGBA; }
    m_impl->m_internal_format = format.m_internal_format;
    m_impl->m_target = format.m_target;
    m_impl->m_datatype = format.m_datatype;
    init(data, dataFormat, format);
}

Texture::Texture(GLenum aTarget, GLuint aTextureID, int aWidth, int aHeight, bool aDoNotDispose)
    : Texture(aTarget, aTextureID, aWidth, aHeight, 1, aDoNotDispose){}
    
Texture::Texture(GLenum aTarget, GLuint aTextureID, int aWidth, int aHeight, int aDepth,
                 bool aDoNotDispose):
m_impl(new TextureImpl(aWidth, aHeight, aDepth))
{
    m_impl->m_target = aTarget;
    m_impl->m_texture_id = aTextureID;
    m_impl->m_internal_format = GL_RGBA;
    m_impl->m_datatype = GL_UNSIGNED_BYTE;
    m_impl->m_do_not_dispose = aDoNotDispose;
}
    
void Texture::init(const void *data, GLint dataFormat, const Format &format)
{
    m_impl->m_do_not_dispose = false;
    m_impl->m_datatype = format.m_datatype;
    
    if(!m_impl->m_texture_id)
    {
        glGenTextures(1, &m_impl->m_texture_id);
        glBindTexture(m_impl->m_target, m_impl->m_texture_id);
        glTexParameteri(m_impl->m_target, GL_TEXTURE_WRAP_S, format.m_wrap_s);
        glTexParameteri(m_impl->m_target, GL_TEXTURE_WRAP_T, format.m_wrap_t);
        glTexParameteri(m_impl->m_target, GL_TEXTURE_MIN_FILTER, format.m_min_filter);
        glTexParameteri(m_impl->m_target, GL_TEXTURE_MAG_FILTER, format.m_mag_filter);
    }
    else{ glBindTexture(m_impl->m_target, m_impl->m_texture_id); }
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
//#if ! defined(KINSKI_GLES)
//    glPixelStorei(GL_UNPACK_ROW_LENGTH, unpackRowLength);
//#endif
    
    if(m_impl->m_target == GL_TEXTURE_2D)
    {
        glTexImage2D(m_impl->m_target, 0, m_impl->m_internal_format,
                     m_impl->m_width, m_impl->m_height, 0, dataFormat,
                     m_impl->m_datatype, data);
    }
#if !defined(KINSKI_GLES)
    else if (m_impl->m_target == GL_TEXTURE_3D ||
             m_impl->m_target == GL_TEXTURE_2D_ARRAY)
    {
        glTexImage3D(m_impl->m_target, 0, m_impl->m_internal_format, m_impl->m_width, m_impl->m_height,
                     m_impl->m_depth, 0, dataFormat, m_impl->m_datatype, data);
    }
#endif
    
#if ! defined(KINSKI_GLES)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    
    if(format.m_mipmapping){ glGenerateMipmap(m_impl->m_target); }
    
#ifndef KINSKI_GLES
    if(dataFormat != GL_RGB && dataFormat != GL_RGBA && dataFormat != GL_BGRA)
    {
        GLint swizzleMask[] = {(GLint)(dataFormat), (GLint)(dataFormat), (GLint)(dataFormat),
            GL_ONE};
        glTexParameteriv(m_impl->m_target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
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
    
void Texture::update(const void *data, GLenum data_type, GLenum data_format, int theWidth, int theHeight,
                     bool flipped)
{
    if(!m_impl){ m_impl = std::make_shared<TextureImpl>(); }
    set_flipped(flipped);
    
    if(m_impl->m_width == theWidth && 
       m_impl->m_height == theHeight &&
       m_impl->m_datatype == data_type)
    {
        glBindTexture(m_impl->m_target, m_impl->m_texture_id);
        glTexSubImage2D(m_impl->m_target, 0, 0, 0, m_impl->m_width, m_impl->m_height, data_format, data_type, data);
    }
    else 
    {
        m_impl->m_datatype = data_type;
        m_impl->m_internal_format = GL_RGBA;
        m_impl->m_width = theWidth;
        m_impl->m_height = theHeight;
        Format f;
        f.m_datatype = data_type;
        init(data, data_format, f);
    }
}
    
bool Texture::data_format_has_alpha(GLint dataFormat)
{
	switch(dataFormat)
    {
		case GL_RGBA:
		case GL_ALPHA:
#if ! defined(KINSKI_GLES)
		case GL_BGRA:
#endif
			return true;
		break;
		default:
			return false;
	}
}

bool Texture::data_format_has_color(GLint the_data_format)
{
	switch(the_data_format)
    {
		case GL_ALPHA:
        //case GL_LUMINANCE:
			return false;
		break;
	}
	
	return true;
}

Texture	Texture::weak_clone() const
{
	gl::Texture result = Texture(m_impl->m_target, m_impl->m_texture_id, m_impl->m_width, m_impl->m_height, true);
	result.m_impl->m_internal_format = m_impl->m_internal_format;
	result.m_impl->m_flipped = m_impl->m_flipped;	
	return result;
}

void Texture::set_deallocator(void(*aDeallocatorFunc)(void *), void *aDeallocatorRefcon)
{
	m_impl->m_deallocator_func = aDeallocatorFunc;
	m_impl->m_deallocator_ref = aDeallocatorRefcon;
}

void Texture::set_do_not_dispose(bool the_do_not_dispose)
{ 
    m_impl->m_do_not_dispose = the_do_not_dispose; 
}

void Texture::set_texture_matrix(const mat4 &theMatrix)
{
    m_textureMatrix = theMatrix;
}
    
void Texture::set_swizzle(GLint red, GLint green, GLint blue, GLint alpha)
{
#if !defined(KINSKI_GLES)
    bind();
    GLint swizzleMask[] = {red, green, blue, alpha};
    glTexParameteriv(target(), GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
#endif
}

void Texture::set_roi(int the_x, int the_y, uint32_t the_width, uint32_t the_height)
{
    int y_inv = height() - the_height - the_y;
    vec3 offset = vec3((float)the_x / width(), (float)y_inv / height(), 0);
    vec3 sc = vec3((float)the_width / width(), (float)the_height / height(), 1);
    m_textureMatrix = translate(mat4(), offset) * scale(mat4(), sc);
}
    
void Texture::set_roi(const Area_<uint32_t> &the_roi)
{
    set_roi(the_roi.x0, the_roi.y0, the_roi.width(), the_roi.height());
}
    
mat4 Texture::texture_matrix() const
{
    mat4 ret = m_textureMatrix;
    
    if(m_impl && m_impl->m_flipped)
    {
        static const mat4 flipY = mat4(vec4(1, 0, 0, 1),
                                       vec4(0, -1, 0, 1),// invert y-coords
                                       vec4(0, 0, 1, 1),
                                       vec4(0, 1, 0, 1));// [0, -1] -> [1, 0]
        ret = flipY * ret;
    }
    return ret;
}

const bool Texture::is_bound() const
{
    return m_impl->m_bound_texture_unit >= 0;
}

const GLint Texture::bound_texture_unit() const
{
    return m_impl ? m_impl->m_bound_texture_unit : -1;
}
    
GLuint Texture::id() const
{ 
    return m_impl ? m_impl->m_texture_id : 0;
}

GLenum Texture::target() const
{ 
    return m_impl ? m_impl->m_target : 0;
}

//!	whether the texture is flipped vertically
bool Texture::flipped() const
{
    return m_impl && m_impl->m_flipped;
}

//!	Marks the texture as being flipped vertically or not
void Texture::set_flipped(bool the_flipped)
{
    if(m_impl){ m_impl->m_flipped = the_flipped; }
}
    
void Texture::set_wrap_s(GLenum the_warp_s)
{
    if(m_impl){ glTexParameteri(m_impl->m_target, GL_TEXTURE_WRAP_S, the_warp_s); }
}

void Texture::set_wrap_t(GLenum the_warp_t)
{
    if(m_impl){ glTexParameteri(m_impl->m_target, GL_TEXTURE_WRAP_T, the_warp_t); }
}

void Texture::set_min_filter(GLenum minFilter)
{
    if(m_impl){ glTexParameteri(m_impl->m_target, GL_TEXTURE_MIN_FILTER, minFilter); }
}

void Texture::set_mag_filter(GLenum magFilter)
{
    if(m_impl){ glTexParameteri(m_impl->m_target, GL_TEXTURE_MAG_FILTER, magFilter); };
}

void Texture::set_mipmapping(bool b)
{
    if(m_impl)
    {
        if(b && !m_impl->m_mip_map)
        {
            set_min_filter(GL_LINEAR_MIPMAP_LINEAR);
            glGenerateMipmap(m_impl->m_target);
        }
        else{ set_min_filter(GL_LINEAR); }
        m_impl->m_mip_map = b;
    }
}

void Texture::set_anisotropic_filter(float f)
{
    if(gl::is_extension_supported("GL_EXT_texture_filter_anisotropic"))
    {
        GLfloat fLargest;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
        bind();
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min(fLargest, f));
        unbind();
    }
}
    
bool Texture::has_alpha() const
{
    if(!m_impl) throw TextureDataExc("Texture not initialized ...");
    
	switch(m_impl->m_internal_format)
    {
#if ! defined(KINSKI_GLES)
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

GLint Texture::internal_format() const
{
    if(!m_impl) throw TextureDataExc("Texture not initialized ...");
#if ! defined(KINSKI_GLES)
	if(m_impl->m_internal_format == -1)
    {
		bind();
		glGetTexLevelParameteriv(m_impl->m_target, 0, GL_TEXTURE_INTERNAL_FORMAT, &m_impl->m_internal_format);
	}
#endif // ! defined(KINSKI_GLES)
	
	return m_impl->m_internal_format;
}

GLint Texture::width() const
{
    if(!m_impl){ return 0; };
    
#if ! defined(KINSKI_GLES)
	if(m_impl->m_width == -1)
    {
		bind();
		glGetTexLevelParameteriv(m_impl->m_target, 0, GL_TEXTURE_WIDTH, &m_impl->m_width);
	}
#endif
	return m_impl->m_width;
}

GLint Texture::height() const
{
    if(!m_impl){ return 0; };
    
#if ! defined(KINSKI_GLES)
	if(m_impl->m_height == -1)
    {
		bind();
		glGetTexLevelParameteriv(m_impl->m_target, 0, GL_TEXTURE_HEIGHT, &m_impl->m_height);	
	}
#endif
	return m_impl->m_height;
}
    
GLint Texture::depth() const
{
    if(!m_impl){ return 0; };
    
#if ! defined(KINSKI_GLES)
    if(m_impl->m_depth == -1)
    {
        bind();
        glGetTexLevelParameteriv(m_impl->m_target, 0, GL_TEXTURE_DEPTH, &m_impl->m_depth);
    }
#endif
    return m_impl->m_depth;
}

void Texture::bind(GLuint textureUnit) const
{
    if(!m_impl) throw TextureDataExc("Texture not initialized ...");
    m_impl->m_bound_texture_unit = textureUnit;
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(m_impl->m_target, m_impl->m_texture_id);
	glActiveTexture(GL_TEXTURE0);
}

void Texture::unbind(GLuint textureUnit) const
{
    if(!m_impl) throw TextureDataExc("Texture not initialized ...");
    m_impl->m_bound_texture_unit = -1;
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(m_impl->m_target, 0);
	glActiveTexture(GL_TEXTURE0);
}

void Texture::enable_and_bind() const
{
	glEnable(m_impl->m_target);
	glBindTexture(m_impl->m_target, m_impl->m_texture_id);
}

void Texture::disable() const
{
	glDisable(m_impl->m_target);
}

/////////////////////////////////////////////////////////////////////////////////

} // namespace gl
}
