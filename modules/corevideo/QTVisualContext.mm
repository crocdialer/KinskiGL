/*
     File: QTVisualContext.mm
 Abstract: 
 Utility class for managing a QT visual context.
 
  Version: 2.0
 
 Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple
 Inc. ("Apple") in consideration of your agreement to the following
 terms, and your use, installation, modification or redistribution of
 this Apple software constitutes acceptance of these terms.  If you do
 not agree with these terms, please do not use, install, modify or
 redistribute this Apple software.
 
 In consideration of your agreement to abide by the following terms, and
 subject to these terms, Apple grants you a personal, non-exclusive
 license, under Apple's copyrights in this original Apple software (the
 "Apple Software"), to use, reproduce, modify and redistribute the Apple
 Software, with or without modifications, in source and/or binary forms;
 provided that if you redistribute the Apple Software in its entirety and
 without modifications, you must retain this notice and the following
 text and disclaimers in all such redistributions of the Apple Software.
 Neither the name, trademarks, service marks or logos of Apple Inc. may
 be used to endorse or promote products derived from the Apple Software
 without specific prior written permission from Apple.  Except as
 expressly stated in this notice, no other rights or licenses, express or
 implied, are granted by Apple herein, including but not limited to any
 patent rights that may be infringed by your derivative works or by other
 works in which the Apple Software may be incorporated.
 
 The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
 MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
 OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 
 IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
 AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 
 Copyright (C) 2013 Apple Inc. All Rights Reserved.
 
 */

#pragma mark -
#pragma mark Private - Headers

#import "QTVisualContext.h"

#pragma mark -
#pragma mark Private - Data Structures

struct QTVisualContextData
{
	GLuint              width;			// Width of the pixel buffer
	GLuint              height;			// Height of the pixel buffer
	QTVisualContextRef  context;		// Pixel buffer or texture visual context
};

typedef struct QTVisualContextData   QTVisualContextData;

#pragma mark -
#pragma mark Private - Utilities - Dictionary

static inline BOOL QTVisualContextDictionarySetValue(CFMutableDictionaryRef pDictionary,
                                                     CFStringRef pKey,
                                                     const SInt32 nValue)
{
	BOOL bSuccess = (pDictionary!= NULL) && (pKey != NULL);
    
    if(bSuccess)
    {
        CFNumberRef  pNumber = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &nValue);
        
        bSuccess = pNumber != NULL;
        
        if(bSuccess)
        {
            CFDictionarySetValue(pDictionary, pKey, pNumber);
            
            CFRelease(pNumber);
        } // if
    } // if
	
    return bSuccess;
} // QTVisualContextDictionarySetValue

static inline BOOL QTVisualContextDictionarySetFormat(CFMutableDictionaryRef pDictionary)
{
	return  QTVisualContextDictionarySetValue(pDictionary, kCVPixelBufferPixelFormatTypeKey, k32BGRAPixelFormat);
} // QTVisualContextDictionarySetFormat

static inline BOOL QTVisualContextDictionarySetSize(const GLuint            nWidth,
                                                    const GLuint            nHeight,
                                                    CFMutableDictionaryRef  pDictionary)
{
	BOOL bSuccess = QTVisualContextDictionarySetValue(pDictionary, kCVPixelBufferWidthKey, nWidth);
	
	return bSuccess && QTVisualContextDictionarySetValue(pDictionary, kCVPixelBufferHeightKey, nHeight);
} // QTVisualContextDictionarySetSize

static inline BOOL QTVisualContextDictionarySetBytesPerRowAlignment(CFMutableDictionaryRef pDictionary)
{
	return  QTVisualContextDictionarySetValue(pDictionary, kCVPixelBufferBytesPerRowAlignmentKey, 1);
} // QTVisualContextDictionarySetSize

static inline void QTVisualContextDictionarySetOpenGLCompatibility(CFMutableDictionaryRef pDictionary)
{
	CFDictionarySetValue(pDictionary, kCVPixelBufferOpenGLCompatibilityKey, kCFBooleanTrue);
} // QTVisualContextDictionarySetOpenGLCompatibility

