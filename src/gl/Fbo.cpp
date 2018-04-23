// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schm_idt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Fbo.hpp"

using namespace std;

namespace kinski { namespace gl {

////////////////////////////////////////////////////////////////////////////////////////////////////

gl::Fbo create_cube_framebuffer(uint32_t the_width, bool with_color_buffer)
{
    uint32_t cube_sz = the_width;
    gl::Fbo::Format fbo_fmt;
    fbo_fmt.set_num_color_buffers(0);
    gl::Fbo fbo(cube_sz, cube_sz, fbo_fmt);
    
    gl::Texture::Format cube_depth_fmt;
    cube_depth_fmt.set_target(GL_TEXTURE_CUBE_MAP);
    cube_depth_fmt.set_data_type(GL_FLOAT);
    cube_depth_fmt.set_internal_format(GL_DEPTH_COMPONENT32F);
    cube_depth_fmt.set_min_filter(GL_NEAREST);
    cube_depth_fmt.set_mag_filter(GL_NEAREST);
    auto cube_depth_tex = gl::Texture(nullptr, GL_DEPTH_COMPONENT, cube_sz, cube_sz, cube_depth_fmt);
    fbo.set_depth_texture(cube_depth_tex);

    if(with_color_buffer)
    {
        GLenum format, internal_format;
        gl::get_texture_format(3, false, &format, &internal_format);
        gl::Texture::Format cube_fmt;
        cube_fmt.set_target(GL_TEXTURE_CUBE_MAP);
        cube_fmt.set_internal_format(internal_format);
        cube_fmt.set_min_filter(GL_NEAREST);
        cube_fmt.set_mag_filter(GL_NEAREST);
        auto cube_tex = gl::Texture(nullptr, format, cube_sz, cube_sz, cube_fmt);
        fbo.add_attachment(cube_tex, 0);
    }
    return fbo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
    
ImagePtr create_image_from_framebuffer(gl::Fbo the_fbo)
{
    gl::SaveFramebufferBinding sfb;
    ImagePtr ret;
    int w = gl::window_dimension().x, h = gl::window_dimension().y, num_comp = 3;
    GLenum format = GL_RGB;
    
    if(the_fbo)
    {
        the_fbo.bind();
        w = the_fbo.width();
        h = the_fbo.height();
        num_comp = 4;
        format = GL_RGBA;
    }
    ret = Image::create(w, h, num_comp);
    ret->m_type = Image::Type::RGBA;
    glReadPixels(0, 0, w, h, format, GL_UNSIGNED_BYTE, ret->data);
    ret->flip();
    return ret;
}
    
GLint Fbo::sMaxSamples = -1;
GLint Fbo::sMaxAttachments = -1;

////////////////////////////////////////////////////////////////////////////////////////////////////
// RenderBuffer::Obj
    
    struct RenderbufferImpl
    {
        RenderbufferImpl():
        m_width(-1),
        m_height(-1),
        m_id(0),
        m_internal_format(0),
        m_num_samples(0){}
        
        RenderbufferImpl(int the_width, int the_height, GLenum the_internal_format,
                         int the_num_samples):
        m_width(the_width),
        m_height(the_height),
        m_internal_format(the_internal_format),
        m_num_samples(the_num_samples)
        {
            glGenRenderbuffers(1, &m_id);
            
            if(m_num_samples > Fbo::max_num_samples())
                m_num_samples = Fbo::max_num_samples();
            
            glBindRenderbuffer(GL_RENDERBUFFER, m_id);
            
#if !defined(KINSKI_GLES_2)
            if(m_num_samples) // create a regular MSAA buffer
                glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_num_samples, m_internal_format, m_width, m_height);
            else
#endif
                glRenderbufferStorage(GL_RENDERBUFFER, m_internal_format, m_width, m_height);
        }
        
        ~RenderbufferImpl(){ if(m_id){ glDeleteRenderbuffers(1, &m_id); } }
        
        int m_width, m_height;
        GLuint m_id;
        GLenum m_internal_format;
        int m_num_samples;
    };

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

Renderbuffer::Renderbuffer(int width, int height, GLenum internalFormat)
	: m_impl(new RenderbufferImpl(width, height, internalFormat, 0))
{}
    
Renderbuffer::Renderbuffer(int width, int height, GLenum internalFormat, int msaaSamples)
	: m_impl(new RenderbufferImpl(width, height, internalFormat, msaaSamples))
{}

////////////////////////////////////////////////////////////////////////////////////////////////////
    
int Renderbuffer::width() const { return m_impl->m_width; }

////////////////////////////////////////////////////////////////////////////////////////////////////
    
int Renderbuffer::height() const { return m_impl->m_height; }
    
////////////////////////////////////////////////////////////////////////////////////////////////////

ivec2 Renderbuffer::size() const { return ivec2(m_impl->m_width, m_impl->m_height); }
    
////////////////////////////////////////////////////////////////////////////////////////////////////

float Renderbuffer::aspect_ratio() const { return m_impl->m_width / (float)m_impl->m_height; }
    
////////////////////////////////////////////////////////////////////////////////////////////////////

GLuint	Renderbuffer::id() const { return m_impl->m_id; }
    
////////////////////////////////////////////////////////////////////////////////////////////////////

GLenum	Renderbuffer::internal_format() const { return m_impl->m_internal_format; }
    
////////////////////////////////////////////////////////////////////////////////////////////////////
    
int Renderbuffer::num_samples() const { return m_impl->m_num_samples; }

////////////////////////////////////////////////////////////////////////////////////////////////////
// Fbo::Obj
    
struct FboImpl
{
    FboImpl():m_id(0), m_resolve_fbo_id(0){}
    
