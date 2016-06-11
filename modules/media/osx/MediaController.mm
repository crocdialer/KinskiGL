#import <AVFoundation/AVFoundation.h>
#include "core/file_functions.hpp"
#include "gl/Texture.hpp"
#include "gl/Buffer.hpp"
#include "MediaController.hpp"

@interface LoopHelper : NSObject
{
    kinski::media::MediaControllerImpl *m_movie_control_impl;
}
- (void) initWith: (kinski::media::MediaControllerImpl*) the_impl;

@property(assign) kinski::media::MediaControllerImpl *movie_control_impl;
@end

namespace kinski{ namespace media{
    
    struct MediaControllerImpl
    {
        AVAssetReaderTrackOutput *m_videoOut = nullptr, *m_audioOut = nullptr;
        AVAssetReader *m_assetReader = nullptr;
        
        AVPlayer *m_player = nullptr;
        AVPlayerItem *m_player_item = nullptr;
        AVPlayerItemVideoOutput *m_output = nullptr;
        
        IOSurfaceRef m_io_surface = nullptr;
        GLuint m_output_tex_name = 0;
        
        LoopHelper *m_loop_helper = nullptr;
        
        std::string m_src_path;
        bool m_loaded = false;
        bool m_has_video = false;
        bool m_has_audio = false;
        bool m_playing = false;
        bool m_loop = false;
        float m_rate = 1.f;
        
        MediaController::MediaCallback m_on_load_cb, m_movie_ended_cb;
        std::weak_ptr<MediaController> m_movie_controller;
        
        gl::Buffer m_pbo[2];
        uint8_t m_pbo_index;
        
        MediaControllerImpl():
        m_output_tex_name(0),
        m_pbo_index(0)
        {
            m_loop_helper = [[LoopHelper alloc] init];
            m_loop_helper.movie_control_impl = this;
        }
        ~MediaControllerImpl()
        {
            if(m_videoOut) [m_videoOut release];
            if(m_audioOut) [m_audioOut release];
            if(m_assetReader) [m_assetReader release];
            if(m_player)
            {
                [m_player release];
            }
            if(m_player_item) [m_player_item release];
            if(m_output) [m_output release];
            if(m_loop_helper) [m_loop_helper dealloc];
//            if(m_io_surface && IOSurfaceIsInUse(m_io_surface))
//            {
//                IOSurfaceDecrementUseCount(m_io_surface);
//            }
            if(m_output_tex_name){ glDeleteTextures(1, &m_output_tex_name); }
        };
    };
    
    MediaControllerPtr MediaController::create()
    {
        return MediaControllerPtr(new MediaController());
    }
    
    MediaControllerPtr MediaController::create(const std::string &filePath, bool autoplay,
                                               bool loop, RenderTarget the_render_target,
                                               AudioTarget the_audio_target)
    {
        auto ptr = MediaControllerPtr(new MediaController());
        ptr->load(filePath, autoplay, loop);
        return ptr;
    }
    
    MediaController::MediaController()
    {
        
    }
    
    MediaController::~MediaController()
    {

    }

    void MediaController::load(const std::string &filePath, bool autoplay, bool loop,
                               RenderTarget the_render_target, AudioTarget the_audio_target)
    {
        MediaCallback on_load = m_impl ? m_impl->m_on_load_cb : MediaCallback();
        MediaCallback on_end = m_impl ? m_impl->m_movie_ended_cb : MediaCallback();
        m_impl.reset(new MediaControllerImpl());
        m_impl->m_movie_controller = shared_from_this();
        m_impl->m_on_load_cb = on_load;
        m_impl->m_movie_ended_cb = on_end;
        NSURL *url = nullptr;
        
        if(!fs::is_url(filePath))
        {
            try{ m_impl->m_src_path = fs::search_file(filePath); }
            catch(fs::FileNotFoundException &e)
            {
                LOG_ERROR << e.what();
                return;
            }
            url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:m_impl->m_src_path.c_str()]];
        }
        else
        {
            url = [NSURL URLWithString:[NSString stringWithUTF8String:m_impl->m_src_path.c_str()]
                         relativeToURL:nil];
            m_impl->m_src_path = filePath;
        }
        
        
        AVURLAsset *asset = [AVURLAsset URLAssetWithURL:url options:nil];
        NSString *tracksKey = @"tracks";
        