static CFMutableDictionaryRef QTVisualContextDictionaryCreateWithBounds(QTVisualContextDataRef pVisualCtx)
{
    CFMutableDictionaryRef pOptions = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                                0,
                                                                &kCFTypeDictionaryKeyCallBacks,
                                                                &kCFTypeDictionaryValueCallBacks);
    
    if(pOptions != NULL)
    {
        CFMutableDictionaryRef  pAttributes = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                                        0,
                                                                        &kCFTypeDictionaryKeyCallBacks,
                                                                        &kCFTypeDictionaryValueCallBacks);
        
        if(pAttributes != NULL)
        {
            if(QTVisualContextDictionarySetFormat(pAttributes))
            {
                if(QTVisualContextDictionarySetSize(pVisualCtx->width, pVisualCtx->height, pAttributes))
                {
                    if(QTVisualContextDictionarySetBytesPerRowAlignment(pAttributes))
                    {
                        QTVisualContextDictionarySetOpenGLCompatibility(pAttributes);
                        
                        CFDictionarySetValue(pOptions,
                                             kQTVisualContextPixelBufferAttributesKey,
                                             pAttributes);
                    } // if
                } // if
            } // if
            
            CFRelease(pAttributes);
        } // if
    } // if
	
	return  pOptions;
} // QTVisualContextDictionaryCreateWithBounds

#pragma mark
#pragma mark Private - Utilties - Acquire

static BOOL QTVisualContextAcquire(QTVisualContextDataRef pVisualCtx)
{
    OSStatus err = kUnknownType;
    
	CFMutableDictionaryRef pOptions = QTVisualContextDictionaryCreateWithBounds(pVisualCtx);
    
	if(pOptions != NULL)
	{
		err = QTPixelBufferContextCreate(kCFAllocatorDefault,
                                         pOptions,
                                         &pVisualCtx->context);
        
		CFRelease(pOptions);
	} // if
	
	return err == noErr;
} // QTVisualContextAcquire

static BOOL QTVisualContextAcquire(NSOpenGLContext *pContext,
                                   NSOpenGLPixelFormat *pPixelFormat,
                                   QTVisualContextDataRef pVisualCtx)
{
	BOOL bSuccess = (pContext != nil) && (pPixelFormat != nil);
    
	if(bSuccess)
	{
		CGLContextObj      pCGLContext     = (CGLContextObj)[pContext CGLContextObj];
		CGLPixelFormatObj  pCGLPixelFormat = (CGLPixelFormatObj)[pPixelFormat CGLPixelFormatObj];
		CFDictionaryRef    pAttributes     = NULL;
        
		// Creates a new OpenGL texture context for a specified OpenGL context and pixel format
		
		OSStatus err = QTOpenGLTextureContextCreate(kCFAllocatorDefault,		// an allocator to Create functions
                                                    pCGLContext,				// the OpenGL context
                                                    pCGLPixelFormat,			// pixelformat object that specifies
                                                                                // buffer types and other attributes
                                                                                // of the context
                                                    pAttributes,				// a CF Dictionary of attributes
                                                    &pVisualCtx->context);      // returned OpenGL texture context
		
		bSuccess = err == noErr;
	} // if
	
	return  bSuccess;
} // initQTOpenGLTextureContext

static BOOL QTVisualContextAcquire(const QTVisualContextType nType,
                                   NSOpenGLContext *pContext,
                                   NSOpenGLPixelFormat *pPixelFormat,
                                   QTVisualContextDataRef pVisualCtx)
{
	BOOL bSuccess = NO;
	
	switch(nType)
	{
		case kQTVisualContextPixelBuffer:
			bSuccess = QTVisualContextAcquire(pVisualCtx);
			break;
			
		case kQTVisualContextOpenGLTexture:
		default:
			bSuccess = QTVisualContextAcquire(pContext, pPixelFormat, pVisualCtx);
	} // switch
	
	return  bSuccess;
} // QTVisualContextAcquire

#pragma mark
#pragma mark Private - Utilties - Constructor

static QTVisualContextDataRef QTVisualContextCreate(const NSSize * const pSize,
                                                    const QTVisualContextType nType,
                                                    NSOpenGLContext *pContext,
                                                    NSOpenGLPixelFormat *pPixelFormat)
{
    QTVisualContextDataRef pVisualCtx = (QTVisualContextDataRef)calloc(1, sizeof(QTVisualContextData));
    
    if (pVisualCtx != NULL)
    {
        pVisualCtx->width  = (pSize != NULL) ? GLuint(pSize->width)  : 1920;
        pVisualCtx->height = (pSize != NULL) ? GLuint(pSize->height) : 1080;
        
        BOOL bSuccess = QTVisualContextAcquire(nType, pContext, pPixelFormat, pVisualCtx);
        
        if (!bSuccess)
        {
            NSLog(@">> [QTVisualContext] ERROR: Failure Obtaining a Visual Context!");
        } // if
    } // if
    
    return pVisualCtx;
} // QTVisualContextCreate