    FboImpl(int the_width, int the_height):
    m_width(the_width),
    m_height(the_height),
    m_id(0), m_resolve_fbo_id(0){}
    
    ~FboImpl()
    {
        if(m_id){ glDeleteFramebuffers(1, &m_id); }
        if(m_resolve_fbo_id){ glDeleteFramebuffers(1, &m_resolve_fbo_id); }
    }
    
    int m_width, m_height;
    Fbo::Format m_format;
    GLuint m_id;
    GLuint m_resolve_fbo_id;
    std::vector<Renderbuffer> mMultisampleColorRenderbuffers;
    Renderbuffer m_multisample_depth_renderbuffer;
    std::vector<Texture> m_color_textures;
    Texture m_depth_texture;
    Renderbuffer m_depthRenderbuffer;
    mutable bool m_needs_resolve, m_needs_mipmap_update;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fbo::Format
Fbo::Format::Format()
{
	m_target = GL_TEXTURE_2D;
#if defined(KINSKI_GLES_2)
	m_color_internal_format = GL_RGBA;
	m_depth_internal_format = GL_ENUM(GL_DEPTH_COMPONENT24);
    m_stencil_internal_format = GL_STENCIL_INDEX8;
	m_depth_buffer_texture = false;
#else
	m_color_internal_format = GL_RGBA8;
    m_depth_data_type = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
    m_depth_internal_format = GL_DEPTH32F_STENCIL8;//GL_DEPTH_COMPONENT32F; //GL_DEPTH24_STENCIL8;
	m_depth_buffer_texture = true;
//#if defined(KINSKI_GLES_3)
//    m_depth_data_type = GL_UNSIGNED_INT_24_8;//GL_DEPTH24_STENCIL8;
//    m_depth_internal_format = GL_DEPTH_COMPONENT24;
//    m_depth_buffer_texture = true;
//#endif
    
#endif
	m_num_samples = 0;
	m_num_coverage_samples = 0;
	m_num_color_buffers = 1;
	m_depth_buffer = true;
	m_stencil_buffer = false;
	m_mipmapping = false;
	m_wrap_s = GL_CLAMP_TO_EDGE;
	m_wrap_t = GL_CLAMP_TO_EDGE;
	m_min_filter = GL_LINEAR;
	m_mag_filter = GL_LINEAR;
}

void Fbo::Format::enable_color_buffer(bool the_color_buffer, int the_num_buffers)
{
#if !defined(KINSKI_GLES_2)
	m_num_color_buffers = (the_color_buffer && m_num_color_buffers) ? 1 : 0;
#else
	m_num_color_buffers = the_color_buffer ? m_num_color_buffers : 0;
#endif
}

void Fbo::Format::enable_depth_buffer(bool the_depth_buffer, bool as_texture)
{
	m_depth_buffer = the_depth_buffer;
#if defined(KINSKI_GLES_2)
	m_depth_buffer_texture = false;
#else
	m_depth_buffer_texture = as_texture;
#endif
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fbo


void Fbo::init()
{
    SaveFramebufferBinding sfb;

	bool useAA = m_impl->m_format.m_num_samples > 0;

	// allocate the framebuffer itself
	glGenFramebuffers(1, &m_impl->m_id);
    glBindFramebuffer(GL_FRAMEBUFFER, m_impl->m_id);

	Texture::Format textureFormat;
	textureFormat.set_target(target());
    auto col_fmt = format().color_internal_format();
	textureFormat.set_internal_format(col_fmt);
    
#if !defined(KINSKI_GLES_2)
    GLint float_types[] = {GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F};
    GLint one_comp_types[] = {GL_RED, GL_GREEN, GL_BLUE, GL_R32F};
    if(contains(float_types, col_fmt)){ textureFormat.set_data_type(GL_FLOAT); }
#endif
	textureFormat.set_wrap(m_impl->m_format.m_wrap_s, m_impl->m_format.m_wrap_t);
	textureFormat.set_min_filter(m_impl->m_format.m_min_filter);
	textureFormat.set_mag_filter(m_impl->m_format.m_mag_filter);
	textureFormat.set_mipmapping(format().has_mipmapping());

	// allocate the color buffers
	for(uint32_t c = 0; c < m_impl->m_format.m_num_color_buffers; ++c)
    {
        auto tex = Texture(m_impl->m_width, m_impl->m_height, textureFormat);
        
#if !defined(KINSKI_GLES_2)
        if(contains(one_comp_types, col_fmt)){ tex.set_swizzle(GL_RED, GL_RED, GL_RED, GL_ONE); }
#endif
		m_impl->m_color_textures.push_back(tex);
	}
	
#if !defined(KINSKI_GLES)
	if(m_impl->m_format.m_num_color_buffers == 0)
    {
        // no color
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	}
#endif
    
    if(useAA){ useAA = init_multisample(); }
    
	if(!useAA)
    {
        // if we don't need any variety of multisampling or it failed to initialize
		// attach all the textures to the framebuffer
		vector<GLenum> drawBuffers;
		for(size_t c = 0; c < m_impl->m_color_textures.size(); ++c)
        {
#if defined(KINSKI_GLES_2)
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + c, target()
                                 m_impl->m_color_textures[c].id(), 0);
#endif
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + c,
                                   m_impl->m_color_textures[c].id(), 0);
			drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + c);
		}
#if !defined(KINSKI_GLES_2)
        if(!drawBuffers.empty()){ glDrawBuffers(drawBuffers.size(), drawBuffers.data()); }
#endif

