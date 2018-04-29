// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schm_idt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "Texture.hpp"

namespace kinski{ namespace gl{

//! Represents an OpenGL Renderbuffer, used primarily in conjunction with FBOs. Supported on OpenGL ES but multisampling is currently ignored. \ImplShared
class KINSKI_API Renderbuffer
{
 public:
	//! Creates a NULL Renderbuffer
	Renderbuffer() {}
	//! Create a Renderbuffer \a width pixels wide and \a heigh pixels high, with an internal format of \a internalFormat, defaulting to GL_RGBA8

	Renderbuffer(int width, int height, GLenum internalFormat = GL_ENUM(GL_RGBA8));

	//! Create a Renderbuffer \a width pixels wide and \a heigh pixels high, with an internal format of \a internalFormat, defaulting to GL_RGBA8, MSAA samples \a msaaSamples
	Renderbuffer(int width, int height, GLenum internalFormat, int msaaSamples);

	//! Returns the width of the Renderbuffer in pixels
    int width() const;
    
	//! Returns the height of the Renderbuffer in pixels
    int height() const;
    
	//! Returns the size of the Renderbuffer in pixels
    ivec2 size() const;
    
	//! Returns the bounding area of the Renderbuffer in pixels
	//Area	getBounds() const { return Area( 0, 0, m_obj->m_width, m_obj->m_height ); }
    
	//! Returns the aspect ratio of the Renderbuffer
    float aspect_ratio() const;

	//! Returns the ID of the Renderbuffer
    GLuint	id() const;
	//! Returns the internal format of the Renderbuffer
    GLenum	internal_format() const;
	//! Returns the number of samples used in MSAA-style antialiasing. Defaults to none, disabling multisampling
    int num_samples() const;
	//! Returns the number of coverage samples used in CSAA-style antialiasing. Defaults to none.
	//int		getCoverageSamples() const { return m_obj->m_num_coverage_samples; }

  private:
    std::shared_ptr<struct RenderbufferImpl> m_impl;

  public:
    explicit operator bool() const { return m_impl.get(); }
	void reset() { m_impl.reset(); }
};

//! Represents an OpenGL Framebuffer Object. \ImplShared
class KINSKI_API Fbo
{
 public:
	struct Format;

	//! Creates a NULL FBO
	Fbo() {}
    //! Creates an FBO \a width pixels wide and \a height pixels high, using Fbo::Format \a format
    Fbo( const vec2 &the_size, Format format = Format() );
	//! Creates an FBO \a width pixels wide and \a height pixels high, using Fbo::Format \a format
	Fbo( int width, int height, Format format = Format() );
	//! Creates an FBO \a width pixels wide and \a height pixels high, with an optional alpha channel, color buffer and depth buffer
	Fbo( int width, int height, bool alpha, bool color = true, bool depth = true );

	//! Returns the width of the FBO in pixels
    int width() const;
	//! Returns the height of the FBO in pixels
    int height() const;
	//! Returns the size of the FBO in pixels
    ivec2 size() const;

	void enable_draw_buffers(bool b = true);

	void add_attachment(gl::Texture the_attachment, int the_index = -1);

	//! Returns the bounding area of the FBO in pixels
	//Area			getBounds() const { return Area( 0, 0, m_obj->m_width, m_obj->m_height ); }
    
	//! Returns the aspect ratio of the FBO
    float aspect_ratio() const;
	//! Returns the Fbo::Format of this FBO
    const Format& format() const;
	//! Returns the texture target for this FBO. Typically \c GL_TEXTURE_2D or \c GL_TEXTURE_RECTANGLE_ARB
    GLenum target() const;

	//! Returns a reference to the color texture of the FBO. \a attachment specifies which attachment in the case of multiple color buffers
	Texture texture(int the_attachment = 0);
	//! Returns a reference to the depth texture of the FBO.
	Texture depth_texture();

    void set_depth_texture(gl::Texture the_depth_tex);
	
