// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef KINSKI_GL_FBO_H_
#define KINSKI_GL_FBO_H_

#include <vector>
#include "Texture.h"

namespace kinski {
namespace gl {

//! Represents an OpenGL Renderbuffer, used primarily in conjunction with FBOs. Supported on OpenGL ES but multisampling is currently ignored. \ImplShared
class KINSKI_API Renderbuffer
{
 public:
	//! Creates a NULL Renderbuffer
	Renderbuffer() {}
	//! Create a Renderbuffer \a width pixels wide and \a heigh pixels high, with an internal format of \a internalFormat, defaulting to GL_RGBA8
#if defined( KINSKI_GLES )
	Renderbuffer(int width, int height, GLenum internalFormat = GL_RGBA8_OES);
#else
	Renderbuffer(int width, int height, GLenum internalFormat = GL_RGBA8);
#endif
	//! Create a Renderbuffer \a width pixels wide and \a heigh pixels high, with an internal format of \a internalFormat, defaulting to GL_RGBA8, MSAA samples \a msaaSamples
	Renderbuffer(int width, int height, GLenum internalFormat, int msaaSamples);

	//! Returns the width of the Renderbuffer in pixels
	int getWidth() const { return m_obj->mWidth; }
    
	//! Returns the height of the Renderbuffer in pixels
	int getHeight() const { return m_obj->mHeight; }
    
	//! Returns the size of the Renderbuffer in pixels
    glm::ivec2 getSize() const { return glm::ivec2(m_obj->mWidth, m_obj->mHeight); }
    
	//! Returns the bounding area of the Renderbuffer in pixels
	//Area	getBounds() const { return Area( 0, 0, m_obj->mWidth, m_obj->mHeight ); }
    
	//! Returns the aspect ratio of the Renderbuffer
	float getAspectRatio() const { return m_obj->mWidth / (float)m_obj->mHeight; }

	//! Returns the ID of the Renderbuffer
	GLuint	getId() const { return m_obj->mId; }
	//! Returns the internal format of the Renderbuffer
	GLenum	getInternalFormat() const { return m_obj->mInternalFormat; }
	//! Returns the number of samples used in MSAA-style antialiasing. Defaults to none, disabling multisampling
	int		getSamples() const { return m_obj->mSamples; }
	//! Returns the number of coverage samples used in CSAA-style antialiasing. Defaults to none.
	//int		getCoverageSamples() const { return m_obj->mCoverageSamples; }

  private:
	struct Obj {
		Obj();
		Obj( int aWidth, int aHeight, GLenum internalFormat, int msaaSamples);
		~Obj();

		int					mWidth, mHeight;
		GLuint				mId;
		GLenum				mInternalFormat;
		int					mSamples;
	};
    
    typedef std::shared_ptr<Obj> ObjPtr;
	ObjPtr m_obj;

  public:
  	//@{
	//! Emulates shared_ptr-like behavior
	typedef ObjPtr Renderbuffer::*unspecified_bool_type;
	operator unspecified_bool_type() const { return ( m_obj.get() == 0 ) ? 0 : &Renderbuffer::m_obj; }
	void reset() { m_obj.reset(); }
	//@}  	
};

//! Represents an OpenGL Framebuffer Object. \ImplShared
class KINSKI_API Fbo
{
 public:
	struct Format;

	//! Creates a NULL FBO
	Fbo() {}
    //! Creates an FBO \a width pixels wide and \a height pixels high, using Fbo::Format \a format
    Fbo( const glm::vec2 &the_size, Format format = Format() );
	//! Creates an FBO \a width pixels wide and \a height pixels high, using Fbo::Format \a format
	Fbo( int width, int height, Format format = Format() );
	//! Creates an FBO \a width pixels wide and \a height pixels high, with an optional alpha channel, color buffer and depth buffer
	Fbo( int width, int height, bool alpha, bool color = true, bool depth = true );

	//! Returns the width of the FBO in pixels
	int				getWidth() const { return m_obj->mWidth; }
	//! Returns the height of the FBO in pixels
	int				getHeight() const { return m_obj->mHeight; }
	//! Returns the size of the FBO in pixels
    glm::vec2  getSize() const { return glm::vec2( m_obj->mWidth, m_obj->mHeight ); }
    
