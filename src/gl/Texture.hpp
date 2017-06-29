// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "core/Image.hpp"
#include "gl/gl.hpp"

namespace kinski{ namespace gl{
    
    typedef unsigned int GLenum;
    
    /** \brief Represents an OpenGL Texture. \ImplShared*/
    class KINSKI_API Texture
    {
    public:
        struct Format;
        
        //! Default initializer.
        Texture(){};
        /** \brief Constructs a texture of size(\a aWidth, \a aHeight), storing the data in internal format \a aInternalFormat. **/
        Texture(int aWidth, int aHeight, Format format = Format());
        
        Texture(int aWidth, int aHeight, int aDepth, Format format = Format());
        
        /** \brief Constructs a texture of size(\a aWidth, \a aHeight), storing the data in internal format \a aInternalFormat. Pixel data is provided by \a data and is expected to be interleaved and in format \a dataFormat, for which \c GL_RGB or \c GL_RGBA would be typical values. **/
        Texture(const unsigned char *data, int dataFormat, int aWidth, int aHeight, Format format = Format());
        
        Texture(const unsigned char *data, int dataFormat, int aWidth, int aHeight, int aDepth,
                Format format = Format());
        
        //! Constructs a Texture based on an externally initialized OpenGL texture. \a aDoNotDispose specifies whether the Texture is responsible for disposing of the associated OpenGL resource.
        Texture(GLenum aTarget, GLuint aTextureID, int aWidth, int aHeight, bool aDoNotDispose);
        
        Texture(GLenum aTarget, GLuint aTextureID, int aWidth, int aHeight, int aDepth, bool aDoNotDispose);
        
        //! Determines whether the Texture will call glDeleteTextures() to free the associated texture objects on destruction
        void set_do_not_dispose(bool do_not_dispose = true);
        
        //! Installs an optional callback which fires when the texture is destroyed. Useful for integrating with external APIs
        void set_deallocator(void(*the_deallocator_func)(void *), void *the_deallocator_refcon);
        
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
        
        void set_texture_matrix(const mat4 &the_matrix);
        
        void set_swizzle(GLint red, GLint green, GLint blue, GLint alpha);
        
        mat4 texture_matrix() const;
        
        const bool is_bound() const;
        const GLint bound_texture_unit() const;
        
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
        const vec2 size() const { return vec2(width(), height()); }
        
        //! the aspect ratio of the texture (width / height)
        float aspect_ratio() const { return width() / (float)height(); }
        
        //! whether the texture has an alpha channel
        bool has_alpha() const;
        
        //! the Texture's internal format, which is the format that OpenGL stores the texture data in memory. Common values include \c GL_RGB, \c GL_RGBA and \c GL_LUMINANCE
        GLint internal_format() const;
        
        //! the ID number for the texture, appropriate to pass to calls like \c glBindTexture()
        GLuint id() const;
        
        //! the target associated with texture. Typical values are \c GL_TEXTURE_2D and \c GL_TEXTURE_RECTANGLE_ARB
        GLenum target() const;
        
        //!	whether the texture is flipped vertically
        bool flipped() const;
        
        //!	Marks the texture as being flipped vertically or not
        void set_flipped(bool the_flip = true);
        
        //!	set a region of interest (subimage), this function will alter the texture matrix appropriately
        void set_roi(int the_x, int the_y, uint32_t the_width, uint32_t the_height);
        
        //!	set a region of interest (subimage), this function will alter the texture matrix appropriately
        void set_roi(const Area_<uint32_t> &the_roi);
        
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
        
        struct Format
        {
            //! Default constructor, sets the target to \c GL_TEXTURE_2D, wrap to \c GL_CLAMP, disables mipmapping, the internal format to "automatic"
            Format();
            
            //! Specifies the texture's target. The default is \c GL_TEXTURE_2D
            void set_target(GLenum the_target) { m_target = the_target; }
            
            //! Specifies the texture's datatype. The default is GL_UNSIGNED_BYTE
            void set_data_type(GLint the_data_type) { m_datatype = the_data_type; }
            
            //! Enables or disables mipmapping. Default is disabled.
            void set_mipmapping(bool the_mip_map = true) { m_mipmapping = the_mip_map; }
            
            //! Enables or disables mipmapping. Default is disabled.
            void set_anisotropic_filter(float level) { m_anisotropic_filter_level = level; }
            