	//! Binds the color texture associated with an Fbo to its target. Optionally binds to a multitexturing unit when \a textureUnit is non-zero.  \a attachment specifies which color buffer in the case of multiple attachments.
	void bind_texture(int the_texture_unit = 0, int the_attachment = 0);
	//! Unbinds the texture associated with an Fbo's target
	void unbind_texture();
	//! Binds the depth texture associated with an Fbo to its target.
	void bind_depth_texture(int the_texture_unit = 0);
	
    //! Binds the Fbo as the currently active framebuffer, meaning it will receive the results of all subsequent rendering until it is unbound
	void bind();
    
	//! Unbinds the Fbo as the currently active framebuffer, restoring the primary context as the target for all subsequent rendering
	static void unbind();

	//! Returns the ID of the framebuffer itself. For antialiased FBOs this is the ID of the output multisampled FBO
    GLuint id() const;

#if !defined(KINSKI_GLES_2)
//	//! For antialiased FBOs this returns the ID of the mirror FBO designed for reading, where the multisampled render buffers are resolved to. For non-antialised, this is the equivalent to getId()
//	GLuint		getResolveId() const { if( m_obj->m_resolve_fbo_id ) return m_obj->m_resolve_fbo_id; else return m_obj->m_id; }
//
    void blit_to_current(const Area_<int> &the_src, const Area_<int> &the_dst,
                         GLenum filter = GL_NEAREST, GLbitfield mask = GL_COLOR_BUFFER_BIT) const;

//	//! Copies to FBO \a dst from \a srcArea to \a dstArea using filter \a filter. \a mask allows specification of color (\c GL_COLOR_BUFFER_BIT) and/or depth(\c GL_DEPTH_BUFFER_BIT). Calls glBlitFramebufferEXT() and is subject to its constraints and coordinate system.
	void blit_to(Fbo the_dst_fbo, const Area_<int> &the_src, const Area_<int> &the_dst,
                 GLenum filter = GL_NEAREST, GLbitfield mask = GL_COLOR_BUFFER_BIT) const;
    
//	//! Copies to the screen from Area \a srcArea to \a dstArea using filter \a filter. \a mask allows specification of color (\c GL_COLOR_BUFFER_BIT) and/or depth(\c GL_DEPTH_BUFFER_BIT). Calls glBlitFramebufferEXT() and is subject to its constraints and coordinate system.
	void blit_to_screen(const Area_<int> &the_src, const Area_<int> &the_dst,
                        GLenum filter = GL_NEAREST, GLbitfield mask = GL_COLOR_BUFFER_BIT) const;

    //! Copies from the screen from Area \a srcArea to \a dstArea using filter \a filter. \a mask allows specification of color (\c GL_COLOR_BUFFER_BIT) and/or depth(\c GL_DEPTH_BUFFER_BIT). Calls glBlitFramebufferEXT() and is subject to its constraints and coordinate system.
        void blit_from_screen(const Area_<int> &the_src, const Area_<int> &the_dst,
                              GLenum filter = GL_NEAREST, GLbitfield mask = GL_COLOR_BUFFER_BIT);
#endif

	//! Returns the maximum number of samples the graphics card is capable of using per pixel in MSAA for an Fbo
	static GLint	max_num_samples();
	//! Returns the maximum number of color attachments the graphics card is capable of using for an Fbo
	static GLint	max_num_attachments();
	
	struct KINSKI_API Format
    {
	  public:
		//! Default constructor, sets the target to \c GL_TEXTURE_2D with an 8-bit color+alpha, a 24-bit depth texture, and no multisampling or mipmapping
		Format();

		//! Set the texture target associated with the FBO. Defaults to \c GL_TEXTURE_2D, \c GL_TEXTURE_RECTANGLE_ARB is a common option as well
		void set_target(GLenum the_target) { m_target = the_target; }
        