        [asset loadValuesAsynchronouslyForKeys:[NSArray arrayWithObject:tracksKey] 
                             completionHandler:
       ^{
           if(!m_impl){ return; }
             
           dispatch_async(dispatch_get_main_queue(),
          ^{
                
              // completion code
              NSError *error = nil;
             
              NSArray *videoTrackArray = [asset tracksWithMediaType:AVMediaTypeVideo];
              NSArray *audioTrackArray = [asset tracksWithMediaType:AVMediaTypeAudio];
             
              m_impl->m_has_video = [videoTrackArray count];
              m_impl->m_has_audio = [audioTrackArray count];
             
              LOG_DEBUG << "video-tracks: " << [videoTrackArray count] << " -- audio-tracks: " << [audioTrackArray count];
             
              if(m_impl->m_has_video || m_impl->m_has_audio)
              {
                  @try
                  {
                      m_impl->m_player_item = [[AVPlayerItem alloc] initWithAsset:asset];
                      m_impl->m_player = [[AVPlayer alloc] initWithPlayerItem:m_impl->m_player_item];
                      m_impl->m_player.actionAtItemEnd = loop ? AVPlayerActionAtItemEndNone :
                      AVPlayerActionAtItemEndPause;
                     
                      if(m_impl->m_has_video)
                      {
                          NSDictionary* settings = @{(id)kCVPixelBufferPixelFormatTypeKey : [NSNumber numberWithInt:kCVPixelFormatType_32BGRA], (id) kCVPixelBufferOpenGLCompatibilityKey :[NSNumber numberWithBool:YES]};
                          m_impl->m_output = [[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:settings];
                          [m_impl->m_player_item addOutput:m_impl->m_output];
                         
                          m_impl->m_assetReader = [[AVAssetReader alloc] initWithAsset:asset error:&error];
                          AVAssetTrack *videoTrack = [videoTrackArray objectAtIndex:0];
                          m_impl->m_videoOut = [[AVAssetReaderTrackOutput alloc] initWithTrack:videoTrack outputSettings:settings];
                          [m_impl->m_assetReader addOutput:m_impl->m_videoOut];
                      }
                      if(!error)
                      {
                          m_impl->m_loaded = true;
                          set_loop(loop || m_impl->m_loop);
                         
                          if(autoplay){ play(); }
                          else{ pause(); }
                         
                          if(m_impl->m_on_load_cb){ m_impl->m_on_load_cb(shared_from_this()); }
                      }
                      else{ LOG_ERROR << "Could not load movie file: " << m_impl->m_src_path; }
                  }
                  @catch(NSException *e){ LOG_ERROR << [[e reason] UTF8String]; }
              }
              else{ LOG_ERROR << "No video tracks found in: " << m_impl->m_src_path; }
          });// dispatch async
       }];
    }
    
    bool MediaController::is_loaded() const
    {
        return m_impl && m_impl->m_loaded;
    }
    
    void MediaController::unload()
    {
        m_impl.reset();
    }
    
    bool MediaController::has_video() const
    {
        return m_impl && m_impl->m_has_video;
    }
    
    bool MediaController::has_audio() const
    {
        return m_impl && m_impl->m_has_audio;
    }
    
