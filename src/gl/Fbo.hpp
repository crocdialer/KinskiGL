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

namespace kinski {
namespace gl {

//! Represents an OpenGL Renderbuffer, used primarily in conjunction with FBOs. Supported on OpenGL ES but multisampling is currently ignored. \ImplShared
class Renderbuffer
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
    GLuint id() const;

    //! Returns the internal format of the Renderbuffer
    GLenum internal_format() const;

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
class Fbo
{
public:

    ~Fbo();

    struct Format
    {
        //! Default constructor, sets the target to \c GL_TEXTURE_2D with an 8-bit color+alpha, a 24-bit depth texture, and no multisampling or mipmapping
        Format();

        uint32_t target;
        uint32_t color_internal_format;
        uint32_t depth_internal_format;
        uint32_t stencil_internal_format;
        uint32_t depth_data_type;
        uint32_t data_type;
        uint32_t num_samples;
        uint32_t num_coverage_samples;
        bool mipmapping;
        bool depth_buffer, depth_buffer_texture, stencil_buffer;
        uint32_t num_color_buffers;
        uint32_t wrap_s, wrap_t;
        uint32_t min_filter, mag_filter;
    };

    static FboPtr create(uint32_t width, uint32_t height, Format format = Format());

    static FboPtr create(const gl::vec2 &the_size, Format format = Format());

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
    const Format &format() const;

    //! Returns the texture target for this FBO. Typically \c GL_TEXTURE_2D or \c GL_TEXTURE_RECTANGLE_ARB
    GLenum target() const;

    //! Returns a reference to the color texture of the FBO. \a attachment specifies which attachment in the case of multiple color buffers
    Texture texture(int the_attachment = 0);

    Texture *texture_ptr(int attachment = 0);

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

    GLuint resolve_id() const;


#if !defined(KINSKI_GLES_2)

//	//! For antialiased FBOs this returns the ID of the mirror FBO designed for reading, where the multisampled render buffers are resolved to. For non-antialised, this is the equivalent to getId()
//	GLuint		getResolveId() const { if( m_obj->m_resolve_fbo_id ) return m_obj->m_resolve_fbo_id; else return m_obj->m_id; }
//
    void blit_to_current(const crocore::Area_<int> &the_src, const crocore::Area_<int> &the_dst,
                         GLenum filter = GL_NEAREST, GLbitfield mask = GL_COLOR_BUFFER_BIT) const;

//	//! Copies to FBO \a dst from \a srcArea to \a dstArea using filter \a filter. \a mask allows specification of color (\c GL_COLOR_BUFFER_BIT) and/or depth(\c GL_DEPTH_BUFFER_BIT). Calls glBlitFramebufferEXT() and is subject to its constraints and coordinate system.
    void blit_to(Fbo the_dst_fbo, const crocore::Area_<int> &the_src, const crocore::Area_<int> &the_dst,
                 GLenum filter = GL_NEAREST, GLbitfield mask = GL_COLOR_BUFFER_BIT) const;

//	//! Copies to the screen from Area \a srcArea to \a dstArea using filter \a filter. \a mask allows specification of color (\c GL_COLOR_BUFFER_BIT) and/or depth(\c GL_DEPTH_BUFFER_BIT). Calls glBlitFramebufferEXT() and is subject to its constraints and coordinate system.
    void blit_to_screen(const crocore::Area_<int> &the_src, const crocore::Area_<int> &the_dst,
                        GLenum filter = GL_NEAREST, GLbitfield mask = GL_COLOR_BUFFER_BIT) const;

    //! Copies from the screen from Area \a srcArea to \a dstArea using filter \a filter. \a mask allows specification of color (\c GL_COLOR_BUFFER_BIT) and/or depth(\c GL_DEPTH_BUFFER_BIT). Calls glBlitFramebufferEXT() and is subject to its constraints and coordinate system.
    void blit_from_screen(const crocore::Area_<int> &the_src, const crocore::Area_<int> &the_dst,
                          GLenum filter = GL_NEAREST, GLbitfield mask = GL_COLOR_BUFFER_BIT);

#endif

    //! Returns the maximum number of samples the graphics card is capable of using per pixel in MSAA for an Fbo
    static uint32_t max_num_samples();

    //! Returns the maximum number of color attachments the graphics card is capable of using for an Fbo
    static uint32_t max_num_attachments();

private:

    //! Creates an FBO \a width pixels wide and \a height pixels high, using Fbo::Format \a format
    Fbo(uint32_t width, uint32_t height, Format format);

    //! Creates an FBO \a width pixels wide and \a height pixels high, with an optional alpha channel, color buffer and depth buffer
//    Fbo( int width, int height, bool alpha, bool color = true, bool depth = true );

    void init();

    bool init_multisample();

    void resolve_textures() const;

    void update_mipmaps(bool the_bind_first, int the_attachment) const;

    bool check_status(class FboExceptionInvalidSpecification *resultExc);


    std::unique_ptr<struct FboImpl> m_impl;
    static GLint sMaxSamples, sMaxAttachments;
};


class FboExceptionInvalidSpecification : public std::runtime_error
{
public:

    explicit FboExceptionInvalidSpecification(const std::string &message = "") throw();

    virtual const char *what() const throw() { return m_message; }

private:
    char m_message[256];
};


//! create a framebuffer object with attached cubemaps
gl::FboPtr create_cube_framebuffer(uint32_t the_width, bool with_color_buffer = true,
                                   GLenum the_datatype = GL_UNSIGNED_BYTE);

/* create an image from the content of the provided framebuffer object.
 * if no framebuffer-object is provided, the display framebuffer will be used instead
 */
crocore::ImagePtr create_image_from_framebuffer(gl::FboPtr the_fbo = gl::FboPtr());

}
}// namespace gl