		//! Sets the GL internal format for the color buffer. Defaults to \c GL_RGBA8 (and \c GL_RGBA on OpenGL ES). Common options also include \c GL_RGB8 and \c GL_RGBA32F
		void set_color_internal_format(GLenum the_color_internal_format)
        {
            m_color_internal_format = the_color_internal_format;
        }
        
		//! Sets the GL internal format for the depth buffer. Defaults to \c GL_DEPTH_COMPONENT24. Common options also include \c GL_DEPTH_COMPONENT16 and \c GL_DEPTH_COMPONENT32
		void set_depth_internal_format(GLenum the_depth_internal_format)
        {
            m_depth_internal_format = the_depth_internal_format;
        }
        
		//! Sets the number of samples used in MSAA-style antialiasing. Defaults to none, disabling multisampling. Note that not all implementations support multisampling. Ignored on OpenGL ES.
		void set_num_samples(int the_num_samples) { m_num_samples = the_num_samples; }
        
		//! Sets the number of coverage samples used in CSAA-style antialiasing. Defaults to none. Note that not all implementations support CSAA, and is currenlty Windows-only Nvidia. Ignored on OpenGL ES.
		void set_coverage_samples(int the_coverage_samples)
        {
            m_num_coverage_samples = the_coverage_samples;
        }
        
		//! Enables or disables the creation of a color buffer for the FBO.. Creates multiple color attachments when \a numColorsBuffers >1, except on OpenGL ES which supports only 1.
		void enable_color_buffer(bool the_color_buffer = true, int the_num_buffers = 1);
        
		//! Enables or disables the creation of a depth buffer for the FBO. If \a asTexture the depth buffer is created as a gl::Texture, obtainable via getDepthTexture(). Not supported on OpenGL ES.
		void enable_depth_buffer(bool the_depth_buffer = true, bool as_texture = true);
        
        void enable_stencil_buffer(bool the_stencil_buffer = true)
        {
            m_stencil_buffer = the_stencil_buffer;
        }
        
		//! Enables or disables mip-mapping for the FBO's textures
		void enable_mipmapping( bool the_enable_mipmapping = true )
        {
            m_mipmapping = the_enable_mipmapping;
        }

		//! Sets the wrapping behavior for the FBO's textures. Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. Default is \c GL_CLAMP_TO_EDGE.
		void set_wrap(GLenum the_wrap_s, GLenum the_wrap_t)
        {
            set_wrap_s(the_wrap_s); set_wrap_t(the_wrap_t);
        }
        
		/** \brief Sets the horizontal wrapping behavior for the FBO's textures. Default is \c GL_CLAMP_TO_EDGE.
			Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. **/
		void set_wrap_s(GLenum the_wrap_s) { m_wrap_s = the_wrap_s; }
        
		/** \brief Sets the vertical wrapping behavior for the FBO's textures. Default is \c GL_CLAMP_TO_EDGE.
			Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. **/
		void set_wrap_t(GLenum the_wrap_t) { m_wrap_t = the_wrap_t; }
        
		/** \brief Sets the minification filtering behavior for the FBO's textures. Default is \c GL_LINEAR:
		 * Possible values are \li \c GL_NEAREST \li \c GL_LINEAR \li \c GL_NEAREST_MIPMAP_NEAREST \li \c GL_LINEAR_MIPMAP_NEAREST \li \c GL_NEAREST_MIPMAP_LINEAR \li \c GL_LINEAR_MIPMAP_LINEAR **/
		void set_min_filter(GLenum the_min_filter) { m_min_filter = the_min_filter; }
        
		/** Sets the magnification filtering behavior for the FBO's textures. Default is \c GL_LINEAR:
		 * Possible values are \li \c GL_NEAREST \li \c GL_LINEAR \li \c GL_NEAREST_MIPMAP_NEAREST \li \c GL_LINEAR_MIPMAP_NEAREST \li \c GL_NEAREST_MIPMAP_LINEAR \li \c GL_LINEAR_MIPMAP_LINEAR **/
		void set_mag_filter(GLenum the_mag_filter) { m_mag_filter = the_mag_filter; }