		// allocate and attach depth texture
		if(m_impl->m_format.m_depth_buffer)
        {
			if(m_impl->m_format.m_depth_buffer_texture)
            {
#if !defined(KINSKI_GLES_2)
				GLuint depthTextureId;
				glGenTextures(1, &depthTextureId);
				glBindTexture(target(), depthTextureId);
				glTexImage2D(target(), 0, format().depth_internal_format(),
                             m_impl->m_width, m_impl->m_height, 0, GL_DEPTH_STENCIL,
                             format().m_depth_data_type, nullptr);
                
				m_impl->m_depth_texture = Texture(target(), depthTextureId, m_impl->m_width,
                                                  m_impl->m_height, false);
                m_impl->m_depth_texture.set_min_filter(m_impl->m_format.m_min_filter);
                m_impl->m_depth_texture.set_mag_filter(m_impl->m_format.m_mag_filter);
                m_impl->m_depth_texture.set_wrap_s(m_impl->m_format.m_wrap_s);
                m_impl->m_depth_texture.set_wrap_t(m_impl->m_format.m_wrap_t);
                
                auto attach = m_impl->m_format.has_stencil_buffer() ?
                              GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
                
#if defined(KINSKI_GLES_3)
                glFramebufferTexture2D(GL_FRAMEBUFFER, attach, target(),
                                       m_impl->m_depth_texture.id(), 0);
#else
                glFramebufferTexture(GL_FRAMEBUFFER, attach, m_impl->m_depth_texture.id(), 0);
#endif
                
#endif//KINSKI_GLES_2
			}
            // implement depth buffer as RenderBuffer
			else if(m_impl->m_format.m_depth_buffer)
            {
				m_impl->m_depthRenderbuffer = Renderbuffer(m_impl->m_width, m_impl->m_height,
                                                          m_impl->m_format.depth_internal_format());
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                          m_impl->m_depthRenderbuffer.id());
                
                if(m_impl->m_format.has_stencil_buffer())
                {
                    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                              GL_RENDERBUFFER, m_impl->m_depthRenderbuffer.id());
                }
			}
		}

