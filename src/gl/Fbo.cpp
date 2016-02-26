// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Fbo.hpp"

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
	mSamples = 0;
}

Renderbuffer::Obj::Obj( int aWidth, int aHeight, GLenum internalFormat, int msaaSamples)
	: mWidth( aWidth ), mHeight( aHeight ), mInternalFormat( internalFormat ), mSamples( msaaSamples )
{
    glGenRenderbuffers( 1, &mId );

	if( mSamples > Fbo::getMaxSamples() )
		mSamples = Fbo::getMaxSamples();

    glBindRenderbuffer( GL_RENDERBUFFER, mId );

#if ! defined( KINSKI_GLES )
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
	: m_obj( new Obj( width, height, internalFormat, 0) )
{
}
Renderbuffer::Renderbuffer( int width, int height, GLenum internalFormat, int msaaSamples)
	: m_obj( new Obj( width, height, internalFormat, msaaSamples))
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
	mDepthInternalFormat = GL_ENUM(GL_DEPTH_COMPONENT24);
    m_stencilInternalFormat = GL_STENCIL_INDEX8;
	m_depthBufferAsTexture = false;
#else
	mColorInternalFormat = GL_RGBA8;
    mDepthInternalFormat = GL_DEPTH32F_STENCIL8;//GL_DEPTH_COMPONENT32F; //GL_DEPTH24_STENCIL8;
	m_depthBufferAsTexture = true;
#endif
	mSamples = 0;
	mCoverageSamples = 0;
	mNumColorBuffers = 1;
	m_depthBuffer = true;
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
	m_depthBuffer = depthBuffer;
#if defined( KINSKI_GLES )
	m_depthBufferAsTexture = false;
#else
	m_depthBufferAsTexture = asTexture;
#endif
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fbo


void Fbo::init()
{
	bool useAA = m_obj->mFormat.mSamples > 0;

	// allocate the framebuffer itself
	glGenFramebuffers(1, &m_obj->mId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_obj->mId);

	Texture::Format textureFormat;
	textureFormat.setTarget(getTarget());
    auto col_fmt = getFormat().getColorInternalFormat();
	textureFormat.setInternalFormat(col_fmt);
    
#if !defined(KINSKI_GLES)
    GLint float_types[] = {GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F};
    GLint one_comp_types[] = {GL_RED, GL_GREEN, GL_BLUE, GL_R32F};
    if(is_in(col_fmt, float_types)){ textureFormat.set_data_type(GL_FLOAT); }
#endif
	textureFormat.setWrap( m_obj->mFormat.mWrapS, m_obj->mFormat.mWrapT);
	textureFormat.setMinFilter( m_obj->mFormat.mMinFilter);
	textureFormat.setMagFilter( m_obj->mFormat.mMagFilter);
	textureFormat.set_mipmapping( getFormat().hasMipMapping());

	// allocate the color buffers
	for( int c = 0; c < m_obj->mFormat.mNumColorBuffers; ++c )
    {
        auto tex = Texture(m_obj->mWidth, m_obj->mHeight, textureFormat);
        
#if !defined(KINSKI_GLES)
        if(is_in(col_fmt, one_comp_types)){ tex.set_swizzle(GL_RED, GL_RED, GL_RED, GL_ONE); }
#endif
		m_obj->mColorTextures.push_back(tex);
	}
	
#if ! defined( KINSKI_GLES )	
	if(m_obj->mFormat.mNumColorBuffers == 0)
    {
        // no color
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	}
#endif
    
    if(useAA){ useAA = initMultisample(); }
    
	if(!useAA)
    {
        // if we don't need any variety of multisampling or it failed to initialize
		// attach all the textures to the framebuffer
		vector<GLenum> drawBuffers;
		for(size_t c = 0; c < m_obj->mColorTextures.size(); ++c)
        {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + c, getTarget(),
                                   m_obj->mColorTextures[c].getId(), 0);
			drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + c);
		}
#if ! defined( KINSKI_GLES )
        if(!drawBuffers.empty()){ glDrawBuffers(drawBuffers.size(), &drawBuffers[0]); }
#endif

		// allocate and attach depth texture
		if(m_obj->mFormat.m_depthBuffer)
        {
			if(m_obj->mFormat.m_depthBufferAsTexture)
            {
	#if ! defined( KINSKI_GLES )			
				GLuint depthTextureId;
				glGenTextures(1, &depthTextureId);
				glBindTexture(getTarget(), depthTextureId);
				glTexImage2D(getTarget(), 0, getFormat().getDepthInternalFormat(),
                             m_obj->mWidth, m_obj->mHeight, 0, GL_DEPTH_STENCIL,
                             GL_FLOAT_32_UNSIGNED_INT_24_8_REV, nullptr);
                
//                glTexImage2D(getTarget(), 0, getFormat().getDepthInternalFormat(),
//                             m_obj->mWidth, m_obj->mHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
                
				glTexParameteri(getTarget(), GL_TEXTURE_MIN_FILTER, m_obj->mFormat.mMinFilter);
				glTexParameteri(getTarget(), GL_TEXTURE_MAG_FILTER, m_obj->mFormat.mMagFilter);
				glTexParameteri(getTarget(), GL_TEXTURE_WRAP_S, m_obj->mFormat.mWrapS);
				glTexParameteri(getTarget(), GL_TEXTURE_WRAP_T, m_obj->mFormat.mWrapT);
				m_obj->m_depthTexture = Texture(getTarget(), depthTextureId, m_obj->mWidth,
                                                m_obj->mHeight, false );
                
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, getTarget(),
                                       m_obj->m_depthTexture.getId(), 0);
                
                if(m_obj->mFormat.hasStencilBuffer())
                {
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, getTarget(),
                                           m_obj->m_depthTexture.getId(), 0);
                }
	#endif
			}
            // implement depth buffer as RenderBuffer
			else if(m_obj->mFormat.m_depthBuffer)
            {
				m_obj->m_depthRenderbuffer = Renderbuffer(m_obj->mWidth, m_obj->mHeight,
                                                          m_obj->mFormat.getDepthInternalFormat());
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                          m_obj->m_depthRenderbuffer.getId());
                
                if(m_obj->mFormat.hasStencilBuffer())
                {
                    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                              GL_RENDERBUFFER, m_obj->m_depthRenderbuffer.getId());
                }
			}
		}

		FboExceptionInvalidSpecification exc;
        // failed creation; throw
		if(!checkStatus(&exc)) { throw exc; }
	}
	
	m_obj->mNeedsResolve = false;
	m_obj->mNeedsMipmapUpdate = false;
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool Fbo::initMultisample()
{
#if defined( KINSKI_GLES )
	return false;
#else
	glGenFramebuffers(1, &m_obj->mResolveFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, m_obj->mResolveFramebufferId);
	
	// bind all of the color buffers to the resolve FB's attachment points
	vector<GLenum> drawBuffers;
	for( size_t c = 0; c < m_obj->mColorTextures.size(); ++c )
    {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + c, getTarget(),
                               m_obj->mColorTextures[c].getId(), 0);
		drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + c);
	}

	if( ! drawBuffers.empty() )
		glDrawBuffers( drawBuffers.size(), &drawBuffers[0] );

	// see if the resolve buffer is ok
	FboExceptionInvalidSpecification ignoredException;
    if(!checkStatus( &ignoredException)){ return false; }

	glBindFramebuffer( GL_FRAMEBUFFER, m_obj->mId );

	m_obj->mFormat.mSamples = std::min(m_obj->mFormat.mSamples, getMaxSamples());
    
	// setup the multisampled color renderbuffers
	for(int c = 0; c < m_obj->mFormat.mNumColorBuffers; ++c)
    {
		m_obj->mMultisampleColorRenderbuffers.push_back(Renderbuffer(m_obj->mWidth, m_obj->mHeight,
                                                                    m_obj->mFormat.mColorInternalFormat,
                                                                    m_obj->mFormat.mSamples));

		// attach the multisampled color buffer
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + c, GL_RENDERBUFFER,
                                  m_obj->mMultisampleColorRenderbuffers.back().getId());
	}
	
	if(!drawBuffers.empty())
		glDrawBuffers(drawBuffers.size(), &drawBuffers[0]);

	if(m_obj->mFormat.m_depthBuffer)
    {
		// create the multisampled depth Renderbuffer
		m_obj->mMultisampleDepthRenderbuffer = Renderbuffer(m_obj->mWidth, m_obj->mHeight,
                                                            m_obj->mFormat.mDepthInternalFormat,
                                                            m_obj->mFormat.mSamples);

		// attach the depth Renderbuffer
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                  m_obj->mMultisampleDepthRenderbuffer.getId());
        
        if(m_obj->mFormat.hasStencilBuffer())
        {
            // attach the depth Renderbuffer
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                      m_obj->mMultisampleDepthRenderbuffer.getId());
        }
	}

	// see if the primary framebuffer turned out ok
	return checkStatus( &ignoredException );
