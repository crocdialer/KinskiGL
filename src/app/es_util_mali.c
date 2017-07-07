#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include "esUtil.h"

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static struct
{
	struct gbm_device *dev;
	struct gbm_surface *surface;
} gbm;

static struct
{
	int fd;
	drmModeModeInfo *mode;
	uint32_t crtc_id;
	uint32_t connector_id;
} drm;

struct drm_fb
{
	struct gbm_bo *bo;
	uint32_t fb_id;
};

fd_set g_fds;
drmEventContext g_evctx =
{
        .version = DRM_EVENT_CONTEXT_VERSION,
        .page_flip_handler = page_flip_handler,
};
struct gbm_bo *g_bo = NULL;
struct drm_fb *g_fb = NULL;

static uint32_t find_crtc_for_encoder(const drmModeRes *resources,
				                      const drmModeEncoder *encoder)
{
	for (int i = 0; i < resources->count_crtcs; i++)
    {
		/* possible_crtcs is a bitmask as described here:
		 * https://dvdhrm.wordpress.com/2012/09/13/linux-drm-mode-setting-api
		 */
		const uint32_t crtc_mask = 1 << i;
		const uint32_t crtc_id = resources->crtcs[i];
		if(encoder->possible_crtcs & crtc_mask){ return crtc_id; }
	}
	/* no match found */
	return -1;
}

static uint32_t find_crtc_for_connector(const drmModeRes *resources,
					const drmModeConnector *connector) {
	int i;

	for (i = 0; i < connector->count_encoders; i++) {
		const uint32_t encoder_id = connector->encoders[i];
		drmModeEncoder *encoder = drmModeGetEncoder(drm.fd, encoder_id);

		if (encoder) {
			const uint32_t crtc_id = find_crtc_for_encoder(resources, encoder);

			drmModeFreeEncoder(encoder);
			if (crtc_id != 0) {
				return crtc_id;
			}
		}
	}

	/* no match found */
	return -1;
}

static int init_drm(void)
{
	drmModeRes *resources;
	drmModeConnector *connector = NULL;
	drmModeEncoder *encoder = NULL;
	int i, area;

	drm.fd = open("/dev/dri/card0", O_RDWR);

	if(drm.fd < 0)
    {
		printf("could not open drm device\n");
		return -1;
	}
	resources = drmModeGetResources(drm.fd);

	if(!resources)
    {
		printf("drmModeGetResources failed: %s\n", strerror(errno));
		return -1;
	}

	/* find a connected connector: */
	for (i = 0; i < resources->count_connectors; i++)
    {
		connector = drmModeGetConnector(drm.fd, resources->connectors[i]);

        /* it's connected, let's use this! */
        if(connector->connection == DRM_MODE_CONNECTED){ break; }
		drmModeFreeConnector(connector);
		connector = NULL;
	}

    /* we could be fancy and listen for hotplug events and wait for
     * a connector..
     */
	if(!connector)
    {
		printf("no connected connector!\n");
		return -1;
	}

	/* find prefered mode or the highest resolution mode: */
	for (i = 0, area = 0; i < connector->count_modes; i++)
    {
		drmModeModeInfo *current_mode = &connector->modes[i];

		if(current_mode->type & DRM_MODE_TYPE_PREFERRED){ drm.mode = current_mode; }

		int current_area = current_mode->hdisplay * current_mode->vdisplay;
		if (current_area > area)
        {
			drm.mode = current_mode;
			area = current_area;
		}
	}

	if(!drm.mode)
    {
		printf("could not find mode!\n");
		return -1;
	}

	/* find encoder: */
	for (i = 0; i < resources->count_encoders; i++)
    {
		encoder = drmModeGetEncoder(drm.fd, resources->encoders[i]);
		if (encoder->encoder_id == connector->encoder_id)
			break;
		drmModeFreeEncoder(encoder);
		encoder = NULL;
	}

	if(encoder){ drm.crtc_id = encoder->crtc_id; }
    else
    {
		uint32_t crtc_id = find_crtc_for_connector(resources, connector);
		if (crtc_id == 0)
        {
			printf("no crtc found!\n");
			return -1;
		}
		drm.crtc_id = crtc_id;
	}
	drm.connector_id = connector->connector_id;
	return 0;
}

static int init_gbm(void)
{
	gbm.dev = gbm_create_device(drm.fd);
	gbm.surface = gbm_surface_create(gbm.dev, drm.mode->hdisplay, drm.mode->vdisplay,
			                         GBM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
	if(!gbm.surface)
    {
		printf("failed to create gbm surface\n");
		return -1;
	}
	return 0;
}

//
// CreateEGLContext()
//
//    Creates an EGL rendering context and all associated elements
//
EGLBoolean CreateEGLContext ( EGLNativeWindowType hWnd, EGLDisplay* eglDisplay,
                              EGLContext* eglContext, EGLSurface* eglSurface,
                              EGLint attribList[])
{
   EGLint numConfigs;
   EGLint major;
   EGLint minor;
   EGLDisplay display;
   EGLContext context;
   EGLSurface surface;
   EGLConfig config;

   EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

   PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display = NULL;
   get_platform_display = (void *) eglGetProcAddress("eglGetPlatformDisplayEXT");
   assert(get_platform_display != NULL);

   // Get Display
   display = get_platform_display(EGL_PLATFORM_GBM_KHR, gbm.dev, NULL);

   if(display == EGL_NO_DISPLAY){ return EGL_FALSE; }

   // Initialize EGL
   if(!eglInitialize(display, &major, &minor)){ return EGL_FALSE; }

   printf("Using display %p with EGL version %d.%d\n", display, major, minor);
   printf("EGL Version \"%s\"\n", eglQueryString(display, EGL_VERSION));
   printf("EGL Vendor \"%s\"\n", eglQueryString(display, EGL_VENDOR));
   printf("EGL Extensions \"%s\"\n", eglQueryString(display, EGL_EXTENSIONS));

   // Get configs
   if(!eglGetConfigs(display, NULL, 0, &numConfigs)){ return EGL_FALSE; }

   // Choose config
   if(!eglChooseConfig(display, attribList, &config, 1, &numConfigs)){ return EGL_FALSE; }

   // Create a surface
   surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)hWnd, NULL);

   if(surface == EGL_NO_SURFACE){ return EGL_FALSE; }

   // Create a GL context
   context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs );

   if(context == EGL_NO_CONTEXT){ return EGL_FALSE; }

   // Make the context current
   if(!eglMakeCurrent(display, surface, surface, context)){ return EGL_FALSE; }

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
    int ret = init_drm();

	if(ret)
    {
		printf("failed to initialize DRM\n");
		return EGL_FALSE;
	}

	FD_ZERO(&g_fds);
	FD_SET(0, &g_fds);
	FD_SET(drm.fd, &g_fds);

	ret = init_gbm();

	if(ret)
    {
		printf("failed to initialize GBM\n");
		return EGL_FALSE;
	}
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
void ESUTIL_API esInitContext(ESContext *esContext)
{
    if(esContext){ memset(esContext, 0, sizeof( ESContext)); }
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

   if(!WinCreate(esContext, title)){ return GL_FALSE; }
   if(!CreateEGLContext(esContext->hWnd, &esContext->eglDisplay,
                        &esContext->eglContext, &esContext->eglSurface,
                        attribList))
   { return GL_FALSE; }
   return GL_TRUE;
}

void ESUTIL_API esSwapBuffer(ESContext *esContext)
{
    eglSwapBuffers(esContext->eglDisplay, esContext->eglSurface);
}