	//! Returns the bounding area of the FBO in pixels
	//Area			getBounds() const { return Area( 0, 0, m_obj->mWidth, m_obj->mHeight ); }
    
	//! Returns the aspect ratio of the FBO
	float			getAspectRatio() const { return m_obj->mWidth / (float)m_obj->mHeight; }
	//! Returns the Fbo::Format of this FBO
	const Format&	getFormat() const { return m_obj->mFormat; }
	//! Returns the texture target for this FBO. Typically \c GL_TEXTURE_2D or \c GL_TEXTURE_RECTANGLE_ARB
	GLenum			getTarget() const { return m_obj->mFormat.mTarget; }

	//! Returns a reference to the color texture of the FBO. \a attachment specifies which attachment in the case of multiple color buffers
	Texture		getTexture( int attachment = 0 );
	//! Returns a reference to the depth texture of the FBO.
	Texture&		getDepthTexture();	
	
	//! Binds the color texture associated with an Fbo to its target. Optionally binds to a multitexturing unit when \a textureUnit is non-zero.  \a attachment specifies which color buffer in the case of multiple attachments.
	void 			bindTexture( int textureUnit = 0, int attachment = 0 );
	//! Unbinds the texture associated with an Fbo's target
	void			unbindTexture();
	//! Binds the depth texture associated with an Fbo to its target.
	void 			bindDepthTexture( int textureUnit = 0 );
	//! Binds the Fbo as the currently active framebuffer, meaning it will receive the results of all subsequent rendering until it is unbound
	void 			bindFramebuffer();
	//! Unbinds the Fbo as the currently active framebuffer, restoring the primary context as the target for all subsequent rendering
	static void 	unbindFramebuffer();

	//! Returns the ID of the framebuffer itself. For antialiased FBOs this is the ID of the output multisampled FBO
	GLuint		getId() const { return m_obj->mId; }

#if ! defined( KINSKI_GLES )
//	//! For antialiased FBOs this returns the ID of the mirror FBO designed for reading, where the multisampled render buffers are resolved to. For non-antialised, this is the equivalent to getId()
//	GLuint		getResolveId() const { if( m_obj->mResolveFramebufferId ) return m_obj->mResolveFramebufferId; else return m_obj->mId; }
//
//	//! Copies to FBO \a dst from \a srcArea to \a dstArea using filter \a filter. \a mask allows specification of color (\c GL_COLOR_BUFFER_BIT) and/or depth(\c GL_DEPTH_BUFFER_BIT). Calls glBlitFramebufferEXT() and is subject to its constraints and coordinate system.
//	void		blitTo( Fbo dst, const Area &srcArea, const Area &dstArea, GLenum filter = GL_NEAREST, GLbitfield mask = GL_COLOR_BUFFER_BIT ) const;
//	//! Copies to the screen from Area \a srcArea to \a dstArea using filter \a filter. \a mask allows specification of color (\c GL_COLOR_BUFFER_BIT) and/or depth(\c GL_DEPTH_BUFFER_BIT). Calls glBlitFramebufferEXT() and is subject to its constraints and coordinate system.
//	void		blitToScreen( const Area &srcArea, const Area &dstArea, GLenum filter = GL_NEAREST, GLbitfield mask = GL_COLOR_BUFFER_BIT ) const;

    //! Copies from the screen from Area \a srcArea to \a dstArea using filter \a filter. \a mask allows specification of color (\c GL_COLOR_BUFFER_BIT) and/or depth(\c GL_DEPTH_BUFFER_BIT). Calls glBlitFramebufferEXT() and is subject to its constraints and coordinate system.
    //    void blitFromScreen( const Area &srcArea, const Area &dstArea, GLenum filter = GL_NEAREST, GLbitfield mask = GL_COLOR_BUFFER_BIT );
#endif

	//! Returns the maximum number of samples the graphics card is capable of using per pixel in MSAA for an Fbo
	static GLint	getMaxSamples();
	//! Returns the maximum number of color attachments the graphics card is capable of using for an Fbo
	static GLint	getMaxAttachments();
	