		FboExceptionInvalidSpecification exc;
        // failed creation; throw
		if(!check_status(&exc)) { LOG_ERROR << exc.what(); }
	}
	
	m_impl->m_needs_resolve = false;
	m_impl->m_needs_mipmap_update = false;
    
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool Fbo::init_multisample()
{
#if defined(KINSKI_GLES_2)
	return false;
#else
	glGenFramebuffers(1, &m_impl->m_resolve_fbo_id);
	glBindFramebuffer(GL_FRAMEBUFFER, m_impl->m_resolve_fbo_id);
	
	// bind all of the color buffers to the resolve FB's attachment points
	vector<GLenum> drawBuffers;
	for(size_t c = 0; c < m_impl->m_color_textures.size(); ++c)
    {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + c, target(),
                               m_impl->m_color_textures[c].id(), 0);
		drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + c);
	}

    if(!drawBuffers.empty()){ glDrawBuffers(drawBuffers.size(), drawBuffers.data()); }

	// see if the resolve buffer is ok
	FboExceptionInvalidSpecification ignoredException;
    if(!check_status(&ignoredException)){ return false; }

	glBindFramebuffer(GL_FRAMEBUFFER, m_impl->m_id);

	m_impl->m_format.m_num_samples = std::min(m_impl->m_format.m_num_samples, max_num_samples());
    
	// setup the multisampled color renderbuffers
	for(uint32_t c = 0; c < m_impl->m_format.m_num_color_buffers; ++c)
    {
		m_impl->mMultisampleColorRenderbuffers.push_back(Renderbuffer(m_impl->m_width, m_impl->m_height,
                                                                      m_impl->m_format.m_color_internal_format,
                                                                      m_impl->m_format.m_num_samples));

		// attach the multisampled color buffer
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + c, GL_RENDERBUFFER,
                                  m_impl->mMultisampleColorRenderbuffers.back().id());
	}

	if(m_impl->m_format.m_depth_buffer)
    {
		// create the multisampled depth Renderbuffer
		m_impl->m_multisample_depth_renderbuffer = Renderbuffer(m_impl->m_width, m_impl->m_height,
                                                            m_impl->m_format.m_depth_internal_format,
                                                            m_impl->m_format.m_num_samples);

		// attach the depth Renderbuffer
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                  m_impl->m_multisample_depth_renderbuffer.id());
        
        if(m_impl->m_format.has_stencil_buffer())
        {
            // attach the depth Renderbuffer
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                      m_impl->m_multisample_depth_renderbuffer.id());
        }
	}

	// see if the primary framebuffer turned out ok
	return check_status(&ignoredException);
