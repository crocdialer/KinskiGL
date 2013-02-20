// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Fbo.h"

using namespace std;

namespace kinski { namespace gl {

GLint Fbo::sMaxSamples = -1;
GLint Fbo::sMaxAttachments = -1;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RenderBuffer::Obj
Renderbuffer::Obj::Obj()
{
	mWidth = mHeight = -1;
	mId = 0;
	mInternalFormat = 0;
	mSamples = mCoverageSamples = 0;
}

Renderbuffer::Obj::Obj( int aWidth, int aHeight, GLenum internalFormat, int msaaSamples, int coverageSamples )
	: mWidth( aWidth ), mHeight( aHeight ), mInternalFormat( internalFormat ), mSamples( msaaSamples ), mCoverageSamples( coverageSamples )
{
    glGenRenderbuffers( 1, &mId );

	if( mSamples > Fbo::getMaxSamples() )
		mSamples = Fbo::getMaxSamples();

    glBindRenderbuffer( GL_RENDERBUFFER, mId );

#if ! defined( KINSKI_GLES )
//	if( mCoverageSamples ) // create a CSAA buffer
//		glRenderbufferStorageMultisampleCoverageNV( GL_RENDERBUFFER_EXT, mCoverageSamples, mSamples, mInternalFormat, mWidth, mHeight );
    
	if( mSamples ) // create a regular MSAA buffer
		glRenderbufferStorageMultisample( GL_RENDERBUFFER, mSamples, mInternalFormat, mWidth, mHeight );
	else
#endif

    glRenderbufferStorage( GL_RENDERBUFFER, mInternalFormat, mWidth, mHeight );

}

Renderbuffer::Obj::~Obj()
{
	if( mId )
		glDeleteRenderbuffers( 1, &mId );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Renderbuffer::Renderbuffer
Renderbuffer::Renderbuffer( int width, int height, GLenum internalFormat )
	: mObj( new Obj( width, height, internalFormat, 0, 0 ) )
{
}
Renderbuffer::Renderbuffer( int width, int height, GLenum internalFormat, int msaaSamples, int coverageSamples )
	: mObj( new Obj( width, height, internalFormat, msaaSamples, coverageSamples ) )
{
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fbo::Obj
Fbo::Obj::Obj()
{
	mId = 0;
	mResolveFramebufferId = 0;
}

Fbo::Obj::Obj( int width, int height )
	: mWidth( width ), mHeight( height )
{
	mId = 0;
	mResolveFramebufferId = 0;
}

Fbo::Obj::~Obj()
{
	if( mId )
		glDeleteFramebuffers( 1, &mId );
	if( mResolveFramebufferId )
		glDeleteFramebuffers( 1, &mResolveFramebufferId );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fbo::Format
Fbo::Format::Format()
{
	mTarget = GL_TEXTURE_2D;
#if defined( KINSKI_GLES )
	mColorInternalFormat = GL_RGBA;
	mDepthInternalFormat = GL_DEPTH_COMPONENT24_OES;
	mDepthBufferAsTexture = false;
#else
	mColorInternalFormat = GL_RGBA8;
	mDepthInternalFormat = GL_DEPTH_COMPONENT24;
	mDepthBufferAsTexture = true;
#endif
	mSamples = 0;
	mCoverageSamples = 0;
	mNumColorBuffers = 1;
	mDepthBuffer = true;
	mStencilBuffer = false;
	mMipmapping = false;
	mWrapS = GL_CLAMP_TO_EDGE;
	mWrapT = GL_CLAMP_TO_EDGE;
	mMinFilter = GL_LINEAR;
	mMagFilter = GL_LINEAR;
}

void Fbo::Format::enableColorBuffer( bool colorBuffer, int numColorBuffers )
{
#if defined( KINSKI_GLES )
	mNumColorBuffers = ( colorBuffer && numColorBuffers ) ? 1 : 0;
#else
	mNumColorBuffers = ( colorBuffer ) ? numColorBuffers : 0;
#endif
}

void Fbo::Format::enableDepthBuffer( bool depthBuffer, bool asTexture )
{
	mDepthBuffer = depthBuffer;
#if defined( KINSKI_GLES )
	mDepthBufferAsTexture = false;
#else
	mDepthBufferAsTexture = asTexture;
#endif
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fbo


void Fbo::init()
{
	//SaveFramebufferBinding bindingSaver;
	
#if defined( CINDER_MSW )
	static bool csaaSupported = ( GLEE_NV_framebuffer_multisample_coverage != 0 );
#else
	static bool csaaSupported = false;
#endif
	bool useCSAA = csaaSupported && ( mObj->mFormat.mCoverageSamples > mObj->mFormat.mSamples );
	bool useMSAA = ( mObj->mFormat.mCoverageSamples > 0 ) || ( mObj->mFormat.mSamples > 0 );
	if( useCSAA )
		useMSAA = false;

	// allocate the framebuffer itself
	glGenFramebuffers( 1, &mObj->mId );
    
    glBindFramebuffer(GL_FRAMEBUFFER, mObj->mId );

	Texture::Format textureFormat;
	textureFormat.setTarget( getTarget() );
	textureFormat.setInternalFormat( getFormat().getColorInternalFormat() );
	textureFormat.setWrap( mObj->mFormat.mWrapS, mObj->mFormat.mWrapT );
	textureFormat.setMinFilter( mObj->mFormat.mMinFilter );
	textureFormat.setMagFilter( mObj->mFormat.mMagFilter );
	textureFormat.enableMipmapping( getFormat().hasMipMapping() );

	// allocate the color buffers
	for( int c = 0; c < mObj->mFormat.mNumColorBuffers; ++c ) {
		mObj->mColorTextures.push_back( Texture( mObj->mWidth, mObj->mHeight, textureFormat ) );
	}
	
#if ! defined( KINSKI_GLES )	
	if( mObj->mFormat.mNumColorBuffers == 0 ) { // no color
		glDrawBuffer( GL_NONE );
		glReadBuffer( GL_NONE );	
	}
#endif
    
    bool useAA = (useCSAA || useMSAA);
    if(useAA)
        useAA = initMultisample( useCSAA );
    
	if( !useAA ) { // if we don't need any variety of multisampling or it failed to initialize
		// attach all the textures to the framebuffer
		vector<GLenum> drawBuffers;
		for( size_t c = 0; c < mObj->mColorTextures.size(); ++c ) {
			glFramebufferTexture2D( GL_FRAMEBUFFER,
             GL_COLOR_ATTACHMENT0 + c, getTarget(), mObj->mColorTextures[c].getId(), 0 );
			drawBuffers.push_back( GL_COLOR_ATTACHMENT0 + c );
		}
#if ! defined( KINSKI_GLES )
		if( ! drawBuffers.empty() )
			glDrawBuffers( drawBuffers.size(), &drawBuffers[0] );
#endif

		// allocate and attach depth texture
		if( mObj->mFormat.mDepthBuffer ) {
			if( mObj->mFormat.mDepthBufferAsTexture ) {
	#if ! defined( KINSKI_GLES )			
				GLuint depthTextureId;
				glGenTextures( 1, &depthTextureId );
				glBindTexture( getTarget(), depthTextureId );
				glTexImage2D( getTarget(), 0, getFormat().getDepthInternalFormat(), mObj->mWidth, mObj->mHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL );
				glTexParameteri( getTarget(), GL_TEXTURE_MIN_FILTER, mObj->mFormat.mMinFilter );
				glTexParameteri( getTarget(), GL_TEXTURE_MAG_FILTER, mObj->mFormat.mMagFilter );
				glTexParameteri( getTarget(), GL_TEXTURE_WRAP_S, mObj->mFormat.mWrapS );
				glTexParameteri( getTarget(), GL_TEXTURE_WRAP_T, mObj->mFormat.mWrapT );
                
                //TODO: check for correctness
//				glTexParameteri( getTarget(), GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE );
                
				mObj->mDepthTexture = Texture( getTarget(), depthTextureId, mObj->mWidth, mObj->mHeight, true );

				glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, getTarget(), mObj->mDepthTexture.getId(), 0 );
	#else
		throw; // this should never fire in OpenGL ES
	#endif
			}
			else if( mObj->mFormat.mDepthBuffer ) { // implement depth buffer as RenderBuffer
				mObj->mDepthRenderbuffer = Renderbuffer( mObj->mWidth, mObj->mHeight, mObj->mFormat.getDepthInternalFormat() );
				glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mObj->mDepthRenderbuffer.getId() );
			}
		}

		FboExceptionInvalidSpecification exc;
		if( ! checkStatus( &exc ) ) { // failed creation; throw
			throw exc;
		}
	}
	
	mObj->mNeedsResolve = false;
	mObj->mNeedsMipmapUpdate = false;
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool Fbo::initMultisample( bool csaa )
{
#if defined( KINSKI_GLES )
	return false;
#else
	glGenFramebuffers( 1, &mObj->mResolveFramebufferId );
	glBindFramebuffer( GL_FRAMEBUFFER, mObj->mResolveFramebufferId ); 
	
	// bind all of the color buffers to the resolve FB's attachment points
	vector<GLenum> drawBuffers;
	for( size_t c = 0; c < mObj->mColorTextures.size(); ++c ) {
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + c, getTarget(), mObj->mColorTextures[c].getId(), 0 );
		drawBuffers.push_back( GL_COLOR_ATTACHMENT0 + c );
	}

	if( ! drawBuffers.empty() )
		glDrawBuffers( drawBuffers.size(), &drawBuffers[0] );

	// see if the resolve buffer is ok
	FboExceptionInvalidSpecification ignoredException;
	if( ! checkStatus( &ignoredException ) )
		return false;

	glBindFramebuffer( GL_FRAMEBUFFER, mObj->mId );

	if( mObj->mFormat.mSamples > getMaxSamples() ) {
		mObj->mFormat.mSamples = getMaxSamples();
	}

	// setup the multisampled color renderbuffers
	for( int c = 0; c < mObj->mFormat.mNumColorBuffers; ++c ) {
		mObj->mMultisampleColorRenderbuffers.push_back( Renderbuffer( mObj->mWidth, mObj->mHeight, mObj->mFormat.mColorInternalFormat, mObj->mFormat.mSamples, mObj->mFormat.mCoverageSamples ) );

		// attach the multisampled color buffer
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + c, GL_RENDERBUFFER, mObj->mMultisampleColorRenderbuffers.back().getId() );
	}
	
	if( ! drawBuffers.empty() )
		glDrawBuffers( drawBuffers.size(), &drawBuffers[0] );

	if( mObj->mFormat.mDepthBuffer ) {
		// create the multisampled depth Renderbuffer
		mObj->mMultisampleDepthRenderbuffer = Renderbuffer( mObj->mWidth, mObj->mHeight, mObj->mFormat.mDepthInternalFormat, mObj->mFormat.mSamples, mObj->mFormat.mCoverageSamples );

		// attach the depth Renderbuffer
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mObj->mMultisampleDepthRenderbuffer.getId() );
	}

	// see if the primary framebuffer turned out ok
	return checkStatus( &ignoredException );
#endif // ! KINSKI_GLES
}

Fbo::Fbo( int width, int height, Format format )
	: mObj( shared_ptr<Obj>( new Obj( width, height ) ) )
{
	mObj->mFormat = format;
	init();
}

Fbo::Fbo( int width, int height, bool alpha, bool color, bool depth )
	: mObj( shared_ptr<Obj>( new Obj( width, height ) ) )
{
	Format format;
#if defined( KINSKI_GLES )
	mObj->mFormat.mColorInternalFormat = ( alpha ) ? GL_RGBA8_OES : GL_RGB8_OES;
#else
	mObj->mFormat.mColorInternalFormat = ( alpha ) ? GL_RGBA8 : GL_RGB8;
#endif
	mObj->mFormat.mDepthBuffer = depth;
	mObj->mFormat.mNumColorBuffers = color ? 1 : 0;
	init();
}

Texture& Fbo::getTexture( int attachment )
{
	resolveTextures();
	updateMipmaps( true, attachment );
	return mObj->mColorTextures[attachment];
}

Texture& Fbo::getDepthTexture()
{
	return mObj->mDepthTexture;
}

void Fbo::bindTexture( int textureUnit, int attachment )
{
	resolveTextures();
	mObj->mColorTextures[attachment].bind( textureUnit );
	updateMipmaps( false, attachment );
}

void Fbo::unbindTexture()
{
	glBindTexture( getTarget(), 0 );
}

void Fbo::bindDepthTexture( int textureUnit )
{
	mObj->mDepthTexture.bind( textureUnit );
}

void Fbo::resolveTextures() const
{
	if( ! mObj->mNeedsResolve )
		return;

#if ! defined( KINSKI_GLES )		
	// if this FBO is multisampled, resolve it, so it can be displayed
	if ( mObj->mResolveFramebufferId ) {
		//SaveFramebufferBinding saveFboBinding;

		glBindFramebuffer( GL_READ_FRAMEBUFFER, mObj->mId );
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, mObj->mResolveFramebufferId );
		
		for( size_t c = 0; c < mObj->mColorTextures.size(); ++c ) {
			glDrawBuffer( GL_COLOR_ATTACHMENT0 + c );
			glReadBuffer( GL_COLOR_ATTACHMENT0 + c );
			GLbitfield bitfield = GL_COLOR_BUFFER_BIT;
			if( mObj->mDepthTexture )
				bitfield |= GL_DEPTH_BUFFER_BIT;
			glBlitFramebuffer( 0, 0, mObj->mWidth, mObj->mHeight, 0, 0, mObj->mWidth, mObj->mHeight, bitfield, GL_NEAREST );
		}

		// restore the draw buffers to the default for the antialiased (non-resolve) framebuffer
		vector<GLenum> drawBuffers;
		for( size_t c = 0; c < mObj->mColorTextures.size(); ++c )
			drawBuffers.push_back( GL_COLOR_ATTACHMENT0 + c );
		glBindFramebuffer( GL_FRAMEBUFFER, mObj->mId );
		glDrawBuffers( drawBuffers.size(), &drawBuffers[0] );
	}
#endif

