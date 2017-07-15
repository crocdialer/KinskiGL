#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>

#include "esUtil.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

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

static void page_flip_handler(int fd, unsigned int frame,
		                      unsigned int sec, unsigned int usec, void *data)
{
	int *waiting_for_flip = data;
	*waiting_for_flip = 0;
}

drmEventContext g_evctx =
{
        .version = DRM_EVENT_CONTEXT_VERSION,
        .page_flip_handler = page_flip_handler,
};
struct gbm_bo *g_bo = NULL;
struct drm_fb *g_fb = NULL;

static uint32_t g_previous_fb = 0;

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

static void
drm_fb_destroy_callback(struct gbm_bo *bo, void *data)
{
	struct drm_fb *fb = data;
	struct gbm_device *gbm = gbm_bo_get_device(bo);

	if (fb->fb_id)
		drmModeRmFB(drm.fd, fb->fb_id);

	free(fb);
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

static struct drm_fb * drm_fb_get_from_bo(struct gbm_bo *bo)
{
	struct drm_fb *fb = gbm_bo_get_user_data(bo);
	uint32_t width, height, stride, handle;
	int ret;

	if (fb)
		return fb;

	fb = calloc(1, sizeof *fb);
	fb->bo = bo;

	width = gbm_bo_get_width(bo);
	height = gbm_bo_get_height(bo);
	stride = gbm_bo_get_stride(bo);
	handle = gbm_bo_get_handle(bo).u32;

	ret = drmModeAddFB(drm.fd, width, height, 24, 32, stride, handle, &fb->fb_id);
	if(ret)
    {
		printf("failed to create fb: %s\n", strerror(errno));
		free(fb);
		return NULL;
	}
	gbm_bo_set_user_data(bo, fb, drm_fb_destroy_callback);
	return fb;
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

   if(!eglBindAPI(EGL_OPENGL_ES_API))
   {
       printf("failed to bind api EGL_OPENGL_ES_API\n");
       return EGL_FALSE;
   }

   // Get configs
   if(!eglGetConfigs(display, NULL, 0, &numConfigs)){ return EGL_FALSE; }

   // Choose config
   if(!eglChooseConfig(display, attribList, &config, 1, &numConfigs) || numConfigs != 1)
   {
       printf("failed to choose config: %d\n", numConfigs);
       return EGL_FALSE;
   }

   // Create a surface
   surface = eglCreateWindowSurface(display, config, gbm.surface, NULL);

   if(surface == EGL_NO_SURFACE)
   {
       printf("failed to create egl surface\n");
       return EGL_FALSE;
   }

   // Create a GL context
   context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);

   if(context == EGL_NO_CONTEXT)
   {
       printf("failed to create context\n");
       return EGL_FALSE;
   }

   // Make the context current
   if(!eglMakeCurrent(display, surface, surface, context))
   {
       printf("failed to make context current\n");
       return EGL_FALSE;
   }

   // printf("GL Extensions: \"%s\"\n", glGetString(GL_EXTENSIONS));

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

   if(!WinCreate(esContext, title)){ return GL_FALSE; }
   if(!CreateEGLContext(esContext->hWnd, &esContext->eglDisplay,
                        &esContext->eglContext, &esContext->eglSurface,
                        attribList))
   { return GL_FALSE; }

   glClear(GL_COLOR_BUFFER_BIT);
   eglSwapBuffers(esContext->eglDisplay, esContext->eglSurface);
   g_bo = gbm_surface_lock_front_buffer(gbm.surface);
   g_fb = drm_fb_get_from_bo(g_bo);

   esContext->width = gbm_bo_get_width(g_bo);
   esContext->height = gbm_bo_get_height(g_bo);

   /* set mode: */
   int ret = drmModeSetCrtc(drm.fd, drm.crtc_id, g_fb->fb_id, 0, 0,
                            &drm.connector_id, 1, drm.mode);
   if(ret)
   {
       printf("failed to set mode: %s\n", strerror(errno));
       return GL_FALSE;
   }
   return GL_TRUE;
}

void ESUTIL_API esSwapBuffer(ESContext *esContext)
{
    struct gbm_bo *next_bo;
    int waiting_for_flip = 1;

    eglSwapBuffers(esContext->eglDisplay, esContext->eglSurface);

    next_bo = gbm_surface_lock_front_buffer(gbm.surface);
    g_fb = drm_fb_get_from_bo(next_bo);

    // /*
    //  * Here you could also update drm plane layers if you want
    //  * hw composition
    //  */
    //
    // int ret = drmModePageFlip(drm.fd, drm.crtc_id, g_fb->fb_id,
    //                           DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip);
    // if(ret)
    // {
    //     printf("failed to queue page flip: %s\n", strerror(errno));
    //     return;
    // }
    //
    // while(waiting_for_flip)
    // {
    //     ret = select(drm.fd + 1, &g_fds, NULL, NULL, NULL);
    //     if(ret < 0)
    //     {
    //         printf("select err: %s\n", strerror(errno));
    //         return;
    //     }
    //     else if(ret == 0)
    //     {
    //         printf("select timeout!\n");
    //         return;
    //     }
    //     else if (FD_ISSET(0, &g_fds))
    //     {
    //         printf("user interrupted!\n");
    //         break;
    //     }
    //     drmHandleEvent(drm.fd, &g_evctx);
    // }
    //
    // /* release last buffer to render on again: */
    // gbm_surface_release_buffer(gbm.surface, g_bo);
    // g_bo = next_bo;

    uint32_t handle = gbm_bo_get_handle(g_bo).u32;
	uint32_t pitch = gbm_bo_get_stride(g_bo);
	uint32_t fb;
	drmModeAddFB(gbm.dev, drm.mode.hdisplay, drm.mode.vdisplay, 24, 32, pitch, handle, &fb);
    drmModeSetCrtc(gbm.dev, drm.crtc_id, fb, 0, 0, &drm.connector_id, 1, &drm.mode);

    /* release last buffer to render on again: */
    drmModeRmFB(gbm.dev, &g_previous_fb);
    gbm_surface_release_buffer(gbm.surface, g_bo);
    g_previous_fb = fb;
    g_bo = next_bo;
}