		//! Returns the texture target associated with the FBO.
		GLenum target() const { return m_target; }
        
		//! Returns the GL internal format for the color buffer. Defaults to \c GL_RGBA8.
		GLenum color_internal_format() const { return m_color_internal_format; }
        
		//! Returns the GL internal format for the depth buffer. Defaults to \c GL_DEPTH_COMPONENT24.
		GLenum depth_internal_format() const { return m_depth_internal_format; }
        
		//! Returns the number of samples used in MSAA-style antialiasing. Defaults to none, disabling multisampling. OpenGL ES does not support multisampling.
		int num_samples() const { return m_num_samples; }
        
		//! Returns the number of coverage samples used in CSAA-style antialiasing. Defaults to none. OpenGL ES does not support multisampling.
		int num_num_coverage_samples() const { return m_num_coverage_samples; }
        
		//! Returns whether the FBO contains a color buffer
		bool has_color_buffer() const { return m_num_color_buffers > 0; }
		
		//! Returns whether the FBO contains a depth buffer
		bool has_depth_buffer() const { return m_depth_buffer; }
        
		//! Returns whether the FBO contains a depth buffer implemened as a texture. Always \c false on OpenGL ES.
		bool has_depth_buffer_texture() const { return m_depth_buffer_texture; }

        bool has_stencil_buffer() const { return m_stencil_buffer; }
        
		//! Returns whether the contents of the FBO textures are mip-mapped.
		bool has_mipmapping() const { return m_mipmapping; }
        
        //! Returns the number of color buffers
		uint32_t num_color_buffers() const { return m_num_color_buffers; }
        void set_num_color_buffers(int the_num) { m_num_color_buffers = the_num; }
		
	  protected:
		GLenum	m_target;
		GLenum	m_color_internal_format, m_depth_internal_format, m_stencil_internal_format;
		GLenum m_depth_data_type;
        
        GLint       m_data_type;
		int			m_num_samples;
		int			m_num_coverage_samples;
		bool		m_mipmapping;
		bool		m_depth_buffer, m_depth_buffer_texture, m_stencil_buffer;
		uint32_t	m_num_color_buffers;
		GLenum		m_wrap_s, m_wrap_t;
		GLenum		m_min_filter, m_mag_filter;
		
		friend class Fbo;
	};

 private:
	void		init();
	bool		init_multisample();
	void		resolve_textures() const;
	void		update_mipmaps(bool the_bind_first, int the_attachment) const;
	bool		check_status( class FboExceptionInvalidSpecification *resultExc );
 
    
	std::shared_ptr<struct FboImpl>	m_impl;
	static GLint sMaxSamples, sMaxAttachments;
	
  public:
	//! Emulates shared_ptr-like behavior
	operator bool() const { return m_impl.get(); }
	void reset() { m_impl.reset(); }
};

class FboException : public Exception {
 public:
    FboException():Exception("FboException"){};
};

class FboExceptionInvalidSpecification : public FboException {
  public:
	FboExceptionInvalidSpecification() : FboException() { m_message[0] = 0; }
	FboExceptionInvalidSpecification( const std::string &message ) throw();
	
	virtual const char * what() const throw() { return m_message; }
	
  private:	
	char m_message[256];
};


//! create a framebuffer object with attached cubemaps
KINSKI_API gl::Fbo create_cube_framebuffer(uint32_t the_width, bool with_color_buffer = true,
										   GLenum the_datatype = GL_UNSIGNED_BYTE);

/* create an image from the content of the provided framebuffer object.
 * if no framebuffer-object is provided, the display framebuffer will be used instead
 */
KINSKI_API ImagePtr create_image_from_framebuffer(gl::Fbo the_fbo = gl::Fbo());

}}// namespace gl
