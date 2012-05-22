/*
 Copyright (c) 2010, The Barbarian Group
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:
 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and
 the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <OpenGL/gl.h>
#include <boost/shared_ptr.hpp>

#include <boost/static_assert.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using boost::shared_ptr;

namespace gl 
{
    
    typedef unsigned int GLenum;
    
    /** \brief Represents an OpenGL Texture. \ImplShared*/
    class Texture {
    public:
        struct Format;
        
        //! Default initializer. Points to a null Obj
        Texture() {}
        /** \brief Constructs a texture of size(\a aWidth, \a aHeight), storing the data in internal format \a aInternalFormat. **/
        Texture( int aWidth, int aHeight, Format format = Format() );
        
        /** \brief Constructs a texture of size(\a aWidth, \a aHeight), storing the data in internal format \a aInternalFormat. Pixel data is provided by \a data and is expected to be interleaved and in format \a dataFormat, for which \c GL_RGB or \c GL_RGBA would be typical values. **/
        Texture( const unsigned char *data, int dataFormat, int aWidth, int aHeight, Format format = Format() );
        
        //! Constructs a Texture based on an externally initialized OpenGL texture. \a aDoNotDispose specifies whether the Texture is responsible for disposing of the associated OpenGL resource.
        Texture( GLenum aTarget, GLuint aTextureID, int aWidth, int aHeight, bool aDoNotDispose );
        
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
        
        /** Designed to accommodate texture where not all pixels are "clean", meaning the maximum texture coordinate value may not be 1.0 (or the texture's width in \c GL_TEXTURE_RECTANGLE_ARB) **/
        void setCleanTexCoords( float maxU, float maxV );
        
        void setTextureMatrix( const glm::mat4 &theMatrix );
        
        const glm::mat4 &getTextureMatrix() const;
        
        const bool isBound() const;
        const GLint getBoundTextureUnit() const;
        
        //	//! Replaces the pixels of a texture with contents of \a surface. Expects \a surface's size to match the Texture's.
        //	void			update( const Surface &surface );
        //	//! Replaces the pixels of a texture with contents of \a surface. Expects \a surface's size to match the Texture's.
        //	void			update( const Surface32f &surface );
        //	/** \brief Replaces the pixels of a texture with contents of \a surface. Expects \a area's size to match the Texture's.
        //		\todo Method for updating a subrectangle with an offset into the source **/
        //	void			update( const Surface &surface, const Area &area );
        //	//! Replaces the pixels of a texture with contents of \a channel. Expects \a channel's size to match the Texture's.
        //	void			update( const Channel32f &channel );
        //	//! Replaces the pixels of a texture with contents of \a channel. Expects \a area's size to match the Texture's.
        //	void			update( const Channel8u &channel, const Area &area );
        
        //! Replaces the pixels of a texture with \a data
        void update( const unsigned char *data,GLenum format, int theWidth, int theHeight, bool flipped = false );
        
        //! the width of the texture in pixels
        GLint getWidth() const;
        //! the height of the texture in pixels
        GLint getHeight() const;
        //! the width of the texture in pixels accounting for its "clean" area - \sa getCleanBounds()
        GLint getCleanWidth() const;
        //! the height of the texture in pixels accounting for its "clean" area - \sa getCleanBounds()
        GLint getCleanHeight() const;
        
        //! the size of the texture in pixels
        //const Eigen::Vector2i getSize() const { return Eigen::Vector2i( getWidth(), getHeight() ); }	
        const glm::vec2 getSize() const { return glm::vec2( getWidth(), getHeight() ); }	
        
        //! the aspect ratio of the texture (width / height)
        float getAspectRatio() const { return getWidth() / (float)getHeight(); }
        
        //! the Area defining the Texture's bounds in pixels: [0,0]-[width,height]
//        Area getBounds() const { return Area( 0, 0, getWidth(), getHeight() ); }
//        //! the Area defining the Texture's clean pixel bounds in pixels: [0,0]-[width*maxU,height*maxV]
//        Area getCleanBounds() const { return Area( 0, 0, getCleanWidth(), getCleanHeight() ); }
        
        //! whether the texture has an alpha channel
        bool hasAlpha() const;
        
        //!	These return the right thing even when the texture coordinate space is flipped
        float getLeft() const;
        float getRight() const;	
        float getTop() const;		
        float getBottom() const;
        
        //!	These do not correspond to "top" and "right" when the texture is flipped
        float getMaxU() const;
        float getMaxV() const;
        
        //	//! Returns the UV coordinates which correspond to the pixels contained in \a area. Does not compensate for clean coordinates. Does compensate for flipping.
        //	Rectf			getAreaTexCoords( const Area &area ) const;
        
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
        
        struct Format {
            //! Default constructor, sets the target to \c GL_TEXTURE_2D, wrap to \c GL_CLAMP, disables mipmapping, the internal format to "automatic"
            Format();
            
            //! Specifies the texture's target. The default is \c GL_TEXTURE_2D
            void	setTarget( GLenum target ) { m_Target = target; }
            
            //! Sets the texture's target to be \c GL_TEXTURE_RECTANGLE_ARB. Not available in OpenGL ES.
            
            //#ifndef CINDER_GLES
            //		void	setTargetRect() { m_Target = GL_TEXTURE_RECTANGLE_ARB; }
            //#endif		
            
            //! Enables or disables mipmapping. Default is disabled.
            void enableMipmapping( bool enableMipmapping = true ) { m_Mipmapping = enableMipmapping; }
            
            //! Sets the Texture's internal format. A value of -1 implies selecting the best format for the context. 
            void setInternalFormat( GLint internalFormat ) { m_InternalFormat = internalFormat; }
            
            //! Sets the Texture's internal format to be automatically selected based on the context.
            void setAutoInternalFormat() { m_InternalFormat = -1; }		
            
            //! Sets the wrapping behavior when a texture coordinate falls outside the range of [0,1]. Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. The default is \c GL_CLAMP
            void setWrap( GLenum wrapS, GLenum wrapT ) { setWrapS( wrapS ); setWrapT( wrapT ); }
            
            /** \brief Sets the horizontal wrapping behavior when a texture coordinate falls outside the range of [0,1].
             Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. The default is \c GL_CLAMP_TO_EDGE **/
            void setWrapS( GLenum wrapS ) { m_WrapS = wrapS; }
            
            /** \brief Sets the verical wrapping behavior when a texture coordinate falls outside the range of [0,1].
             Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. The default is \c GL_CLAMP_TO_EDGE. **/
            void setWrapT( GLenum wrapT ) { m_WrapT = wrapT; }
            
            /** \brief Sets the filtering behavior when a texture is displayed at a lower resolution than its native resolution. Default is \c GL_LINEAR
             * Possible values are \li \c GL_NEAREST \li \c GL_LINEAR \li \c GL_NEAREST_MIPMAP_NEAREST \li \c GL_LINEAR_MIPMAP_NEAREST \li \c GL_NEAREST_MIPMAP_LINEAR \li \c GL_LINEAR_MIPMAP_LINEAR **/
            void setMinFilter( GLenum minFilter ) { m_MinFilter = minFilter; }
            
            /** Sets the filtering behavior when a texture is displayed at a higher resolution than its native resolution. Default is \c GL_LINEAR
             * Possible values are \li \c GL_NEAREST \li \c GL_LINEAR \li \c GL_NEAREST_MIPMAP_NEAREST \li \c GL_LINEAR_MIPMAP_NEAREST \li \c GL_NEAREST_MIPMAP_LINEAR \li \c GL_LINEAR_MIPMAP_LINEAR **/
            void setMagFilter( GLenum magFilter ) { m_MagFilter = magFilter; }
            
            //! Returns the texture's target
            GLenum	getTarget() const { return m_Target; }
            
            //! Returns whether the texture has mipmapping enabled
            bool	hasMipmapping() const { return m_Mipmapping; }
            
            //! Returns the Texture's internal format. A value of -1 implies automatic selection of the internal format based on the context.
            GLint	getInternalFormat() const { return m_InternalFormat; }
            
            //! Returns whether the Texture's internal format will be automatically selected based on the context.
            bool	isAutoInternalFormat() const { return m_InternalFormat == -1; }
            
            //! Returns the horizontal wrapping behavior for the texture coordinates.
            GLenum	getWrapS() const { return m_WrapS; }
            
            //! Returns the vertical wrapping behavior for the texture coordinates.
            GLenum	getWrapT() const { return m_WrapT; }
            
            //! Returns the texture minifying function, which is used whenever the pixel being textured maps to an area greater than one texture element.
            GLenum	getMinFilter() const { return m_MinFilter; }
            
            //! Returns the texture magnifying function, which is used whenever the pixel being textured maps to an area less than or equal to one texture element.
            GLenum	getMagFilter() const { return m_MagFilter; }
            
        protected:
            GLenum			m_Target;
            GLenum			m_WrapS, m_WrapT;
            GLenum			m_MinFilter, m_MagFilter;
            bool			m_Mipmapping;
            GLint			m_InternalFormat;
            
            friend class Texture;
        };
        
    protected:
        void	init( const unsigned char *data, int unpackRowLength, GLenum dataFormat, GLenum type, const Format &format );	
        void	init( const float *data, GLint dataFormat, const Format &format );
        
        // forward declared Implementation object
        struct Obj;
        typedef shared_ptr<Obj> ObjPtr;
        ObjPtr m_Obj;
        
    public:
        //@{
        //! Emulates shared_ptr-like behavior
        typedef ObjPtr Texture::*unspecified_bool_type;
        operator unspecified_bool_type() const { return ( m_Obj.get() == 0 ) ? 0 : &Texture::m_Obj; }
        void reset() { m_Obj.reset(); }
        //@}  
    };
    
    class TextureDataExc : public std::exception {
    public:	
        TextureDataExc( const std::string &log ) throw();
        virtual const char* what() const throw()
        {
            return mMessage;
        }
        
    private:
        char	mMessage[16001];
        GLint	mShaderType;
    };
    
    template<typename T> class scoped_bind
    {
    public:
        scoped_bind():
        m_obj(NULL),m_isBound(false)
        {}
        
        explicit scoped_bind(T &theObj):
        m_obj(&theObj),m_isBound(false)
        {
            bind();
        }
        
        ~scoped_bind()
        {
            if(m_isBound)
                unbind();
        }
        
        inline void bind()
        {
            m_obj->bind();
            m_isBound = true;
        }
        
        inline void unbind()
        {
            m_obj->unbind();
            m_isBound = false;
        }
        
    private:
        
        T *m_obj;
        bool m_isBound;
    };
    
}