#endif // ! KINSKI_GLES
}

Fbo::Fbo(const glm::vec2 &the_size, Format format): Fbo(the_size.x, the_size.y, format){}
    
Fbo::Fbo(int width, int height, Format format):
m_impl(new FboImpl(width, height))
{
	m_impl->m_format = format;
	init();
}

Fbo::Fbo(int width, int height, bool alpha, bool color, bool depth):
m_impl(new FboImpl(width, height))
{
	Format format;
	m_impl->m_format.m_color_internal_format = (alpha) ? GL_ENUM(GL_RGBA8) : GL_ENUM(GL_RGB8);
	m_impl->m_format.m_depth_buffer = depth;
	m_impl->m_format.m_num_color_buffers = color ? 1 : 0;
	init();
}

GLuint Fbo::id() const { return m_impl->m_id; }
    
int Fbo::width() const{ return m_impl->m_width; }
    
int Fbo::height() const{ return m_impl->m_height; }

ivec2 Fbo::size() const{ return ivec2( m_impl->m_width, m_impl->m_height ); }
    
float Fbo::aspect_ratio() const{ return m_impl->m_width / (float)m_impl->m_height; }

const Fbo::Format& Fbo::format() const { return m_impl->m_format; }
    
GLenum Fbo::target() const { return m_impl->m_format.m_target; }

void Fbo::enable_draw_buffers(bool b)
{
	if(!m_impl){ return; }
#if !defined(KINSKI_GLES)
	SaveFramebufferBinding sfb;
	bind();

	if(b)
	{
		vector<GLenum> drawBuffers;
		for(size_t c = 0; c < m_impl->m_color_textures.size(); ++c)
		{
			drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + c);
		}

		if(!drawBuffers.empty()){ glDrawBuffers(drawBuffers.size(), drawBuffers.data()); }
	}
	else{ glDrawBuffer(GL_NONE); }
#endif
}

void Fbo::add_attachment(gl::Texture the_attachment, int the_index)
{
	if(!m_impl){ return; }
    uint32_t index = the_index <= 0 ? m_impl->m_color_textures.size() : the_index;

    SaveFramebufferBinding sfb;
    glBindFramebuffer(GL_FRAMEBUFFER, m_impl->m_id);
#if defined(KINSKI_GLES_2)
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, target(),
                           the_attachment.id(), 0);
#else
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, the_attachment.id(), 0);
#endif

    FboExceptionInvalidSpecification exc;

    if(check_status(&exc))
    {
    	if(the_index >= 0)
    	{
            m_impl->m_color_textures.resize(std::max<int>(m_impl->m_color_textures.size(), the_index + 1));
    	    m_impl->m_color_textures[the_index] = the_attachment;
    	}
        else{ m_impl->m_color_textures.push_back(the_attachment); }
        m_impl->m_format.set_num_color_buffers(m_impl->m_color_textures.size());
        enable_draw_buffers(true);
    }
    else{ LOG_ERROR << exc.what(); }
}

Texture Fbo::texture(int attachment)
{
	if(m_impl)
	{
		resolve_textures();
		update_mipmaps(true, attachment);
		return m_impl->m_color_textures.empty() ? gl::Texture() : m_impl->m_color_textures[attachment];
	}
	return gl::Texture();
}

Texture Fbo::depth_texture()
{
	return m_impl ? m_impl->m_depth_texture : gl::Texture();
}