#endif // ! KINSKI_GLES
}

Fbo::Fbo(const glm::vec2 &the_size, Format format): Fbo(the_size.x, the_size.y, format){}
    
Fbo::Fbo(int width, int height, Format format)
	: m_obj(new Obj( width, height ))
{
	m_obj->mFormat = format;
	init();
}

Fbo::Fbo(int width, int height, bool alpha, bool color, bool depth)
	: m_obj(new Obj( width, height ))
{
	Format format;
	m_obj->mFormat.mColorInternalFormat = ( alpha ) ? GL_ENUM(GL_RGBA8) : GL_ENUM(GL_RGB8);
	m_obj->mFormat.m_depthBuffer = depth;
	m_obj->mFormat.mNumColorBuffers = color ? 1 : 0;
	init();
}

Texture Fbo::getTexture( int attachment )
{
	resolveTextures();
	updateMipmaps( true, attachment );
	return m_obj->mColorTextures.empty() ? gl::Texture() : m_obj->mColorTextures[attachment];
}

Texture& Fbo::getDepthTexture()
{
	return m_obj->m_depthTexture;
}

void Fbo::bindTexture( int textureUnit, int attachment )
{
	resolveTextures();
	m_obj->mColorTextures[attachment].bind( textureUnit );
	updateMipmaps( false, attachment );
}

