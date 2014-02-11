#import <AVFoundation/AVFoundation.h>
#include "kinskiGL/Texture.h"
#include "kinskiGL/Buffer.h"
#include "CameraController.h"

@interface Camera : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
{
    AVCaptureSession *_captureSession;
    AVCaptureDeviceInput *_videoInput;
    AVCaptureVideoDataOutput *_videoOutput;

    CMSampleBufferRef _sampleBuffer;
    
    bool _has_new_frame;
}

@property(assign) AVCaptureSession* captureSession;
@property(assign) bool has_new_frame;
@property CMSampleBufferRef sampleBuffer;

@end

@implementation Camera

//@synthesize captureSession;
//@synthesize pixelBuffer;

- (id)init
{
    if (!(self = [super init]))
		return nil;
    
    _sampleBuffer = nil;
    _has_new_frame = false;
    
	// Grab the back-facing camera
	AVCaptureDevice *backFacingCamera = nil;
	NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
	for (AVCaptureDevice *device in devices)
	{
		if ([device position] == AVCaptureDevicePositionUnspecified /*AVCaptureDevicePositionBack*/)
		{
			backFacingCamera = device;
		}
	}
    
	// Create the capture session
	_captureSession = [[AVCaptureSession alloc] init];
	
    // fill configuration queue
    [_captureSession beginConfiguration];
    
	// Add the video input
	NSError *error = nil;
	_videoInput = [[AVCaptureDeviceInput alloc] initWithDevice:backFacingCamera error:&error];
	if ([_captureSession canAddInput:_videoInput])
	{
		[_captureSession addInput:_videoInput];
	}
    
	// Add the video frame output
	_videoOutput = [[AVCaptureVideoDataOutput alloc] init];
	[_videoOutput setAlwaysDiscardsLateVideoFrames:YES];
    
    // Use RGB frames instead of YUV to ease color processing
    // TODO: do YUV colour conversion in shader
	[_videoOutput setVideoSettings:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:kCVPixelFormatType_32BGRA] forKey:(id)kCVPixelBufferPixelFormatTypeKey]];
    
    [_videoOutput setSampleBufferDelegate:self queue:dispatch_get_main_queue()];
    
	if ([_captureSession canAddOutput:_videoOutput])
    {
		[_captureSession addOutput:_videoOutput];
	} else
    {
		LOG_ERROR << "Couldn't add video output";
	}
    
    // too much workload e.g. on Iphone 4s (larger cam-resolution)
    [_captureSession setSessionPreset:AVCaptureSessionPresetHigh];
    
	//[captureSession setSessionPreset:AVCaptureSessionPreset640x480];
    
    // apply all settings as batch
    [_captureSession commitConfiguration];
	
    return self;
}

- (void)dealloc
{
    [super dealloc];
    
    [_captureSession release];
    [_videoInput release];
    [_videoOutput release];
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
        didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
        fromConnection:(AVCaptureConnection *)connection
{
    if(_sampleBuffer)
        CFRelease(_sampleBuffer);
    
    _sampleBuffer = sampleBuffer;
    CFRetain(_sampleBuffer);
    
    _has_new_frame = true;
}

@end

namespace kinski
{
    struct CameraController::Impl
    {
        Camera* m_camera;
        
        gl::Buffer m_pbo[2];
        uint8_t m_pbo_index;
        
        Impl():
        m_camera(NULL),
        m_pbo_index(0)
        {
            m_camera = [[Camera alloc] init];
        }
        
        ~Impl()
        {
            [m_camera release];
        };
    };
    
    CameraController::CameraController():
    m_impl(new Impl)
    {
        
    }
    
    CameraController::~CameraController()
    {

    }
    
    void CameraController::start_capture()
    {
        m_impl->m_pbo[0] = gl::Buffer(GL_PIXEL_UNPACK_BUFFER, GL_STREAM_DRAW);
        m_impl->m_pbo[1] = gl::Buffer(GL_PIXEL_UNPACK_BUFFER, GL_STREAM_DRAW);
        [m_impl->m_camera.captureSession startRunning];
    }
    
    void CameraController::stop_capture()
    {
        [m_impl->m_camera.captureSession stopRunning];
    }
    
    bool CameraController::copy_frame_to_texture(gl::Texture &tex)
    {
        if(!m_impl->m_camera.has_new_frame) return false;
        
        CVPixelBufferRef buffer = CMSampleBufferGetImageBuffer(m_impl->m_camera.sampleBuffer);
        
        if(buffer)
        {
            m_impl->m_camera.has_new_frame = false;
            
            GLuint width = CVPixelBufferGetWidth(buffer);
            GLuint height = CVPixelBufferGetHeight(buffer);
            
            // ping pong our pbo index
            m_impl->m_pbo_index = (m_impl->m_pbo_index + 1) % 2;
            
            size_t num_bytes = CVPixelBufferGetDataSize(buffer);
            
            // lock base adress
            CVPixelBufferLockBaseAddress(buffer, 0);
            
            if(m_impl->m_pbo[m_impl->m_pbo_index].numBytes() != num_bytes)
            {
                m_impl->m_pbo[m_impl->m_pbo_index].setData(NULL, num_bytes);
            }
            
            // map buffer, copy data
            uint8_t *ptr = m_impl->m_pbo[m_impl->m_pbo_index].map();
            memcpy(ptr, CVPixelBufferGetBaseAddress(buffer), num_bytes);
            m_impl->m_pbo[m_impl->m_pbo_index].unmap();
            
            // bind pbo and schedule texture upload
            m_impl->m_pbo[m_impl->m_pbo_index].bind();
            tex.update(NULL, GL_UNSIGNED_BYTE, GL_BGRA, width, height, true);
            m_impl->m_pbo[m_impl->m_pbo_index].unbind();
            
            // unlock base address, release buffer
            CVPixelBufferUnlockBaseAddress(buffer, 0);
//            CFRelease(buffer);
            
            return true;
        }
        return false;
    }
}