void Fbo::set_depth_texture(gl::Texture the_depth_tex)
{
#if !defined(KINSKI_GLES_2)
    if(m_impl && m_impl->m_format.m_depth_buffer)
    {
        if(m_impl->m_format.m_depth_buffer_texture)
        {
            SaveFramebufferBinding sfb;
            bind();
            auto attach = m_impl->m_format.has_stencil_buffer() ?
                          GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
            
#if defined(KINSKI_GLES_3)
            glFramebufferTexture2D(GL_FRAMEBUFFER, attach, the_depth_tex.target(),
                                   the_depth_tex.id(), 0);
#else
            glFramebufferTexture(GL_FRAMEBUFFER, attach, the_depth_tex.id(), 0);
#endif
            FboExceptionInvalidSpecification exc;

            // failed creation; throw
            if(check_status(&exc)) { m_impl->m_depth_texture = the_depth_tex; }
            else{ LOG_ERROR << exc.what(); }
        }
    }
#endif
}

void Fbo::bind_texture(int the_texture_unit, int the_attachment)
{
	if(m_impl)
	{
		resolve_textures();
		m_impl->m_color_textures[the_attachment].bind(the_texture_unit);
		update_mipmaps(false, the_attachment);
	}
}

void Fbo::unbind_texture()
{
	glBindTexture(target(), 0);
}

void Fbo::bind_depth_texture(int the_texture_unit)
{
	if(m_impl){ m_impl->m_depth_texture.bind(the_texture_unit); }
}

void Fbo::resolve_textures() const
{
    if(!m_impl || !m_impl->m_needs_resolve){ return; }

#if !defined(KINSKI_GLES_2)
	// if this FBO is multisampled, resolve it, so it can be displayed
	if(m_impl->m_resolve_fbo_id)
    {
		//SaveFramebufferBinding saveFboBinding;

		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_impl->m_id);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_impl->m_resolve_fbo_id);
		
		for(size_t c = 0; c < m_impl->m_color_textures.size(); ++c)
        {
#if !defined(KINSKI_GLES)
			glDrawBuffer(GL_COLOR_ATTACHMENT0 + c);
			glReadBuffer(GL_COLOR_ATTACHMENT0 + c);
#endif
			GLbitfield bitfield = GL_COLOR_BUFFER_BIT;
            
			if(m_impl->m_depth_texture && m_impl->m_depth_texture.id())
                bitfield |= GL_DEPTH_BUFFER_BIT;
            
			glBlitFramebuffer(0, 0, m_impl->m_width, m_impl->m_height, 0, 0, m_impl->m_width,
                              m_impl->m_height, bitfield, GL_NEAREST);
            KINSKI_CHECK_GL_ERRORS();
		}

		// restore the draw buffers to the default for the antialiased (non-resolve) framebuffer
		vector<GLenum> drawBuffers;
        
		for(size_t c = 0; c < m_impl->m_color_textures.size(); ++c)
			drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + c);
        
		glBindFramebuffer(GL_FRAMEBUFFER, m_impl->m_id);
		glDrawBuffers(drawBuffers.size(), drawBuffers.data());
        KINSKI_CHECK_GL_ERRORS();
	}
#endif
	m_impl->m_needs_resolve = false;
}

void Fbo::update_mipmaps(bool the_bind_first, int the_attachment) const
{
    if(!m_impl->m_needs_mipmap_update){ return; }
	
	if(the_bind_first)
    {
		m_impl->m_color_textures[the_attachment].bind();
		glGenerateMipmap(target());
	}
    else{ glGenerateMipmap(target()); }

	m_impl->m_needs_mipmap_update = false;
}

void Fbo::bind()
{
	if(m_impl)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_impl->m_id);
		if(m_impl->m_resolve_fbo_id){ m_impl->m_needs_resolve = true; }
		if(m_impl->m_format.has_mipmapping()){ m_impl->m_needs_mipmap_update = true; }
	}
}

