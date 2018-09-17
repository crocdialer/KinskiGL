#import <AVFoundation/AVFoundation.h>
#include "gl/Texture.hpp"
#include "gl/Buffer.hpp"
#include "CameraController.hpp"

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

- (id)initWithDeviceId:(int) device_id
{
    if (!(self = [super init]))
		return nil;
    
    _sampleBuffer = nil;
    _has_new_frame = false;
    
	// Grab the back-facing camera
	AVCaptureDevice *camera = nil;
	NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    
    int num_devices = [devices count];
    
    // no camera found
    if(num_devices == 0)
    {
        LOG_WARNING << "no camera found ...";
        return nil;
    }
    
    device_id = kinski::clamp(device_id, 0, num_devices - 1);
    int i = 0;
    
	for (AVCaptureDevice *device in devices)
	{
        LOG_DEBUG << "camera("<< i <<"): " << [device.localizedName UTF8String];
		if ([device position] == AVCaptureDevicePositionUnspecified /*AVCaptureDevicePositionBack*/
            && i == device_id)
		{
			camera = device;
		}
        i++;
	}
    
	// Create the capture session
	_captureSession = [[AVCaptureSession alloc] init];
	
    // fill configuration queue
    [_captureSession beginConfiguration];
    
	// Add the video input
	NSError *error = nil;
	_videoInput = [[AVCaptureDeviceInput alloc] initWithDevice:camera error:&error];
	if ([_captureSession canAddInput:_videoInput])
	{
		[_captureSession addInput:_videoInput];
	}
    
	// Add the video frame output
	_videoOutput = [[AVCaptureVideoDataOutput alloc] init];
	[_videoOutput setAlwaysDiscardsLateVideoFrames:YES];
    
    // Use RGB frames instead of YUV to ease color processing
    // TODO: do YUV colour conversion in shader?
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
    [_captureSession release];
    [_videoInput release];
    [_videoOutput release];
    
    if(_sampleBuffer)
        CFRelease(_sampleBuffer);
    
    [super dealloc];
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

namespace kinski{ namespace media{
    
    struct CameraControllerImpl
    {
        Camera* m_camera;
        
        gl::Buffer m_pbo[2];
        uint8_t m_pbo_index;

        CameraControllerImpl(int device_id):
        m_camera(nullptr),
        m_pbo_index(0)
        {
            m_camera = [[Camera alloc] initWithDeviceId: device_id];
        }
        
        ~CameraControllerImpl()
        {
            [m_camera dealloc];
        };
    };
    
    CameraControllerPtr CameraController::create(int device_id)
    {
        return CameraControllerPtr(new CameraController(device_id));
    }
    
    CameraController::CameraController(int device_id):
    m_impl(new CameraControllerImpl(device_id))
    {
        
    }
    
    int CameraController::device_id() const
    {
        return -1;
    }
    
    CameraController::~CameraController()
    {
        stop_capture();
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
        
        if(m_impl->m_camera.sampleBuffer)
        {
            CFRelease(m_impl->m_camera.sampleBuffer);
            m_impl->m_camera.sampleBuffer = NULL;
        }
    }
    
    bool CameraController::copy_frame(std::vector<uint8_t>& data, int *width, int *height)
    {
        if(!m_impl->m_camera.has_new_frame) return false;
        
        CVPixelBufferRef buffer = CMSampleBufferGetImageBuffer(m_impl->m_camera.sampleBuffer);
        
        if(buffer)
        {
            m_impl->m_camera.has_new_frame = false;
            
            if(width){*width = CVPixelBufferGetWidth(buffer);}
            if(height){*height = CVPixelBufferGetHeight(buffer);}
                
            size_t num_bytes = CVPixelBufferGetDataSize(buffer);
            data.resize(num_bytes);
            
            // lock base adress
            CVPixelBufferLockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
            memcpy(&data[0], CVPixelBufferGetBaseAddress(buffer), num_bytes);
            
            // unlock base address, release buffer
            CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
            return true;
        }
        return false;
    }
    
    bool CameraController::copy_frame_to_image(ImagePtr& the_image)
    {
        if(!m_impl->m_camera.has_new_frame) return false;
        
        CVPixelBufferRef buffer = CMSampleBufferGetImageBuffer(m_impl->m_camera.sampleBuffer);
        
        if(buffer)
        {
            m_impl->m_camera.has_new_frame = false;
            uint32_t w = CVPixelBufferGetWidth(buffer);
            uint32_t h = CVPixelBufferGetHeight(buffer);
            constexpr uint8_t num_channels = 4;
            
            if(!the_image || the_image->width() != w || the_image->height() != h ||
               the_image->num_components() != num_channels)
            {
                auto img = Image_<uint8_t>::create(w, h, num_channels);
                img->m_type = Image::Type::BGRA;
                the_image = img;
            }
            
            // the buffer seems to hold some extra bytes at the end -> cap
            size_t num_bytes = std::min(CVPixelBufferGetDataSize(buffer), the_image->num_bytes());
            
            // lock base adress
            CVPixelBufferLockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
            memcpy(the_image->data(), CVPixelBufferGetBaseAddress(buffer), num_bytes);
            
            // unlock base address, release buffer
            CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
            return true;
        }
        return false;
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
            
            if(m_impl->m_pbo[m_impl->m_pbo_index].num_bytes() != num_bytes)
            {
                m_impl->m_pbo[m_impl->m_pbo_index].set_data(NULL, num_bytes);
            }
            
            // map buffer, copy data
            uint8_t *ptr = m_impl->m_pbo[m_impl->m_pbo_index].map();
            memcpy(ptr, CVPixelBufferGetBaseAddress(buffer), num_bytes);
            m_impl->m_pbo[m_impl->m_pbo_index].unmap();
            
            // bind pbo and schedule texture upload
            m_impl->m_pbo[m_impl->m_pbo_index].bind();
            tex.update(nullptr, GL_UNSIGNED_BYTE, GL_BGRA, width, height, true);
            m_impl->m_pbo[m_impl->m_pbo_index].unbind();
            
            // unlock base address, release buffer
            CVPixelBufferUnlockBaseAddress(buffer, 0);

            return true;
        }
        return false;
    }
    
    bool CameraController::is_capturing() const
    {
        return m_impl->m_camera.captureSession.isRunning;
    }

    void CameraController::set_capture_mode(const capture_mode_t &the_mode)
    {
        LOG_WARNING << "CameraController::set_capture_mode(const capture_mode_t &the_mode) NOT IMPLEMENTED ...";
    }

}}
