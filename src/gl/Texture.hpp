// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include <crocore/Image.hpp>
#include "gl/gl.hpp"

namespace kinski{ namespace gl{

    /** \brief Represents an OpenGL Texture. \ImplShared*/
    class Texture
    {
    public:

        enum class Usage : uint32_t {COLOR = 1 << 0, NORMAL = 1 << 1, SPECULAR = 1 << 2, AO_ROUGHNESS_METAL = 1 << 3,
            EMISSION = 1 << 4, DISPLACEMENT = 1 << 5, SHADOW = 1 << 6, DEPTH = 1 << 7, ENVIROMENT = 1 << 8,
            ENVIROMENT_CONV_DIFF = 1 << 9, ENVIROMENT_CONV_SPEC = 1 << 10, BRDF_LUT = 1 << 11,
            NOISE = 1 << 12, MASK = 1 << 13};

        struct Format
        {
            uint32_t target = GL_TEXTURE_2D;
            uint32_t wrap_s = GL_REPEAT;
            uint32_t wrap_t = GL_REPEAT;
            uint32_t min_filter = GL_LINEAR;
            uint32_t mag_filter = GL_LINEAR;
            float anisotropic_filter_level = 0.f;
            bool mipmapping = false;
            uint32_t internal_format = GL_RGBA;
            uint32_t datatype = GL_UNSIGNED_BYTE;
            Format(){}
        };

        glm::mat4 texture_matrix = mat4(1);

        //! Default initializer.
        Texture(){};

        /** \brief Constructs a texture of size(\a aWidth, \a aHeight), storing the data in internal format \a aInternalFormat. **/
        Texture(int aWidth, int aHeight, Format format = Format());

        Texture(int aWidth, int aHeight, int aDepth, Format format = Format());

        /** \brief Constructs a texture of size(\a aWidth, \a aHeight), storing the data in internal format \a aInternalFormat. Pixel data is provided by \a data and is expected to be interleaved and in format \a dataFormat, for which \c GL_RGB or \c GL_RGBA would be typical values. **/
        Texture(const void *data, int dataFormat, int aWidth, int aHeight, Format format = Format());

        Texture(const void *data, int dataFormat, int aWidth, int aHeight, int aDepth,
                Format format = Format());

        //! Constructs a Texture based on an externally initialized OpenGL texture. \a aDoNotDispose specifies whether the Texture is responsible for disposing of the associated OpenGL resource.
        Texture(GLenum aTarget, GLuint aTextureID, int aWidth, int aHeight, bool aDoNotDispose);

        Texture(GLenum aTarget, GLuint aTextureID, int aWidth, int aHeight, int aDepth, bool aDoNotDispose);

        //! Determines whether the Texture will call glDeleteTextures() to free the associated texture objects on destruction
        void set_do_not_dispose(bool do_not_dispose = true);

        //! Sets the wrapping behavior when a texture coordinate falls outside the range of [0,1]. Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE.
        void set_wrap(GLenum the_wrap_s, GLenum the_wrap_t) { set_wrap_s(the_wrap_s); set_wrap_t(the_wrap_t); }

        /** \brief Sets the horizontal wrapping behavior when a texture coordinate falls outside the range of [0,1].
         Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. **/
        void set_wrap_s(GLenum the_wrap_s);

        /** \brief Sets the verical wrapping behavior when a texture coordinate falls outside the range of [0,1].
         Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. **/
        void set_wrap_t(GLenum the_wrap_t);

        /** \brief Sets the filtering behavior when a texture is displayed at a lower resolution than its native resolution.
         * Possible values are \li \c GL_NEAREST \li \c GL_LINEAR \li \c GL_NEAREST_MIPMAP_NEAREST \li \c GL_LINEAR_MIPMAP_NEAREST \li \c GL_NEAREST_MIPMAP_LINEAR \li \c GL_LINEAR_MIPMAP_LINEAR **/
        void set_min_filter(GLenum the_min_filter);

        /** Sets the filtering behavior when a texture is displayed at a higher resolution than its native resolution.
         * Possible values are \li \c GL_NEAREST \li \c GL_LINEAR \li \c GL_NEAREST_MIPMAP_NEAREST \li \c GL_LINEAR_MIPMAP_NEAREST \li \c GL_NEAREST_MIPMAP_LINEAR \li \c GL_LINEAR_MIPMAP_LINEAR **/
        void set_mag_filter(GLenum the_mag_filter);