	mObj->mNeedsResolve = false;
}

void Fbo::updateMipmaps( bool bindFirst, int attachment ) const
{
	if( ! mObj->mNeedsMipmapUpdate )
		return;
	
	if( bindFirst ) 
    {
		mObj->mColorTextures[attachment].bind();
		glGenerateMipmap( getTarget() );
	}
	else {
		glGenerateMipmap( getTarget() );
	}

	mObj->mNeedsMipmapUpdate = false;
}

void Fbo::bindFramebuffer()
{
	glBindFramebuffer( GL_FRAMEBUFFER, mObj->mId );
	if( mObj->mResolveFramebufferId ) {
		mObj->mNeedsResolve = true;
	}
	if( mObj->mFormat.hasMipMapping() ) {
		mObj->mNeedsMipmapUpdate = true;
	}
}

void Fbo::unbindFramebuffer()
{
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

bool Fbo::checkStatus( FboExceptionInvalidSpecification *resultExc )
{
	GLenum status;
	status = (GLenum) glCheckFramebufferStatus( GL_FRAMEBUFFER );
	switch( status ) {
		case GL_FRAMEBUFFER_COMPLETE:
		break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			*resultExc = FboExceptionInvalidSpecification( "Unsupported framebuffer format" );
		return false;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: missing attachment" );
		return false;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: duplicate attachment" );
		return false;

#if ! defined( KINSKI_GLES )
		case GL_ENUM(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER):
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: missing draw buffer" );
		return false;
		case GL_ENUM(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER):
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: missing read buffer" );
		return false;
#endif
		default:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer invalid: unknown reason" );
		return false;
    }
	
    return true;
}

GLint Fbo::getMaxSamples()
{
#if ! defined( KINSKI_GLES )
	if( sMaxSamples < 0 ) {
//		if( ( ! gl::isExtensionAvailable( "GL_EXT_framebuffer_multisample" ) ) || ( ! gl::isExtensionAvailable( "GL_EXT_framebuffer_blit" ) ) ) {
//			sMaxSamples = 0;
//		}
//		else
			glGetIntegerv( GL_MAX_SAMPLES, &sMaxSamples);	
	}
	
	return sMaxSamples;
#else
	return 0;
#endif
}

GLint Fbo::getMaxAttachments()
{
#if ! defined( KINSKI_GLES )
	if( sMaxAttachments < 0 ) {
		glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS, &sMaxAttachments );
	}
	
	return sMaxAttachments;
#else
	return 1;
#endif
}

//#if ! defined( KINSKI_GLES )
//void Fbo::blitTo( Fbo dst, const Area &srcArea, const Area &dstArea, GLenum filter, GLbitfield mask ) const
//{
//	SaveFramebufferBinding saveFboBinding;
//
//	glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, mObj->mId );
//	glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, dst.getId() );		
//	glBlitFramebufferEXT( srcArea.getX1(), srcArea.getY1(), srcArea.getX2(), srcArea.getY2(), dstArea.getX1(), dstArea.getY1(), dstArea.getX2(), dstArea.getY2(), mask, filter );
//}
//
//void Fbo::blitToScreen( const Area &srcArea, const Area &dstArea, GLenum filter, GLbitfield mask ) const
//{
//	SaveFramebufferBinding saveFboBinding;
//
//	glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, mObj->mId );
//	glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, 0 );		
//	glBlitFramebufferEXT( srcArea.getX1(), srcArea.getY1(), srcArea.getX2(), srcArea.getY2(), dstArea.getX1(), dstArea.getY1(), dstArea.getX2(), dstArea.getY2(), mask, filter );
//}
//
//void Fbo::blitFromScreen( const Area &srcArea, const Area &dstArea, GLenum filter, GLbitfield mask )
//{
//	SaveFramebufferBinding saveFboBinding;
//
//	glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, GL_NONE );
//	glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, mObj->mId );		
//	glBlitFramebufferEXT( srcArea.getX1(), srcArea.getY1(), srcArea.getX2(), srcArea.getY2(), dstArea.getX1(), dstArea.getY1(), dstArea.getX2(), dstArea.getY2(), mask, filter );
//}
//#endif

FboExceptionInvalidSpecification::FboExceptionInvalidSpecification( const string &message ) throw()
	: FboException()
{
	strncpy( mMessage, message.c_str(), 255 );
}

}// namespace gl
}