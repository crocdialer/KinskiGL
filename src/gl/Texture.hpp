// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "gl/gl.hpp"

namespace kinski{ namespace gl{
    
    typedef unsigned int GLenum;
    
    template<typename T>
    class Area
    {
    public:
        T x1, y1, x2, y2;
        
        Area():x1(0), y1(0), x2(0), y2(0){};
        Area(const T &theX1, const T &theY1, const T &theX2, const T &theY2):
        x1(theX1), y1(theY1), x2(theX2), y2(theY2){};
        
        // does not seem to work in std::map !?
        bool operator<(const Area<T> &other) const
        {
            if ( x1 != other.x1 ) return x1 < other.x1;
            if ( y1 != other.y1 ) return y1 < other.y1;
            if ( x2 != other.x2 ) return x2 < other.x2;
            if ( y2 != other.y2 ) return y2 < other.y2;
            return false;
        }
                
        inline uint32_t width() const { return x2 - x1; };
        inline uint32_t height() const { return y2 - y1; };
    };
    
    class Image
    {
    public:
        
        uint8_t* data = nullptr;
        uint32_t rows = 0, cols = 0;
        uint32_t bytes_per_pixel = 1;
        Area<uint32_t> roi;
        bool do_not_dispose = false;
        
        static ImagePtr create()
        {
            return ImagePtr(new Image());
        };
        
        static ImagePtr create(uint8_t* theData, uint32_t theRows, uint32_t theCols, uint32_t theBytesPerPixel = 1,
                        const Area<uint32_t> &theRoi = Area<uint32_t>())
        {
            return ImagePtr(new Image(theData, theRows, theCols, theBytesPerPixel, theRoi));
        };
        
        inline uint8_t* data_start_for_roi() const {return data + (roi.y1 * cols + roi.x1) * bytes_per_pixel;}
        
        inline size_t num_bytes() const { return rows * cols * bytes_per_pixel; }
        
        ~Image()
        {
            if(data && !do_not_dispose)
            {
                LOG_TRACE_2 << "disposing image";
                free(data);
            }
        };
        
    private:
        Image():data(nullptr), rows(0), cols(0), bytes_per_pixel(0){};
        
        Image(uint8_t* theData, uint32_t theRows, uint32_t theCols, uint32_t theBytesPerPixel = 1,
              const Area<uint32_t> &theRoi = Area<uint32_t>()):
        data(theData), rows(theRows), cols(theCols), bytes_per_pixel(theBytesPerPixel), roi(theRoi){};
    };
                
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
        Texture(const unsigned char *data, int dataFormat, int aWidth, int aHeight, Format format = Format() );
        
        Texture(const unsigned char *data, int dataFormat, int aWidth, int aHeight, int aDepth,
                Format format = Format() );
        
        //! Constructs a Texture based on an externally initialized OpenGL texture. \a aDoNotDispose specifies whether the Texture is responsible for disposing of the associated OpenGL resource.
        Texture( GLenum aTarget, GLuint aTextureID, int aWidth, int aHeight, bool aDoNotDispose );
        
        Texture( GLenum aTarget, GLuint aTextureID, int aWidth, int aHeight, int aDepth, bool aDoNotDispose );
        
        //! Determines whether the Texture will call glDeleteTextures() to free the associated texture objects on destruction
        void setDoNotDispose( bool aDoNotDispose = true );
        
        //! Installs an optional callback which fires when the texture is destroyed. Useful for integrating with external APIs
        void setDeallocator( void(*aDeallocatorFunc)( void * ), void *aDeallocatorRefcon );
        
        //! Sets the wrapping behavior when a texture coordinate falls outside the range of [0,1]. Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE.
        void setWrap( GLenum wrapS, GLenum wrapT ) { setWrapS( wrapS ); setWrapT( wrapT ); }
        
        /** \brief Sets the horizontal wrapping behavior when a texture coordinate falls outside the range of [0,1].
         Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. **/
        void setWrapS( GLenum wrapS );
        
        /** \brief Sets the verical wrapping behavior when a texture coordinate falls outside the range of [0,1].
         Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. **/
        void setWrapT( GLenum wrapT );
        
        /** \brief Sets the filtering behavior when a texture is displayed at a lower resolution than its native resolution.
         * Possible values are \li \c GL_NEAREST \li \c GL_LINEAR \li \c GL_NEAREST_MIPMAP_NEAREST \li \c GL_LINEAR_MIPMAP_NEAREST \li \c GL_NEAREST_MIPMAP_LINEAR \li \c GL_LINEAR_MIPMAP_LINEAR **/
        void setMinFilter( GLenum minFilter );	
        