    void MediaController::play()
    {
        if(!is_loaded()){ return; }
        
        LOG_TRACE << "starting movie playback";
        [m_impl->m_player play];
        [m_impl->m_player setRate: m_impl->m_rate];
        m_impl->m_playing = true;
    }
    
    
    void MediaController::pause()
    {
        if(!is_loaded()){ return; }
        
        [m_impl->m_player pause];
        [m_impl->m_player setRate: 0.f];
        m_impl->m_playing = false;
    }
    
    bool MediaController::is_playing() const
    {
        return m_impl && [m_impl->m_player rate] != 0.0f;
    }
    
    void MediaController::restart()
    {
        if(!is_loaded()){ return; }
        
        [m_impl->m_player seekToTime:kCMTimeZero];
        play();
    }
    
    float MediaController::volume() const
    {
        return m_impl ? m_impl->m_player.volume : 0.f;
    }
    
    void MediaController::set_volume(float the_volume)
    {
        if(!is_loaded()){ return; }
        m_impl->m_player.volume = clamp(the_volume, 0.f, 1.f);
    }
    
    bool MediaController::copy_frame(std::vector<uint8_t>& data, int *width, int *height)
    {
        if(!m_impl || !m_impl->m_playing || !m_impl->m_output || !m_impl->m_player_item) return false;
        
        CMTime ct = [m_impl->m_player currentTime];
        
        if (![m_impl->m_output hasNewPixelBufferForItemTime:ct])
        {
            return false;
        }
        
        CVPixelBufferRef buffer = [m_impl->m_output copyPixelBufferForItemTime:ct itemTimeForDisplay:nil];
        
        if(buffer)
        {
            size_t num_bytes = CVPixelBufferGetDataSize(buffer);
            data.resize(num_bytes);
            
            if(width){*width = CVPixelBufferGetWidth(buffer);}
            if(height){*height = CVPixelBufferGetHeight(buffer);}
            
            // lock base adress
            CVPixelBufferLockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
            memcpy(&data[0], CVPixelBufferGetBaseAddress(buffer), num_bytes);
            
            // unlock base address, release buffer
            CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
            CFRelease(buffer);
            
            return true;
        }
        return false;
    }
    
    bool MediaController::copy_frame_to_texture(gl::Texture &tex, bool as_texture2D)
    {
        if(!m_impl || !is_playing() || !m_impl->m_output || !m_impl->m_player_item) return false;
        
        CMTime ct = [m_impl->m_player currentTime];
        
        if (![m_impl->m_output hasNewPixelBufferForItemTime:ct])
        {
            return false;
        }
        
        CVPixelBufferRef buffer = [m_impl->m_output copyPixelBufferForItemTime:ct itemTimeForDisplay:nil];
        
        if(buffer)
        {
            GLuint width = CVPixelBufferGetWidth(buffer);
            GLuint height = CVPixelBufferGetHeight(buffer);
            
            IOSurfaceRef io_surface = CVPixelBufferGetIOSurface(buffer);
            
            if(!as_texture2D && io_surface)
            {
                if(m_impl->m_io_surface != io_surface) //*IOSurfaceIsInUse(m_impl->m_io_surface)*/)
                {
//                    IOSurfaceDecrementUseCount(m_impl->m_io_surface);
                    IOSurfaceIncrementUseCount(io_surface);
                    m_impl->m_io_surface = io_surface;
                    
                    if(!m_impl->m_output_tex_name) glGenTextures(1, &m_impl->m_output_tex_name);
                    
                    tex = gl::Texture(GL_TEXTURE_RECTANGLE,
                                      m_impl->m_output_tex_name,
                                      width,
                                      height, true);
                    gl::scoped_bind<gl::Texture> tex_bind(tex);
                    CGLTexImageIOSurface2D(CGLGetCurrentContext(),
                                           GL_TEXTURE_RECTANGLE,
                                           GL_RGBA,
                                           width,
                                           height,
                                           GL_BGRA,
                                           GL_UNSIGNED_INT_8_8_8_8_REV,
                                           m_impl->m_io_surface,
                                           0);
                    tex.setFlipped();
                }
            }
            else
            {
                if(!m_impl->m_pbo[0])
                { m_impl->m_pbo[0] = gl::Buffer(GL_PIXEL_UNPACK_BUFFER, GL_STREAM_DRAW); }
                
                if(!m_impl->m_pbo[1])
                { m_impl->m_pbo[1] = gl::Buffer(GL_PIXEL_UNPACK_BUFFER, GL_STREAM_DRAW); }
                
                // ping pong our pbo index
                m_impl->m_pbo_index = (m_impl->m_pbo_index + 1) % 2;
                
                // bool is_planar = CVPixelBufferIsPlanar(buffer);// false
                // int num_planes = CVPixelBufferGetPlaneCount(buffer);// 0
                
                size_t num_bytes = CVPixelBufferGetDataSize(buffer);
                
                // lock base adress
                CVPixelBufferLockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
                
                // copy data to pbo buffer
                m_impl->m_pbo[m_impl->m_pbo_index].set_data(CVPixelBufferGetBaseAddress(buffer),
                                                           num_bytes);
                
                // bind pbo and schedule texture upload
                m_impl->m_pbo[m_impl->m_pbo_index].bind();
                tex.update(nullptr, GL_UNSIGNED_BYTE, GL_BGRA, width, height, true);
                
                m_impl->m_pbo[m_impl->m_pbo_index].unbind();
            }
            
            // unlock base address, release buffer
            CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
            CFRelease(buffer);
            
            return true;
        }
        return false;
    }
    
