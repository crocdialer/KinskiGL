// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schm_idt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include <unordered_map>
#include "Fbo.hpp"

using namespace std;

namespace kinski { namespace gl {

////////////////////////////////////////////////////////////////////////////////////////////////////

gl::FboPtr create_cube_framebuffer(uint32_t the_width, bool with_color_buffer, GLenum the_datatype)
{
#if !defined(KINSKI_GLES_2)
    uint32_t cube_sz = the_width;
    gl::Fbo::Format fbo_fmt;
    fbo_fmt.num_color_buffers = 0;
    auto fbo = gl::Fbo::create(cube_sz, cube_sz, fbo_fmt);
    
    gl::Texture::Format cube_depth_fmt;
    cube_depth_fmt.target = GL_TEXTURE_CUBE_MAP;
    cube_depth_fmt.datatype = GL_FLOAT;
    cube_depth_fmt.internal_format = GL_DEPTH_COMPONENT32F;
    cube_depth_fmt.min_filter = GL_NEAREST;
    cube_depth_fmt.mag_filter = GL_NEAREST;
    auto cube_depth_tex = gl::Texture(nullptr, GL_DEPTH_COMPONENT, cube_sz, cube_sz, cube_depth_fmt);
    fbo->set_depth_texture(cube_depth_tex);

    if(with_color_buffer)
    {
        constexpr uint32_t num_color_components = 3;
        GLenum format, internal_format;
        gl::get_texture_format(num_color_components, false, the_datatype, &format, &internal_format);

        gl::Texture::Format col_fmt;
        col_fmt.datatype = the_datatype;
        col_fmt.internal_format = internal_format;
        col_fmt.target = GL_TEXTURE_CUBE_MAP;
        col_fmt.min_filter = GL_NEAREST;
        col_fmt.mag_filter = GL_NEAREST;
        auto cube_tex = gl::Texture(nullptr, format, cube_sz, cube_sz, col_fmt);
        fbo->add_attachment(cube_tex, 0);
    }
    return fbo;
#endif
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
    
ImagePtr create_image_from_framebuffer(gl::FboPtr the_fbo)
{
    gl::SaveFramebufferBinding sfb;
    int w = gl::window_dimension().x, h = gl::window_dimension().y, num_comp = 3;
    GLenum format = GL_RGB;
    
    if(the_fbo)
    {
        the_fbo->bind();
        w = the_fbo->width();
        h = the_fbo->height();
        num_comp = 4;
        format = GL_RGBA;
    }
    auto ret = Image_<uint8_t >::create(w, h, num_comp);
    ret->m_type = Image::Type::RGBA;
    glReadPixels(0, 0, w, h, format, GL_UNSIGNED_BYTE, ret->data());
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
        uint32_t m_num_samples;
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

struct FboImpl
{
    FboImpl(){}
    
    FboImpl(int the_width, int the_height):
    m_width(the_width),
    m_height(the_height){}
    
    ~FboImpl()
    {

    }

    int m_width, m_height;
    Fbo::Format m_format;

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
	target = GL_TEXTURE_2D;
#if defined(KINSKI_GLES_2)
    color_internal_format = GL_RGBA;
	depth_internal_format = GL_ENUM(GL_DEPTH_COMPONENT24);
    stencil_internal_format = GL_STENCIL_INDEX8;
	depth_buffer_texture = false;
#else
	color_internal_format = GL_RGBA8;
    depth_data_type = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
    depth_internal_format = GL_DEPTH32F_STENCIL8;//GL_DEPTH_COMPONENT32F; //GL_DEPTH24_STENCIL8;
	depth_buffer_texture = true;
//#if defined(KINSKI_GLES_3)
//    m_depth_data_type = GL_UNSIGNED_INT_24_8;//GL_DEPTH24_STENCIL8;
//    m_depth_internal_format = GL_DEPTH_COMPONENT24;
//    m_depth_buffer_texture = true;
//#endif
    
#endif
	num_samples = 0;
	num_coverage_samples = 0;
	num_color_buffers = 1;
	depth_buffer = true;
	stencil_buffer = false;
	mipmapping = false;
	wrap_s = GL_CLAMP_TO_EDGE;
	wrap_t = GL_CLAMP_TO_EDGE;
	min_filter = GL_LINEAR;
	mag_filter = GL_LINEAR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fbo

Fbo::~Fbo()
{
    gl::context()->clear_fbo(this, 0);
    gl::context()->clear_fbo(this, 1);
}

FboPtr Fbo::create(uint32_t width, uint32_t height, Format format)
{
    return FboPtr(new Fbo(width, height, format));
}

FboPtr Fbo::create(const gl::vec2 &the_size, Format format)
{
    return FboPtr(new Fbo(the_size.x, the_size.y, format));
}


void Fbo::init()
{
    SaveFramebufferBinding sfb;

	bool useAA = m_impl->m_format.num_samples > 0;

	uint32_t fbo_id = gl::context()->create_fbo(this, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);

	Texture::Format textureFormat;
	textureFormat.target = target();
    auto col_fmt = format().color_internal_format;
	textureFormat.internal_format = col_fmt;
    
#if !defined(KINSKI_GLES_2)
    GLint float_types[] = {GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F};
    GLint one_comp_types[] = {GL_RED, GL_GREEN, GL_BLUE, GL_R32F};
    if(contains(float_types, col_fmt)){ textureFormat.datatype = GL_FLOAT; }
#endif
	textureFormat.wrap_s = m_impl->m_format.wrap_s;
    textureFormat.wrap_t = m_impl->m_format.wrap_t;
	textureFormat.min_filter = m_impl->m_format.min_filter;
	textureFormat.mag_filter = m_impl->m_format.mag_filter;
	textureFormat.mipmapping = format().mipmapping;

	// color textures might already exist, e.g. after context switch/loss
    if(m_impl->m_color_textures.size() != m_impl->m_format.num_color_buffers)
    {
        m_impl->m_color_textures.clear();

        // allocate the color buffers
        for(uint32_t c = 0; c < m_impl->m_format.num_color_buffers; ++c)
        {
            auto tex = Texture(m_impl->m_width, m_impl->m_height, textureFormat);

#if !defined(KINSKI_GLES_2)
        if(contains(one_comp_types, col_fmt)){ tex.set_swizzle(GL_RED, GL_RED, GL_RED, GL_ONE); }
#endif
            m_impl->m_color_textures.push_back(tex);
        }
    }

#if !defined(KINSKI_GLES)
	if(m_impl->m_format.num_color_buffers == 0)
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
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + c, target(),
                                 m_impl->m_color_textures[c].id(), 0);
#else
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + c,
                                   m_impl->m_color_textures[c].id(), 0);
			drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + c);
#endif
		}
#if !defined(KINSKI_GLES_2)
        if(!drawBuffers.empty()){ glDrawBuffers(drawBuffers.size(), drawBuffers.data()); }
#endif