void Fbo::unbindTexture()
{
	glBindTexture( getTarget(), 0 );
}

void Fbo::bindDepthTexture(int textureUnit)
{
	m_obj->m_depthTexture.bind(textureUnit);
}

void Fbo::resolveTextures() const
{
	if( ! m_obj->mNeedsResolve )
		return;

#if ! defined( KINSKI_GLES )		
	// if this FBO is multisampled, resolve it, so it can be displayed
	if ( m_obj->mResolveFramebufferId )
    {
		//SaveFramebufferBinding saveFboBinding;

		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_obj->mId);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_obj->mResolveFramebufferId);
		
		for(size_t c = 0; c < m_obj->mColorTextures.size(); ++c)
        {
			glDrawBuffer(GL_COLOR_ATTACHMENT0 + c);
			glReadBuffer(GL_COLOR_ATTACHMENT0 + c);
			GLbitfield bitfield = GL_COLOR_BUFFER_BIT;
			if(m_obj->m_depthTexture && m_obj->m_depthTexture.getId())
                bitfield |= GL_DEPTH_BUFFER_BIT;
			glBlitFramebuffer(0, 0, m_obj->mWidth, m_obj->mHeight, 0, 0, m_obj->mWidth,
                              m_obj->mHeight, bitfield, GL_NEAREST);
            KINSKI_CHECK_GL_ERRORS();
		}

		// restore the draw buffers to the default for the antialiased (non-resolve) framebuffer
		vector<GLenum> drawBuffers;
		for(size_t c = 0; c < m_obj->mColorTextures.size(); ++c)
			drawBuffers.push_back( GL_COLOR_ATTACHMENT0 + c );
		glBindFramebuffer( GL_FRAMEBUFFER, m_obj->mId );
		glDrawBuffers( drawBuffers.size(), &drawBuffers[0] );
        KINSKI_CHECK_GL_ERRORS();
	}
#endif

	m_obj->mNeedsResolve = false;
}

void Fbo::updateMipmaps( bool bindFirst, int attachment ) const
{
	if( ! m_obj->mNeedsMipmapUpdate )
		return;
	
	if( bindFirst ) 
    {
		m_obj->mColorTextures[attachment].bind();
		glGenerateMipmap( getTarget() );
	}
	else {
		glGenerateMipmap( getTarget() );
	}

	m_obj->mNeedsMipmapUpdate = false;
}

void Fbo::bindFramebuffer()
{
	glBindFramebuffer( GL_FRAMEBUFFER, m_obj->mId );
	if( m_obj->mResolveFramebufferId ) {
		m_obj->mNeedsResolve = true;
	}
	if( m_obj->mFormat.hasMipMapping() ) {
		m_obj->mNeedsMipmapUpdate = true;
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
	
    return status == GL_FRAMEBUFFER_COMPLETE;
}

GLint Fbo::getMaxSamples()
{
#if ! defined( KINSKI_GLES )
    if( sMaxSamples < 0 ){ glGetIntegerv( GL_MAX_SAMPLES, &sMaxSamples); }
	
	return sMaxSamples;
#else
	return 0;
#endif
}

GLint Fbo::getMaxAttachments()
{
#if ! defined( KINSKI_GLES )
	if(sMaxAttachments < 0) { glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS, &sMaxAttachments ); }
	
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
//	glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, m_obj->mId );
//	glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, dst.getId() );		
//	glBlitFramebufferEXT( srcArea.getX1(), srcArea.getY1(), srcArea.getX2(), srcArea.getY2(), dstArea.getX1(), dstArea.getY1(), dstArea.getX2(), dstArea.getY2(), mask, filter );
//}
//
//void Fbo::blitToScreen( const Area &srcArea, const Area &dstArea, GLenum filter, GLbitfield mask ) const
//{
//	SaveFramebufferBinding saveFboBinding;
//
//	glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, m_obj->mId );
//	glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, 0 );		
//	glBlitFramebufferEXT( srcArea.getX1(), srcArea.getY1(), srcArea.getX2(), srcArea.getY2(), dstArea.getX1(), dstArea.getY1(), dstArea.getX2(), dstArea.getY2(), mask, filter );
//}
//
//void Fbo::blitFromScreen( const Area &srcArea, const Area &dstArea, GLenum filter, GLbitfield mask )
//{
//	SaveFramebufferBinding saveFboBinding;
//
//	glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, GL_NONE );
//	glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, m_obj->mId );		
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