    bool MediaController::copy_frames_offline(gl::Texture &tex, bool compress)
    {
        if(!m_impl || !m_impl->m_videoOut) return false;
        
        std::list<CMSampleBufferRef> samples;
        
        CMSampleBufferRef sampleBuffer = NULL;
        [m_impl->m_assetReader startReading];
        
        while([m_impl->m_assetReader status] == AVAssetReaderStatusReading)
        {
            sampleBuffer = [m_impl->m_videoOut copyNextSampleBuffer];
            if(sampleBuffer){ samples.push_back(sampleBuffer); }
        }
        
        // determine num frames
        int num_frames = samples.size();
        
        GLuint width, height;
        
        if(!samples.empty())
        {
            CVPixelBufferRef pixbuf = CMSampleBufferGetImageBuffer(samples.front());
            width = CVPixelBufferGetWidth(pixbuf);
            height = CVPixelBufferGetHeight(pixbuf);
            
            // aquire gpu-memory for our frames
            gl::Texture::Format fmt;
            fmt.setTarget(GL_TEXTURE_2D_ARRAY);
            fmt.setInternalFormat(compress ? GL_COMPRESSED_RGBA_S3TC_DXT5_EXT : GL_RGBA);
            tex = gl::Texture(width, height, num_frames, fmt);
            tex.setFlipped();
            
            // swizzle color components
            tex.set_swizzle(GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA);
            KINSKI_CHECK_GL_ERRORS();
        }
        else
        {
            LOG_ERROR << "no samples";
            return false;
        }
        
        int i = 0;
        tex.bind();
        
        // release samples
        for (auto buffer : samples)
        {
            if(buffer)
            {
                CVPixelBufferRef pixbuf = CMSampleBufferGetImageBuffer(buffer);
                
                if(CVPixelBufferLockBaseAddress(pixbuf, kCVPixelBufferLock_ReadOnly) != kCVReturnSuccess)
                {
                    LOG_ERROR << "could not aquire pixelbuffer";
                    continue;
                }
                
                // upload data
                glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, width, height, 1,
                                GL_RGBA, GL_UNSIGNED_BYTE,
                                CVPixelBufferGetBaseAddress(pixbuf));
                
                CVPixelBufferUnlockBaseAddress(pixbuf, kCVPixelBufferLock_ReadOnly);
                i++;
                //
                CFRelease(buffer);
            }
        }
        tex.unbind();
        LOG_TRACE << "copied " << i << " frames into GL_TEXTURE_2D_ARRAY (compression: " << compress<<")";
        KINSKI_CHECK_GL_ERRORS();
        