void Fbo::unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool Fbo::check_status(FboExceptionInvalidSpecification *resultExc)
{
	GLenum status;
	status = (GLenum) glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch(status)
    {
		case GL_FRAMEBUFFER_COMPLETE:
		break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			*resultExc = FboExceptionInvalidSpecification("Unsupported framebuffer format");
		return false;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			*resultExc = FboExceptionInvalidSpecification("Framebuffer incomplete: missing attachment");
		return false;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			*resultExc = FboExceptionInvalidSpecification("Framebuffer incomplete: duplicate attachment");
		return false;

#if ! defined(KINSKI_GLES)
		case GL_ENUM(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER):
			*resultExc = FboExceptionInvalidSpecification("Framebuffer incomplete: missing draw buffer");
		return false;
		case GL_ENUM(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER):
			*resultExc = FboExceptionInvalidSpecification("Framebuffer incomplete: missing read buffer");
		return false;
#endif
		default:
			*resultExc = FboExceptionInvalidSpecification("Framebuffer invalid: unknown reason");
		return false;
    }
	
    return status == GL_FRAMEBUFFER_COMPLETE;
}

GLint Fbo::max_num_samples()
{
#if ! defined(KINSKI_GLES)
    if(sMaxSamples < 0){ glGetIntegerv(GL_MAX_SAMPLES, &sMaxSamples); }
	return sMaxSamples;
#else
	return 0;
#endif
}

GLint Fbo::max_num_attachments()
{
#if ! defined(KINSKI_GLES)
	if(sMaxAttachments < 0) { glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &sMaxAttachments); }
	return sMaxAttachments;
#else
	return 1;
#endif
}

#if !defined(KINSKI_GLES_2)

void Fbo::blit_to_current(const Area_<int> &the_src, const Area_<int> &the_dst,
                          GLenum filter, GLbitfield mask) const
{
	if(!m_impl){ return; }
    SaveFramebufferBinding sb;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_impl->m_id);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sb.value());
    glBlitFramebuffer(the_src.x0, the_src.y0, the_src.x1, the_src.y1, the_dst.x0, the_dst.y0,
                      the_dst.x1, the_dst.y1, mask, filter);
	KINSKI_CHECK_GL_ERRORS();
}

void Fbo::blit_to(Fbo the_dst_fbo, const Area_<int> &the_src, const Area_<int> &the_dst,
                  GLenum filter, GLbitfield mask) const
{
	if(!m_impl){ return; }
	SaveFramebufferBinding sb;

	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_impl->m_id);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, the_dst_fbo.id());
	glBlitFramebuffer(the_src.x0, the_src.y0, the_src.x1, the_src.y1, the_dst.x0, the_dst.y0,
                      the_dst.x1, the_dst.y1, mask, filter);
}

void Fbo::blit_to_screen(const Area_<int> &the_src, const Area_<int> &the_dst,
                         GLenum filter, GLbitfield mask) const
{
	if(!m_impl){ return; }
	SaveFramebufferBinding sb;

	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_impl->m_id);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(the_src.x0, the_src.y0, the_src.x1, the_src.y1,
                      the_dst.x0, the_dst.y0, the_dst.x1, the_dst.y1, mask, filter);
}

void Fbo::blit_from_screen(const Area_<int> &the_src, const Area_<int> &the_dst, GLenum filter,
                           GLbitfield mask)
{
	if(!m_impl){ return; }
	SaveFramebufferBinding sb;
	glBindFramebuffer(GL_READ_FRAMEBUFFER, GL_NONE);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_impl->m_id);
	glBlitFramebuffer(the_src.x0, the_src.y0, the_src.x1, the_src.y1,
                      the_dst.x0, the_dst.y0, the_dst.x1, the_dst.y1, mask, filter);
}
#endif

FboExceptionInvalidSpecification::FboExceptionInvalidSpecification(const string &message) throw()
	: FboException()
{
	strncpy(m_message, message.c_str(), 255);
}

}// namespace gl
}