        /** Sets the filtering behavior when a texture is displayed at a higher resolution than its native resolution.
         * Possible values are \li \c GL_NEAREST \li \c GL_LINEAR \li \c GL_NEAREST_MIPMAP_NEAREST \li \c GL_LINEAR_MIPMAP_NEAREST \li \c GL_NEAREST_MIPMAP_LINEAR \li \c GL_LINEAR_MIPMAP_LINEAR **/
        void setMagFilter( GLenum magFilter );
        
        void set_mipmapping( bool b = true );
        void set_anisotropic_filter(float f);
        
        void setTextureMatrix( const mat4 &theMatrix );
        
        void set_swizzle(GLint red, GLint green, GLint blue, GLint alpha);
        
        mat4 getTextureMatrix() const;
        
        const bool isBound() const;
        const GLint getBoundTextureUnit() const;
        
        //! Replaces the pixels of a texture with \a data
        void update( const uint8_t *data,GLenum format, int theWidth, int theHeight, bool flipped = false );
        
        //! Replaces the pixels of a texture with \a data
        void update( const float *data, GLenum format, int theWidth, int theHeight, bool flipped = false );
        
        void update(const void *data, GLenum dataType, GLenum format, int theWidth, int theHeight,
                    bool flipped = false);
        
        //! the width of the texture in pixels
        GLint getWidth() const;
        //! the height of the texture in pixels
        GLint getHeight() const;
        //! the depth of the texture in pixels
        GLint getDepth() const;
        
        //! the size of the texture in pixels
        const vec2 getSize() const { return vec2( getWidth(), getHeight() ); }	
        
        //! the aspect ratio of the texture (width / height)
        float getAspectRatio() const { return getWidth() / (float)getHeight(); }
        
        //! whether the texture has an alpha channel
        bool hasAlpha() const;
        
        //!	These return the right thing even when the texture coordinate space is flipped
        float getLeft() const;
        float getRight() const;	
        float getTop() const;		
        float getBottom() const;
        
        //! the Texture's internal format, which is the format that OpenGL stores the texture data in memory. Common values include \c GL_RGB, \c GL_RGBA and \c GL_LUMINANCE
        GLint getInternalFormat() const;
        
        //! the ID number for the texture, appropriate to pass to calls like \c glBindTexture()
        GLuint getId() const;
        
        //! the target associated with texture. Typical values are \c GL_TEXTURE_2D and \c GL_TEXTURE_RECTANGLE_ARB
        GLenum getTarget() const;
        
        //!	whether the texture is flipped vertically
        bool isFlipped() const;
        
        //!	Marks the texture as being flipped vertically or not
        void setFlipped( bool aFlipped = true );
        
        //!	set a region of interest (subimage), this function will alter the texture matrix appropriately
        void set_roi(int x, int y, uint32_t width, uint32_t height);
        
        //!	set a region of interest (subimage), this function will alter the texture matrix appropriately
        void set_roi(const Area<uint32_t> &the_roi);
        
        /**	\brief Enables the Texture's target and binds its associated texture.
         Equivalent to calling \code glEnable( target ); glBindTexture( target, textureID ); \endcode **/
        void enableAndBind() const;
        
        //!	Disables the Texture's target
        void disable() const;
        
        //!	Binds the Texture's texture to its target in the multitexturing unit \c GL_TEXTURE0 + \a textureUnit
        void bind( GLuint textureUnit = 0 ) const;
        
        //!	Unbinds the Texture currently bound in the Texture's target
        void unbind( GLuint textureUnit = 0 ) const;
        
        //! Returns whether a given OpenGL dataFormat contains an alpha channel
        static bool	dataFormatHasAlpha( GLint dataFormat );
        
        //! Returns whether a give OpenGL dataFormat contains color channels
        static bool dataFormatHasColor( GLint dataFormat );
        
        //! Creates a clone of this texture which does not have ownership, but points to the same resource
        Texture weakClone() const;
        
        struct Format
        {
            //! Default constructor, sets the target to \c GL_TEXTURE_2D, wrap to \c GL_CLAMP, disables mipmapping, the internal format to "automatic"
            Format();
            
            //! Specifies the texture's target. The default is \c GL_TEXTURE_2D
            void setTarget( GLenum target ) { m_target = target; }
            
            //! Specifies the texture's datatype. The default is GL_UNSIGNED_BYTE
            void set_data_type(GLint the_data_type) { m_datatype = the_data_type; }
            