        return true;
    }
    
    double MediaController::duration() const
    {
        if(!m_impl){ return 0.0; }
        AVPlayerItem *playerItem = [m_impl->m_player currentItem];
        
        if ([playerItem status] == AVPlayerItemStatusReadyToPlay)
            return CMTimeGetSeconds([[playerItem asset] duration]);

        return 0.f;
    }
    
    double MediaController::current_time() const
    {
        return is_loaded() ? CMTimeGetSeconds([m_impl->m_player currentTime]) : 0.0;
    }
    
    void MediaController::seek_to_time(float value)
    {
        if(!m_impl){ return; }
        CMTime t = CMTimeMakeWithSeconds(value, NSEC_PER_SEC);
        [m_impl->m_player seekToTime:t];
        [m_impl->m_player setRate: m_impl->m_rate];
    }
    
    void MediaController::set_loop(bool b)
    {
        if(!m_impl){ return; }
        
        // remove any existing observer
        [[NSNotificationCenter defaultCenter] removeObserver:m_impl->m_loop_helper];
        
        m_impl->m_loop = b;
        
        m_impl->m_player.actionAtItemEnd = AVPlayerActionAtItemEndPause;
        
        // register our loop helper as observer
        [[NSNotificationCenter defaultCenter] addObserver:m_impl->m_loop_helper
                                                 selector:@selector(playerItemDidReachEnd:)
                                                     name:AVPlayerItemDidPlayToEndTimeNotification
                                                   object:[m_impl->m_player currentItem]];
    }
    
    bool MediaController::loop() const
    {
        return m_impl && m_impl->m_loop;
    }
    
    float MediaController::rate() const
    {
        return m_impl ? [m_impl->m_player rate] : 1.f;
    }
    
    void MediaController::set_rate(float r)
    {
        if(!m_impl){ return; }
        
        m_impl->m_rate = r;
        [m_impl->m_player setRate: r];
    }
    
    const std::string& MediaController::path() const
    {
        static std::string ret = "";
        return m_impl ? m_impl->m_src_path : ret;
    }
    
    void MediaController::set_on_load_callback(MediaCallback c)
    {
        if(!m_impl){ return; }
        m_impl->m_on_load_cb = c;
    }
    
    void MediaController::set_media_ended_callback(MediaCallback c)
    {
        if(!m_impl){ return; }
        m_impl->m_movie_ended_cb = c;
    }
    
    MediaController::RenderTarget MediaController::render_target() const
    {
        return RenderTarget::TEXTURE;
    }
    
    MediaController::AudioTarget MediaController::audio_target() const
    {
        return AudioTarget::AUTO;
    }
    
}}// namespaces

@implementation LoopHelper

- (void) initWith: (kinski::media::MediaControllerImpl*) the_impl
{
    [self init];
    self.movie_control_impl = the_impl;
}

- (void) dealloc
{
    [super dealloc];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)playerItemDidReachEnd:(NSNotification *)notification
{
    AVPlayerItem *p = [notification object];
    [p seekToTime:kCMTimeZero];
    
    if(self.movie_control_impl->m_loop)
    {
        [self.movie_control_impl->m_player setRate: self.movie_control_impl->m_rate];
        [self.movie_control_impl->m_player play];
        self.movie_control_impl->m_playing = true;
    }
    else{ self.movie_control_impl->m_playing = false; }
    
    if(self.movie_control_impl->m_movie_ended_cb)
    {
        auto mc = self.movie_control_impl->m_movie_controller.lock();
        if(mc){ self.movie_control_impl->m_movie_ended_cb(mc); }
    }
    
    LOG_TRACE << "playerItemDidReachEnd";
}

@end