	struct KINSKI_API Format
    {
	  public:
		//! Default constructor, sets the target to \c GL_TEXTURE_2D with an 8-bit color+alpha, a 24-bit depth texture, and no multisampling or mipmapping
		Format();

		//! Set the texture target associated with the FBO. Defaults to \c GL_TEXTURE_2D, \c GL_TEXTURE_RECTANGLE_ARB is a common option as well
		void setTarget(GLenum target) { mTarget = target; }
        
		//! Sets the GL internal format for the color buffer. Defaults to \c GL_RGBA8 (and \c GL_RGBA on OpenGL ES). Common options also include \c GL_RGB8 and \c GL_RGBA32F
		void setColorInternalFormat(GLenum colorInternalFormat){ mColorInternalFormat = colorInternalFormat; }
        
		//! Sets the GL internal format for the depth buffer. Defaults to \c GL_DEPTH_COMPONENT24. Common options also include \c GL_DEPTH_COMPONENT16 and \c GL_DEPTH_COMPONENT32
		void setDepthInternalFormat(GLenum depthInternalFormat) { mDepthInternalFormat = depthInternalFormat; }
        
		//! Sets the number of samples used in MSAA-style antialiasing. Defaults to none, disabling multisampling. Note that not all implementations support multisampling. Ignored on OpenGL ES.
		void setSamples(int samples) { mSamples = samples; }
        
		//! Sets the number of coverage samples used in CSAA-style antialiasing. Defaults to none. Note that not all implementations support CSAA, and is currenlty Windows-only Nvidia. Ignored on OpenGL ES.
		void setCoverageSamples(int coverageSamples) { mCoverageSamples = coverageSamples; }
        
		//! Enables or disables the creation of a color buffer for the FBO.. Creates multiple color attachments when \a numColorsBuffers >1, except on OpenGL ES which supports only 1.
		void enableColorBuffer( bool colorBuffer = true, int numColorBuffers = 1 );
        
		//! Enables or disables the creation of a depth buffer for the FBO. If \a asTexture the depth buffer is created as a gl::Texture, obtainable via getDepthTexture(). Not supported on OpenGL ES.
		void enableDepthBuffer(bool depthBuffer = true, bool asTexture = true);
        
        void enableStencilBuffer(bool stencilBuffer = true) { mStencilBuffer = stencilBuffer; }
        
		//! Enables or disables mip-mapping for the FBO's textures
		void enableMipmapping( bool enableMipmapping = true ) { mMipmapping = enableMipmapping; }

		//! Sets the wrapping behavior for the FBO's textures. Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. Default is \c GL_CLAMP_TO_EDGE.
		void setWrap(GLenum wrapS, GLenum wrapT) { setWrapS(wrapS); setWrapT(wrapT); }
        
		/** \brief Sets the horizontal wrapping behavior for the FBO's textures. Default is \c GL_CLAMP_TO_EDGE.
			Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. **/
		void setWrapS(GLenum wrapS) { mWrapS = wrapS; }
        
		/** \brief Sets the vertical wrapping behavior for the FBO's textures. Default is \c GL_CLAMP_TO_EDGE.
			Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. **/
		void setWrapT(GLenum wrapT) { mWrapT = wrapT; }
        
		/** \brief Sets the minification filtering behavior for the FBO's textures. Default is \c GL_LINEAR:
		 * Possible values are \li \c GL_NEAREST \li \c GL_LINEAR \li \c GL_NEAREST_MIPMAP_NEAREST \li \c GL_LINEAR_MIPMAP_NEAREST \li \c GL_NEAREST_MIPMAP_LINEAR \li \c GL_LINEAR_MIPMAP_LINEAR **/
		void setMinFilter(GLenum minFilter) { mMinFilter = minFilter; }
        