#pragma mark
#pragma mark Private - Utilties - Destructor

static void QTVisualContextDelete(QTVisualContextDataRef pVisualCtx)
{
    if(pVisualCtx != NULL)
    {
        if(pVisualCtx->context != NULL)
        {
            QTVisualContextRelease(pVisualCtx->context);
            
            pVisualCtx->context = NULL;
        } // if
        
        free(pVisualCtx);
    } // if
} // QTVisualContextDelete

#pragma mark
#pragma mark Private - Utilties - Images

static inline BOOL QTVisualContextIsNewImageAvailable(const CVTimeStamp * const pTimeStamp,
                                                      QTVisualContextDataRef pVisualCtx)
{
	BOOL bSuccess = pTimeStamp != NULL;
	
    if(bSuccess)
    {
        bSuccess = QTVisualContextIsNewImageAvailable(pVisualCtx->context,
                                                      pTimeStamp);
    } // if
	
	return  bSuccess;
} // QTVisualContextIsNewImageAvailable

// Get a "frame" (image image) from the Visual Context, indexed by the
// provided time.
static CVImageBufferRef QTVisualContextCopyImageForTime(const CVTimeStamp *pTimeStamp,
                                                        QTVisualContextDataRef pVisualCtx)
{
	CVImageBufferRef  pImageBuffer = NULL;
	
    if(pTimeStamp != NULL)
    {
        OSStatus nStatus = QTVisualContextCopyImageForTime(pVisualCtx->context,
                                                           kCFAllocatorDefault,
                                                           pTimeStamp,
                                                           &pImageBuffer);
        
        if((nStatus != noErr) && (pImageBuffer != NULL))
        {
            CFRelease(pImageBuffer);
            
            pImageBuffer = NULL;
        } // if
    } // if
	
	return  pImageBuffer;
} // QTVisualContextCopyImageForTime

#pragma mark
#pragma mark Private - Utilties - Movies

static BOOL QTVisualContextSetMovie(QTMovie *pQTMovie,
                                    QTVisualContextDataRef pVisualCtx)
{
	BOOL  bSuccess = pQTMovie != nil;
    
    if(bSuccess)
    {
        Movie hMovie = [pQTMovie quickTimeMovie];
        
        bSuccess = (hMovie != NULL) && (*hMovie != NULL);
        
        if(bSuccess)
        {
            OSStatus nStatus = SetMovieVisualContext(hMovie, pVisualCtx->context);
            
            bSuccess = nStatus == noErr;
        } // if
    } // if
    
	return  bSuccess;
} // QTVisualContextSetMovie

#pragma mark

@implementation QTVisualContext

#pragma mark
#pragma mark Public - Designated Initializer

// Designated initializer
- (id) initVisualContextWithSize:(const NSSize *)theSize
                            type:(const QTVisualContextType)theType
                         context:(NSOpenGLContext *)theContext
                          format:(NSOpenGLPixelFormat *)thePixelFormat;
{
	self = [super init];
	
	if (self)
	{
        mpVisualCtx = QTVisualContextCreate(theSize, theType, theContext, thePixelFormat);
	} // if
	
	return self;
} // initQTVisualContextWithSize

#pragma mark
#pragma mark Public - Destructor

- (void) dealloc
{
    QTVisualContextDelete(mpVisualCtx);
    
    [super dealloc];
} // dealloc

#pragma mark
#pragma mark Public - Utilities

- (BOOL) isValid
{
	return(mpVisualCtx->context != NULL);
} // isValid

- (BOOL) isImageAvailable:(const CVTimeStamp *)theTimeStamp
{
	return  QTVisualContextIsNewImageAvailable(theTimeStamp, mpVisualCtx);
} // isImageAvailable

// Get a "frame" (image image) from the Visual Context, indexed by the
// provided time.
- (CVImageBufferRef) copyImageForTime:(const CVTimeStamp *)theTimeStamp
{
	return  QTVisualContextCopyImageForTime(theTimeStamp, mpVisualCtx);
} // copyImageForTime

- (BOOL) setMovie:(QTMovie *)theQTMovie
{
	return  QTVisualContextSetMovie(theQTMovie, mpVisualCtx);
} // setMovie

// Give time to the Visual Context so it can release internally held
// resources for later re-use this function should be called in every
// rendering pass, after old images have been released, new images
// have been used and all rendering has been flushed to the screen.
- (void) task
{
	QTVisualContextTask(mpVisualCtx->context);
} // task

@end