            //! Enables or disables mipmapping. Default is disabled.
            void set_mipmapping( bool b = true ) { m_Mipmapping = b; }
            
            //! Enables or disables mipmapping. Default is disabled.
            void set_anisotropic_filter(float level) { m_anisotropic_filter_level = level; }
            
            //! Sets the Texture's internal format. A value of -1 implies selecting the best format for the context. 
            void setInternalFormat( GLint internalFormat ) { m_InternalFormat = internalFormat; }
            
            //! Sets the Texture's internal format to be automatically selected based on the context.
            void setAutoInternalFormat() { m_InternalFormat = -1; }		
            
            //! Sets the wrapping behavior when a texture coordinate falls outside the range of [0,1]. Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. The default is \c GL_CLAMP
            void setWrap( GLenum wrapS, GLenum wrapT ) { setWrapS( wrapS ); setWrapT( wrapT ); }
            
            /** \brief Sets the horizontal wrapping behavior when a texture coordinate falls outside the range of [0,1].
             Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. The default is \c GL_CLAMP_TO_EDGE **/
            void setWrapS( GLenum wrapS ) { m_wrap_s = wrapS; }
            
            /** \brief Sets the verical wrapping behavior when a texture coordinate falls outside the range of [0,1].
             Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. The default is \c GL_CLAMP_TO_EDGE. **/
            void setWrapT( GLenum wrapT ) { m_wrap_t = wrapT; }
            
            /** \brief Sets the filtering behavior when a texture is displayed at a lower resolution than its native resolution. Default is \c GL_LINEAR
             * Possible values are \li \c GL_NEAREST \li \c GL_LINEAR \li \c GL_NEAREST_MIPMAP_NEAREST \li \c GL_LINEAR_MIPMAP_NEAREST \li \c GL_NEAREST_MIPMAP_LINEAR \li \c GL_LINEAR_MIPMAP_LINEAR **/
            void setMinFilter( GLenum minFilter ) { m_MinFilter = minFilter; }
            
            /** Sets the filtering behavior when a texture is displayed at a higher resolution than its native resolution. Default is \c GL_LINEAR
             * Possible values are \li \c GL_NEAREST \li \c GL_LINEAR \li \c GL_NEAREST_MIPMAP_NEAREST \li \c GL_LINEAR_MIPMAP_NEAREST \li \c GL_NEAREST_MIPMAP_LINEAR \li \c GL_LINEAR_MIPMAP_LINEAR **/
            void setMagFilter( GLenum magFilter ) { m_MagFilter = magFilter; }
            
            //! Returns the texture's target
            GLenum	getTarget() const { return m_target; }
            
            //! Returns whether the texture has mipmapping enabled
            bool	hasMipmapping() const { return m_Mipmapping; }
            
            //! Returns the Texture's internal format. A value of -1 implies automatic selection of the internal format based on the context.
            GLint	getInternalFormat() const { return m_InternalFormat; }
            
            //! Returns whether the Texture's internal format will be automatically selected based on the context.
            bool	isAutoInternalFormat() const { return m_InternalFormat == -1; }
            
            //! Returns the horizontal wrapping behavior for the texture coordinates.
            GLenum	getWrapS() const { return m_wrap_s; }
            
            //! Returns the vertical wrapping behavior for the texture coordinates.
            GLenum	getWrapT() const { return m_wrap_t; }
            
            //! Returns the texture minifying function, which is used whenever the pixel being textured maps to an area greater than one texture element.
            GLenum	getMinFilter() const { return m_MinFilter; }
            
            //! Returns the texture magnifying function, which is used whenever the pixel being textured maps to an area less than or equal to one texture element.
            GLenum	getMagFilter() const { return m_MagFilter; }
            
            GLenum			m_target;
            GLenum			m_wrap_s, m_wrap_t;
            GLenum			m_MinFilter, m_MagFilter;
            float           m_anisotropic_filter_level;
            bool			m_Mipmapping;
            GLint			m_InternalFormat, m_datatype;
            
        };
        
    private:

        void init(const void *data, GLint dataFormat, const Format &format);
        
        mat4           m_textureMatrix;
        
        // forward declared Implementation object
        struct Obj;
        typedef std::shared_ptr<Obj> ObjPtr;
        ObjPtr m_Obj;
        
    public:
        //! Emulates shared_ptr-like behavior
        operator bool() const { return m_Obj.get() != nullptr; }
        void reset() { m_Obj.reset(); }
    };
    
    class TextureDataExc : public Exception
    {
    public:	
        TextureDataExc(const std::string &log):Exception("TextureData Error: " + log){};
    };
}}// namespace