		/** Sets the magnification filtering behavior for the FBO's textures. Default is \c GL_LINEAR:
		 * Possible values are \li \c GL_NEAREST \li \c GL_LINEAR \li \c GL_NEAREST_MIPMAP_NEAREST \li \c GL_LINEAR_MIPMAP_NEAREST \li \c GL_NEAREST_MIPMAP_LINEAR \li \c GL_LINEAR_MIPMAP_LINEAR **/
		void setMagFilter(GLenum magFilter) { mMagFilter = magFilter; }

		//! Returns the texture target associated with the FBO.
		GLenum	getTarget() const { return mTarget; }
        
		//! Returns the GL internal format for the color buffer. Defaults to \c GL_RGBA8.
		GLenum getColorInternalFormat() const { return mColorInternalFormat; }
        
		//! Returns the GL internal format for the depth buffer. Defaults to \c GL_DEPTH_COMPONENT24.
		GLenum getDepthInternalFormat() const { return mDepthInternalFormat; }
        
		//! Returns the number of samples used in MSAA-style antialiasing. Defaults to none, disabling multisampling. OpenGL ES does not support multisampling.
		int getSamples() const { return mSamples; }
        
		//! Returns the number of coverage samples used in CSAA-style antialiasing. Defaults to none. OpenGL ES does not support multisampling.
		int getCoverageSamples() const { return mCoverageSamples; }
        
		//! Returns whether the FBO contains a color buffer
		bool hasColorBuffer() const { return mNumColorBuffers > 0; }
		
		//! Returns whether the FBO contains a depth buffer
		bool hasDepthBuffer() const { return m_depthBuffer; }
        
		//! Returns whether the FBO contains a depth buffer implemened as a texture. Always \c false on OpenGL ES.
		bool hasDepthBufferTexture() const { return m_depthBufferAsTexture; }

        bool hasStencilBuffer() const { return mStencilBuffer; }
        
		//! Returns whether the contents of the FBO textures are mip-mapped.
		bool hasMipMapping() const { return mMipmapping; }
        
        //! Returns the number of color buffers
		int getNumColorBuffers() const { return mNumColorBuffers; }
        void setNumColorBuffers(int the_num) { mNumColorBuffers = the_num; }
		
	  protected:
		GLenum		mTarget;
		GLenum		mColorInternalFormat, mDepthInternalFormat, m_stencilInternalFormat;
		int			mSamples;
		int			mCoverageSamples;
		bool		mMipmapping;
		bool		m_depthBuffer, m_depthBufferAsTexture, mStencilBuffer;
		int			mNumColorBuffers;
		GLenum		mWrapS, mWrapT;
		GLenum		mMinFilter, mMagFilter;
		
		friend class Fbo;
	};

 protected:
	void		init();
	bool		initMultisample();
	void		resolveTextures() const;
	void		updateMipmaps( bool bindFirst, int attachment ) const;
	bool		checkStatus( class FboExceptionInvalidSpecification *resultExc );

	struct Obj {
		Obj();
		Obj( int aWidth, int aHeight );
		~Obj();

		int					mWidth, mHeight;
		Format				mFormat;
		GLuint				mId;
		GLuint				mResolveFramebufferId;
		std::vector<Renderbuffer>	mMultisampleColorRenderbuffers;
		Renderbuffer				mMultisampleDepthRenderbuffer;
		std::vector<Texture>		mColorTextures;
		Texture						m_depthTexture;
		Renderbuffer				m_depthRenderbuffer;
		mutable bool		mNeedsResolve, mNeedsMipmapUpdate;
	};
 
    typedef std::shared_ptr<Obj> ObjPtr;
	ObjPtr	m_obj;
	
	static GLint			sMaxSamples, sMaxAttachments;
	
  public:
	//! Emulates shared_ptr-like behavior
	operator bool() const { return m_obj.get() != nullptr; }
	void reset() { m_obj.reset(); }
};

class FboException : public Exception {
 public:
    FboException():Exception("FboException"){};
};

class FboExceptionInvalidSpecification : public FboException {
  public:
	FboExceptionInvalidSpecification() : FboException() { mMessage[0] = 0; }
	FboExceptionInvalidSpecification( const std::string &message ) throw();
	
	virtual const char * what() const throw() { return mMessage; }
	
  private:	
	char	mMessage[256];
};

}}// namespace gl
#endif