		// allocate and attach depth texture
		if(m_impl->m_format.depth_buffer)
        {
			if(m_impl->m_format.depth_buffer_texture)
            {
#if !defined(KINSKI_GLES_2)
                if(!m_impl->m_depth_texture)
                {
                    gl::Texture::Format tex_fmt;
                    tex_fmt.target = target();
                    tex_fmt.internal_format = format().depth_internal_format;
                    tex_fmt.datatype = format().depth_data_type;
                    m_impl->m_depth_texture = Texture(nullptr, GL_DEPTH_STENCIL, m_impl->m_width, m_impl->m_height, tex_fmt);
                    m_impl->m_depth_texture.set_min_filter(m_impl->m_format.min_filter);
                    m_impl->m_depth_texture.set_mag_filter(m_impl->m_format.mag_filter);
                    m_impl->m_depth_texture.set_wrap_s(m_impl->m_format.wrap_s);
                    m_impl->m_depth_texture.set_wrap_t(m_impl->m_format.wrap_t);
                }

                auto attach = m_impl->m_format.stencil_buffer ?
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
			else if(m_impl->m_format.depth_buffer)
            {
				m_impl->m_depthRenderbuffer = Renderbuffer(m_impl->m_width, m_impl->m_height,
                                                          m_impl->m_format.depth_internal_format);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                          m_impl->m_depthRenderbuffer.id());
                
                if(m_impl->m_format.stencil_buffer)
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
}

bool Fbo::init_multisample()
{
#if defined(KINSKI_GLES_2)
	return false;
#else

    uint32_t resolve_fbo_id = gl::context()->create_fbo(this, 1);
    glBindFramebuffer(GL_FRAMEBUFFER, resolve_fbo_id);
	
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

	glBindFramebuffer(GL_FRAMEBUFFER, id());

	m_impl->m_format.num_samples = std::min(m_impl->m_format.num_samples, max_num_samples());
    
	// create the multisampled color renderbuffers
    if(m_impl->mMultisampleColorRenderbuffers.size() != m_impl->m_format.num_color_buffers)
    {
        m_impl->mMultisampleColorRenderbuffers.clear();

        for(uint32_t c = 0; c < m_impl->m_format.num_color_buffers; ++c)
        {
            auto render_buf = Renderbuffer(m_impl->m_width, m_impl->m_height, m_impl->m_format.color_internal_format,
                                           m_impl->m_format.num_samples);
            m_impl->mMultisampleColorRenderbuffers.push_back(render_buf);
        }
    }

    // attach the multisampled color renderbuffers
	for(uint32_t c = 0; c < m_impl->mMultisampleColorRenderbuffers.size(); ++c)
    {
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + c, GL_RENDERBUFFER,
                                  m_impl->mMultisampleColorRenderbuffers[c].id());
	}

	if(m_impl->m_format.depth_buffer)
    {
		// create the multisampled depth Renderbuffer
        if(!m_impl->m_multisample_depth_renderbuffer)
        {
		    m_impl->m_multisample_depth_renderbuffer = Renderbuffer(m_impl->m_width, m_impl->m_height,
                                                                    m_impl->m_format.depth_internal_format,
                                                                    m_impl->m_format.num_samples);
        }

		// attach the depth Renderbuffer
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                  m_impl->m_multisample_depth_renderbuffer.id());
        
        if(m_impl->m_format.stencil_buffer)
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

    
Fbo::Fbo(uint32_t width, uint32_t height, Format format):
m_impl(new FboImpl(width, height))
{
	m_impl->m_format = format;
	init();
}

//Fbo::Fbo(int width, int height, bool alpha, bool color, bool depth):
//m_impl(new FboImpl(width, height))
//{
//	Format format;
//	m_impl->m_format.m_color_internal_format = (alpha) ? GL_ENUM(GL_RGBA8) : GL_ENUM(GL_RGB8);
//	m_impl->m_format.m_depth_buffer = depth;
//	m_impl->m_format.m_num_color_buffers = color ? 1 : 0;
//	init();
//}

GLuint Fbo::id() const
{
    return gl::context()->get_fbo(this, 0);
}

GLuint Fbo::resolve_id() const
{
    return gl::context()->get_fbo(this, 1);
}

int Fbo::width() const{ return m_impl->m_width; }
    
int Fbo::height() const{ return m_impl->m_height; }

ivec2 Fbo::size() const{ return ivec2( m_impl->m_width, m_impl->m_height ); }
    
float Fbo::aspect_ratio() const{ return m_impl->m_width / (float)m_impl->m_height; }

const Fbo::Format& Fbo::format() const { return m_impl->m_format; }
    
GLenum Fbo::target() const { return m_impl->m_format.target; }

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
    bind();

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
        m_impl->m_format.num_color_buffers = m_impl->m_color_textures.size();
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
    if(m_impl && m_impl->m_format.depth_buffer)
    {
        if(m_impl->m_format.depth_buffer_texture)
        {
            SaveFramebufferBinding sfb;
            bind();
            auto attach = m_impl->m_format.stencil_buffer ?
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
    uint32_t resolve_id = gl::context()->get_fbo(this, 1);

	if(resolve_id)
    {
		//SaveFramebufferBinding saveFboBinding;

		glBindFramebuffer(GL_READ_FRAMEBUFFER, id());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolve_id);
		
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
        
		glBindFramebuffer(GL_FRAMEBUFFER, resolve_id);
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
	    if(!id()){ init(); }
		glBindFramebuffer(GL_FRAMEBUFFER, id());
		if(resolve_id()){ m_impl->m_needs_resolve = true; }
		if(m_impl->m_format.mipmapping){ m_impl->m_needs_mipmap_update = true; }
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

uint32_t Fbo::max_num_samples()
{
#if ! defined(KINSKI_GLES)
    if(sMaxSamples < 0){ glGetIntegerv(GL_MAX_SAMPLES, &sMaxSamples); }
	return sMaxSamples;
#else
	return 0;
#endif
}

uint32_t Fbo::max_num_attachments()
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

    glBindFramebuffer(GL_READ_FRAMEBUFFER, id());
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

	glBindFramebuffer(GL_READ_FRAMEBUFFER, id());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, the_dst_fbo.id());
	glBlitFramebuffer(the_src.x0, the_src.y0, the_src.x1, the_src.y1, the_dst.x0, the_dst.y0,
                      the_dst.x1, the_dst.y1, mask, filter);
}

void Fbo::blit_to_screen(const Area_<int> &the_src, const Area_<int> &the_dst,
                         GLenum filter, GLbitfield mask) const
{
	if(!m_impl){ return; }
	SaveFramebufferBinding sb;

	glBindFramebuffer(GL_READ_FRAMEBUFFER, id());
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
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id());
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
