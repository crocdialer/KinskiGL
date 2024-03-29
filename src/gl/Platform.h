//
// Created by crocdialer on 3/24/17.
//

#ifndef KINSKIGL_PLATFORM_H
#define KINSKIGL_PLATFORM_H

//triggers checks with glGetError()
//#define KINSKI_GL_REPORT_ERRORS

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#define KINSKI_MSW
#else
#if defined(linux) || defined(__linux) || defined(__linux__)
#define KINSKI_LINUX

#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#define KINSKI_COCOA
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE
#define KINSKI_COCOA_TOUCH
#else
#define KINSKI_MAC
#endif
// This is defined to prevent the inclusion of some unfortunate macros in <AssertMacros.h>
#define __ASSERTMACROS__
#else
#error "kinski compile error: Unknown platform"
#endif
#endif

#if defined(KINSKI_COCOA_TOUCH) || defined(KINSKI_ARM)
#define KINSKI_GLES
#endif

#if defined(KINSKI_ARM)
#define KINSKI_EGL
#ifndef GL_GLEXT_PROTOTYPES
    #define GL_GLEXT_PROTOTYPES
#endif
#endif

#if defined(KINSKI_RASPI)
#define KINSKI_GLES_2
#define KINSKI_NO_VAO
#endif

#if defined(KINSKI_MALI)
#define KINSKI_GLES_3
#endif

#ifdef KINSKI_GLES // OpenGL ES

#ifdef KINSKI_COCOA_TOUCH // iOS
#ifdef KINSKI_GLES_3 // ES 3
#import <OpenGLES/ES3/gl.h>
#import <OpenGLES/ES3/glext.h>
#else // ES 2
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#endif // KINSKI_COCOA_TOUCH (iOS)

#elif defined(KINSKI_GLES_3)
#include <GLES3/gl31.h>
#include <GLES2/gl2ext.h>
#else // general ES2
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif // KINSKI_GLES (OpenGL ES)

#elif defined(KINSKI_COCOA)// desktop GL3
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/glcorearb.h>
#include <GL/glext.h>
#endif

#if defined(KINSKI_COCOA)
typedef struct _CGLContextObject *CGLContextObj;
#elif defined(KINSKI_COCOA_TOUCH)
#if defined( __OBJC__ )
		@class	EAGLContext;
	#else
		class	EAGLContext;
	#endif
#elif defined(KINSKI_EGL)
    typedef void* EGLContext;
	typedef void* EGLDisplay;
	typedef void* EGLSurface;
#endif

// crossplattform helper-macros to append either nothing or "OES"
// appropriately to a symbol based on either OpenGL3+ or OpenGLES2
#if defined(KINSKI_GLES) && !defined(KINSKI_GLES_3) // only ES2 here
#define GL_SUFFIX(sym) sym##OES
#define GL_ENUM(sym) sym##_OES
#else // all versions, except ES2
#define GL_SUFFIX(sym) sym
#define GL_ENUM(sym) sym
#endif

namespace kinski{ namespace gl
{

struct PlatformData
{
    virtual ~PlatformData(){};
};

#if defined(KINSKI_EGL)
struct PlatformDataEGL : public PlatformData
{
    PlatformDataEGL(EGLDisplay the_display, EGLContext the_context, EGLSurface the_surface):
    egl_display(the_display),
    egl_context(the_context),
    egl_surface(the_surface)
    {}

    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;
};

#elif defined(KINSKI_MAC)
struct PlatformDataCGL : public PlatformData
{
    PlatformDataCGL(CGLContextObj the_context):
    cgl_context(the_context)
    {}

    CGLContextObj cgl_context;
};

#endif

}}

#endif //KINSKIGL_PLATFORM_H