        void set_mipmapping(bool b = true);
        void set_anisotropic_filter(float f);

        void set_swizzle(GLint red, GLint green, GLint blue, GLint alpha);

        mat4 transform() const;

        bool is_bound() const;
        GLint bound_texture_unit() const;

        //! Replaces the pixels of a texture with \a data
        void update(const uint8_t *data,GLenum format, int theWidth, int theHeight,
                    bool flipped = false);

        //! Replaces the pixels of a texture with \a data
        void update(const float *data, GLenum format, int theWidth, int theHeight,
                    bool flipped = false);

        void update(const void *data, GLenum dataType, GLenum format, int theWidth, int theHeight,
                    bool flipped = false);

        //! the width of the texture in pixels
        uint32_t width() const;
        //! the height of the texture in pixels
        uint32_t height() const;
        //! the depth of the texture in pixels
        uint32_t depth() const;

        //! the size of the texture in pixels
        const ivec2 size() const { return ivec2(width(), height()); }

        //! the aspect ratio of the texture (width / height)
        float aspect_ratio() const { return width() / (float)height(); }

        //! whether the texture has an alpha channel
        bool has_alpha() const;

        //! the Texture's internal format, which is the format that OpenGL stores the texture data in memory. Common values include \c GL_RGB, \c GL_RGBA and \c GL_LUMINANCE
        GLint internal_format() const;

        //! the ID number for the texture, appropriate to pass to calls like \c glBindTexture()
        GLuint id() const;

        GLenum datatype() const;

        //! the target associated with texture. Typical values are \c GL_TEXTURE_2D and \c GL_TEXTURE_RECTANGLE_ARB
        GLenum target() const;

        //!	whether the texture is flipped vertically
        bool flipped() const;

        //!	Marks the texture as being flipped vertically or not
        void set_flipped(bool the_flip = true);

        void set_uvw_offset(const glm::vec3 &the_uvw_offset);

        const glm::vec3& uvw_offset() const;

        //! retrieve the current scale applied to texture-coordinates
        const gl::vec2& uv_scale() const;

        //! set the current scale applied to texture-coordinates
        void set_uv_scale(const gl::vec2 &the_scale);

        //!	set a region of interest (subimage), this function will alter the texture matrix appropriately
        void set_roi(int the_x, int the_y, uint32_t the_width, uint32_t the_height);

        //!	set a region of interest (subimage), this function will alter the texture matrix appropriately
        void set_roi(const crocore::Area_<uint32_t> &the_roi);

        /**	\brief Enables the Texture's target and binds its associated texture.
         Equivalent to calling \code glEnable(target); glBindTexture(target, textureID); \endcode **/
        void enable_and_bind() const;

        //!	Disables the Texture's target
        void disable() const;

        //!	Binds the Texture's texture to its target in the multitexturing unit \c GL_TEXTURE0 + \a textureUnit
        void bind(GLuint textureUnit = 0) const;

        //!	Unbinds the Texture currently bound in the Texture's target
        void unbind(GLuint textureUnit = 0) const;

        //! Returns whether a given OpenGL dataFormat contains an alpha channel
        static bool	data_format_has_alpha(GLint the_format);

        //! Returns whether a give OpenGL dataFormat contains color channels
        static bool data_format_has_color(GLint the_format);

        //! Creates a clone of this texture which does not have ownership, but points to the same resource
        Texture weak_clone() const;

    private:

        void init(const void *data, GLint dataFormat, const Format &format);

        gl::vec2 m_texcoord_scale = gl::vec2(1.f);
        gl::vec3 m_uvw_offset = gl::vec3(0.f);
        std::shared_ptr<struct TextureImpl> m_impl;

    public:
        //! Emulates shared_ptr-like behavior
        explicit operator bool() const { return m_impl.get(); }
        void reset() { m_impl.reset(); }
    };

class TextureDataExc : public std::runtime_error
    {
    public:
        TextureDataExc(const std::string &log):std::runtime_error("TextureData Error: " + log){};
    };

}}// namespace
