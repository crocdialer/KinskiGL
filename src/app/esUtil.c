//
// Book:      OpenGL(R) ES 2.0 Programming Guide
// Authors:   Aaftab Munshi, Dan Ginsburg, Dave Shreiner
// ISBN-10:   0321502795
// ISBN-13:   9780321502797
// Publisher: Addison-Wesley Professional
// URLs:      http://safari.informit.com/9780321563835
//            http://www.opengles-book.com
//

// ESUtil.c
//
//    A utility library for OpenGL ES.  This library provides a
//    basic common framework for the example applications in the
//    OpenGL ES 2.0 Programming Guide.
//

///
//  Includes
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include "esUtil.h"

#include  "bcm_host.h"

///
// CreateEGLContext()
//
//    Creates an EGL rendering context and all associated elements
//
EGLBoolean CreateEGLContext ( EGLNativeWindowType hWnd, EGLDisplay* eglDisplay,
                              EGLContext* eglContext, EGLSurface* eglSurface,
                              EGLint attribList[])
{
   EGLint numConfigs;
   EGLint majorVersion;
   EGLint minorVersion;
   EGLDisplay display;
   EGLContext context;
   EGLSurface surface;
   EGLConfig config;

   EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };


   // Get Display
   display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   if ( display == EGL_NO_DISPLAY )
   {
      return EGL_FALSE;
   }

   // Initialize EGL
   if ( !eglInitialize(display, &majorVersion, &minorVersion) )
   {
      return EGL_FALSE;
   }

   // Get configs
   if ( !eglGetConfigs(display, NULL, 0, &numConfigs) )
   {
      return EGL_FALSE;
   }

   // Choose config
   if ( !eglChooseConfig(display, attribList, &config, 1, &numConfigs) )
   {
      return EGL_FALSE;
   }

   // Create a surface
   surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)hWnd, NULL);
   if ( surface == EGL_NO_SURFACE )
   {
      return EGL_FALSE;
   }

   // Create a GL context
   context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs );
   if ( context == EGL_NO_CONTEXT )
   {
      return EGL_FALSE;
   }

   // Make the context current
   if ( !eglMakeCurrent(display, surface, surface, context) )
   {
      return EGL_FALSE;
   }

   *eglDisplay = display;
   *eglSurface = surface;
   *eglContext = context;
   return EGL_TRUE;
}

///
//  WinCreate() - RaspberryPi, direct surface (No X, Xlib)
//
//      This function initialized the display and window for EGL
//
EGLBoolean WinCreate(ESContext *esContext, const char *title)
{
    int32_t success = 0;

    static EGL_DISPMANX_WINDOW_T nativewindow;

    DISPMANX_ELEMENT_HANDLE_T dispman_element;
    DISPMANX_DISPLAY_HANDLE_T dispman_display;
    DISPMANX_UPDATE_HANDLE_T dispman_update;
    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;

    int display_width;
    int display_height;

    // create an EGL window surface, passing context width/height
    success = graphics_get_display_size(0 /* LCD */, &display_width, &display_height);
    if(success < 0){ return EGL_FALSE; }

    esContext->width = display_width;
    esContext->height = display_height;

    // You can hardcode the resolution here:
    //display_width = 640;
    //display_height = 480;
    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = display_width;
    dst_rect.height = display_height;

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = display_width << 16;
    src_rect.height = display_height << 16;

    dispman_display = vc_dispmanx_display_open(0 /* LCD */);
    dispman_update = vc_dispmanx_update_start(0);

    int egl_layer = 0;

    // our egl layer
    VC_DISPMANX_ALPHA_T alpha_hints;
    alpha_hints.flags = DISPMANX_FLAGS_ALPHA_FROM_SOURCE;
    // alpha_hints.flags = DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS;
    // alpha_hints.opacity = 255;

    dispman_element = vc_dispmanx_element_add (dispman_update, dispman_display,
                                               egl_layer, &dst_rect, 0/*src*/,
                                               &src_rect, DISPMANX_PROTECTION_NONE,
                                               &alpha_hints, 0/*clamp*/, 0/*transform*/);
    nativewindow.element = dispman_element;
    nativewindow.width = display_width;
    nativewindow.height = display_height;
    vc_dispmanx_update_submit_sync(dispman_update);
    esContext->hWnd = &nativewindow;
	return EGL_TRUE;
}
//////////////////////////////////////////////////////////////////
//
//  Public Functions
//
//

///
//  esInitContext()
//
//      Initialize ES utility context.  This must be called before calling any other
//      functions.
//
void ESUTIL_API esInitContext ( ESContext *esContext )
{
   bcm_host_init();

   if ( esContext != NULL )
   {
      memset( esContext, 0, sizeof( ESContext) );
   }
}


///
//  esCreateWindow()
//
//      title - name for title bar of window
//      width - width of window to create
//      height - height of window to create
//      flags  - bitwise or of window creation flags
//          ES_WINDOW_ALPHA       - specifies that the framebuffer should have alpha
//          ES_WINDOW_DEPTH       - specifies that a depth buffer should be created
//          ES_WINDOW_STENCIL     - specifies that a stencil buffer should be created
//          ES_WINDOW_MULTISAMPLE - specifies that a multi-sample buffer should be created
//
GLboolean ESUTIL_API esCreateWindow ( ESContext *esContext, const char* title, GLuint flags )
{
   EGLint attribList[] =
   {
       EGL_RED_SIZE,       8,//5,
       EGL_GREEN_SIZE,     8,//6,
       EGL_BLUE_SIZE,      8,//5,
       EGL_ALPHA_SIZE,     (flags & ES_WINDOW_ALPHA) ? 8 : EGL_DONT_CARE,
       EGL_DEPTH_SIZE,     (flags & ES_WINDOW_DEPTH) ? 8 : EGL_DONT_CARE,
       EGL_STENCIL_SIZE,   (flags & ES_WINDOW_STENCIL) ? 8 : EGL_DONT_CARE,
       EGL_SAMPLE_BUFFERS, (flags & ES_WINDOW_MULTISAMPLE) ? 1 : 0,
       EGL_NONE
   };

   if(!esContext){ return GL_FALSE; }

  //  esContext->width = width;
  //  esContext->height = height;

   if (!WinCreate(esContext, title)){ return GL_FALSE; }


   if ( !CreateEGLContext ( esContext->hWnd,
                            &esContext->eglDisplay,
                            &esContext->eglContext,
                            &esContext->eglSurface,
                            attribList) )
   {
      return GL_FALSE;
   }


   return GL_TRUE;
}