            //! Sets the Texture's internal format. A value of -1 implies selecting the best format for the context. 
            void set_internal_format(GLint the_internal_format) { m_internal_format = the_internal_format; }
            
            
            //! Sets the wrapping behavior when a texture coordinate falls outside the range of [0,1]. Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. The default is \c GL_CLAMP
            void set_wrap(GLenum the_wrap_s, GLenum the_wrap_t)
            {
                set_wrap_s(the_wrap_s); set_wrap_t(the_wrap_t);
            }
            
            /** \brief Sets the horizontal wrapping behavior when a texture coordinate falls outside the range of [0,1].
             Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. The default is \c GL_CLAMP_TO_EDGE **/
            void set_wrap_s(GLenum the_wrap_s) { m_wrap_s = the_wrap_s; }
            
            /** \brief Sets the verical wrapping behavior when a texture coordinate falls outside the range of [0,1].
             Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. The default is \c GL_CLAMP_TO_EDGE. **/
            void set_wrap_t(GLenum the_wrap_t) { m_wrap_t = the_wrap_t; }
            
            /** \brief Sets the filtering behavior when a texture is displayed at a lower resolution than its native resolution. Default is \c GL_LINEAR
             * Possible values are \li \c GL_NEAREST \li \c GL_LINEAR \li \c GL_NEAREST_MIPMAP_NEAREST \li \c GL_LINEAR_MIPMAP_NEAREST \li \c GL_NEAREST_MIPMAP_LINEAR \li \c GL_LINEAR_MIPMAP_LINEAR **/
            void set_min_filter(GLenum the_min_filter) { m_min_filter = the_min_filter; }
            
            /** Sets the filtering behavior when a texture is displayed at a higher resolution than its native resolution. Default is \c GL_LINEAR
             * Possible values are \li \c GL_NEAREST \li \c GL_LINEAR \li \c GL_NEAREST_MIPMAP_NEAREST \li \c GL_LINEAR_MIPMAP_NEAREST \li \c GL_NEAREST_MIPMAP_LINEAR \li \c GL_LINEAR_MIPMAP_LINEAR **/
            void set_mag_filter(GLenum the_mag_filter) { m_mag_filter = the_mag_filter; }
            
            //! Returns the texture's target
            GLenum target() const { return m_target; }
            
            //! Returns whether the texture has mipmapping enabled
            bool has_mipmapping() const { return m_mipmapping; }
            
            //! Returns the Texture's internal format. A value of -1 implies automatic selection of the internal format based on the context.
            GLint internal_format() const { return m_internal_format; }
            
            //! Returns the horizontal wrapping behavior for the texture coordinates.
            GLenum wrap_s() const { return m_wrap_s; }
            
            //! Returns the vertical wrapping behavior for the texture coordinates.
            GLenum wrap_t() const { return m_wrap_t; }
            
            //! Returns the texture minifying function, which is used whenever the pixel being textured maps to an area greater than one texture element.
            GLenum min_filter() const { return m_min_filter; }
            
            //! Returns the texture magnifying function, which is used whenever the pixel being textured maps to an area less than or equal to one texture element.
            GLenum mag_filter() const { return m_mag_filter; }
            
            GLenum m_target;
            GLenum m_wrap_s, m_wrap_t;
            GLenum m_min_filter, m_mag_filter;
            float m_anisotropic_filter_level;
            bool m_mipmapping;
            GLint m_internal_format, m_datatype;
            
        };
        
    private:

        void init(const void *data, GLint dataFormat, const Format &format);
        
        mat4 m_textureMatrix;
        std::shared_ptr<struct TextureImpl> m_impl;
        
    public:
        //! Emulates shared_ptr-like behavior
        operator bool() const { return m_impl.get(); }
        void reset() { m_impl.reset(); }
    };
    
    class TextureDataExc : public Exception
    {
    public:	
        TextureDataExc(const std::string &log):Exception("TextureData Error: " + log){};
    };
    
    /*********************************** inbuilt Texture loading **********************************/
    
    KINSKI_API Texture create_texture_from_file(const std::string &theFileName,
                                                bool mipmap = false,
                                                bool compress = false,
                                                GLfloat anisotropic_filter_lvl = 1.f);
    
    KINSKI_API Texture create_texture_from_image(const ImagePtr &the_img, bool mipmap = false,
                                                 bool compress = false,
                                                 GLfloat anisotropic_filter_lvl = 1.f);
    
    KINSKI_API Texture create_texture_from_data(const std::vector<uint8_t> &the_data,
                                                bool mipmap = false,
                                                bool compress = false,
                                                GLfloat anisotropic_filter_lvl = 1.f);
    
    KINSKI_API ImagePtr create_image_from_texture(const gl::Texture &the_texture);

    
}}// namespace
