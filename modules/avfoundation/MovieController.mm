#import <AVFoundation/AVFoundation.h>
#include "kinskiGL/Texture.h"
#include "kinskiGL/ArrayTexture.h"
#include "kinskiGL/Buffer.h"
#include "MovieController.h"

namespace kinski {
    
    struct MovieController::Impl
    {
        AVAssetReaderTrackOutput *m_videoOut, *m_audioOut;
        AVAssetReader *m_assetReader;
        
        AVPlayer *m_player;
        AVPlayerItem *m_player_item;
        AVPlayerItemVideoOutput *m_output;
        AVAudioPlayer *m_audioPlayer;
        
        std::string m_src_path;
        bool m_playing;
        bool m_loop;
        float m_rate;
        
        MovieCallback m_on_load_cb;
        
        gl::Buffer m_pbo[2];
        uint8_t m_pbo_index;
        
        Impl(): m_videoOut(NULL),
                m_audioOut(NULL),
                m_assetReader(NULL),
                m_player(NULL),
                m_player_item(NULL),
                m_output(NULL),
                m_audioPlayer(NULL),
                m_playing(false),
                m_loop(true),
                m_rate(1.f),
                m_pbo_index(0){}
        ~Impl()
        {
            if(m_videoOut) [m_videoOut release];
            if(m_audioOut) [m_audioOut release];
            if(m_assetReader) [m_assetReader release];
            if(m_player) [m_player release];
            if(m_player_item) [m_player_item release];
            if(m_output) [m_output release];
            if(m_audioPlayer) [m_audioPlayer release];
        };
    };
    
    MovieController::MovieController():
    m_impl(new Impl)
    {
        
    }
    
    MovieController::~MovieController()
    {

    }
        
    bool MovieController::isPlaying() const
    {
        return [m_impl->m_player rate] != 0.0f;
    }
    
    void MovieController::load(const std::string &filePath, bool autoplay)
    {
        MovieCallback on_load = m_impl->m_on_load_cb;
        m_impl.reset(new Impl);
        m_impl->m_on_load_cb = on_load;
        
        m_impl->m_pbo[0] = gl::Buffer(GL_PIXEL_UNPACK_BUFFER, GL_STREAM_DRAW);
        m_impl->m_pbo[1] = gl::Buffer(GL_PIXEL_UNPACK_BUFFER, GL_STREAM_DRAW);
        
        try{ m_impl->m_src_path = kinski::searchFile(filePath); }
        catch(FileNotFoundException &e)
        {
            LOG_ERROR << e.what();
            return;
        }
        
        NSURL *url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:m_impl->m_src_path.c_str()]];
        
        // create new audio-player instance
        m_impl->m_audioPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:nil];
        
        AVURLAsset *asset = [AVURLAsset URLAssetWithURL:url options:nil];
        
        NSString *tracksKey = @"tracks";
        
        [asset loadValuesAsynchronouslyForKeys:[NSArray arrayWithObject:tracksKey] 
                             completionHandler:
         ^{
             // completion code
             NSError *error = nil;
             
             NSArray *videoTrackArray = [asset tracksWithMediaType:AVMediaTypeVideo];
//             NSArray *audioTrackArray = [asset tracksWithMediaType:AVMediaTypeAudio];
             
             if([videoTrackArray count])
             {
                 NSDictionary* settings = @{ (id)kCVPixelBufferPixelFormatTypeKey : [NSNumber numberWithInt:kCVPixelFormatType_32BGRA] };
                 m_impl->m_output = [[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:settings];
             
                 m_impl->m_player_item = [[AVPlayerItem playerItemWithAsset:asset] retain];
                 [m_impl->m_player_item addOutput:m_impl->m_output];
                 m_impl->m_player = [[AVPlayer playerWithPlayerItem:m_impl->m_player_item] retain];
                 
                 m_impl->m_assetReader = [[AVAssetReader alloc] initWithAsset:asset error:&error];
                 
                 AVAssetTrack *videoTrack = [videoTrackArray objectAtIndex:0];
                 m_impl->m_videoOut = [[AVAssetReaderTrackOutput alloc] initWithTrack:videoTrack outputSettings:settings];

                 if(!error)
                 {
                     if(autoplay)
                         play();
                     
                     [m_impl->m_assetReader addOutput:m_impl->m_videoOut];
                  
                     if(m_impl->m_on_load_cb)
                         m_impl->m_on_load_cb(*this);
                 }
                 else
                 {
                     LOG_ERROR << "Could not load movie file: " << m_impl->m_src_path;
                 }
             }
         }];
    }
    void MovieController::play()
    {        
        // already playing. nothing to do
        //if(m_impl->m_player)
        {
            LOG_TRACE << "starting movie playback";
            [m_impl->m_player seekToTime:kCMTimeZero];
            [m_impl->m_player play];
            [m_impl->m_player setRate: m_impl->m_rate];
            
            m_impl->m_playing = true;
        }
    }
    
    void MovieController::stop()
    {
        m_impl.reset(new Impl);
        m_impl->m_playing = false;
    }
    
    void MovieController::pause()
    {
        if(m_impl->m_playing)
        {
            [m_impl->m_player pause];
        }
        else
        {
            [m_impl->m_player play];
            [m_impl->m_player setRate: m_impl->m_rate];
        }
        m_impl->m_playing = !m_impl->m_playing;
    }
    
    float MovieController::volume() const
    {
        return m_impl->m_player.volume;
    }
    
    void MovieController::set_volume(float newVolume)
    {
        float val = newVolume;
        if(val < 0.f) val = 0.f;
        if(val > 1.f) val = 1.f;
        
        m_impl->m_audioPlayer.volume = val;
        m_impl->m_player.volume = val;
    }
    
    bool MovieController::copy_frame(std::vector<uint8_t>& data)
    {
        if(!m_impl->m_playing || !m_impl->m_output || !m_impl->m_player_item) return false;
        
        if(m_impl->m_loop && current_time() >= duration())
        {
            play();
        }
        
        CMTime ct = [m_impl->m_player currentTime];
        
        if (![m_impl->m_output hasNewPixelBufferForItemTime:ct])
            return false;
        
        CVPixelBufferRef buffer = [m_impl->m_output copyPixelBufferForItemTime:ct itemTimeForDisplay:nil];
        
        if(buffer)
        {
            size_t num_bytes = CVPixelBufferGetDataSize(buffer);
            data.resize(num_bytes);
            
            // lock base adress
            CVPixelBufferLockBaseAddress(buffer, 0);
            memcpy(&data[0], CVPixelBufferGetBaseAddress(buffer), num_bytes);
            
            // unlock base address, release buffer
            CVPixelBufferUnlockBaseAddress(buffer, 0);
            CFRelease(buffer);
            
            return true;
        }
        return false;
    }
    
    bool MovieController::copy_frame_to_texture(gl::Texture &tex)
    {
        if(!m_impl->m_playing || !m_impl->m_output || !m_impl->m_player_item) return false;

        if(m_impl->m_loop && current_time() >= duration())
        {
            play();
        }
        
        CMTime ct = [m_impl->m_player currentTime];
        
        if (![m_impl->m_output hasNewPixelBufferForItemTime:ct])
            return false;
            
        CVPixelBufferRef buffer = [m_impl->m_output copyPixelBufferForItemTime:ct itemTimeForDisplay:nil];
        
        if(buffer)
        {
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
            CFRelease(buffer);
            
            return true;
        }
        return false;
    }
    
    bool MovieController::copy_frames_offline(gl::ArrayTexture &tex)
    {
        if(!m_impl->m_videoOut) return false;
        
        std::list<CMSampleBufferRef> samples;
        
        CMSampleBufferRef sampleBuffer = NULL;
        [m_impl->m_assetReader startReading];
        
        while([m_impl->m_assetReader status] == AVAssetReaderStatusReading)
        {
            sampleBuffer = [m_impl->m_videoOut copyNextSampleBuffer];
            samples.push_back(sampleBuffer);
        }
        
        // determine num frames
        int num_frames = samples.size();
        
        GLuint width, height;
        
        if(!samples.empty())
        {
            CVPixelBufferRef pixbuf = CMSampleBufferGetImageBuffer(samples.front());
//            LOG_DEBUG << "planar: " << (bool) CVPixelBufferIsPlanar(pixbuf);
            
            width = CVPixelBufferGetWidth(pixbuf);
            height = CVPixelBufferGetHeight(pixbuf);
            
            // aquire gpu-memory for our frames
            gl::ArrayTexture::Format fmt;
            fmt.setInternalFormat(GL_BGRA);
            tex = gl::ArrayTexture(width, height, num_frames - 1);
            tex.setFlipped();
            KINSKI_CHECK_GL_ERRORS();
        }
        else
        {
            LOG_ERROR << "no samples";
            return false;
        }
        
        int i = 0;
        tex.bind();
        
//        GLint swizzleMask[] = {GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA};
//        glTexParameteriv(tex.getTarget(), GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
        
        // release samples
        for (auto buffer : samples)
        {
            if(buffer)
            {
                CVPixelBufferRef pixbuf = CMSampleBufferGetImageBuffer(buffer);
                
                if(CVPixelBufferLockBaseAddress(pixbuf, 0) != kCVReturnSuccess)
                {
                    LOG_ERROR << "could not aquire pixelbuffer";
                    continue;
                }
                
                // upload data
                glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, width, height, 1,
                                GL_BGRA, GL_UNSIGNED_BYTE,
                                CVPixelBufferGetBaseAddress(pixbuf));
                
                CVPixelBufferUnlockBaseAddress(pixbuf, 0);
                i++;
                //
                CFRelease(buffer);
            }
        }
        tex.unbind();
        LOG_DEBUG << i <<" frames";
        KINSKI_CHECK_GL_ERRORS();
        
        return true;
    }
    
    double MovieController::duration() const
    {
        AVPlayerItem *playerItem = [m_impl->m_player currentItem];
        
        if ([playerItem status] == AVPlayerItemStatusReadyToPlay)
            return CMTimeGetSeconds([[playerItem asset] duration]);

        return 0.f;
    }
    
    double MovieController::current_time() const
    {
        return CMTimeGetSeconds([m_impl->m_player currentTime]);
    }
    
    void MovieController::seek_to_time(float value)
    {
        CMTime t = CMTimeMakeWithSeconds(value, NSEC_PER_SEC);
        [m_impl->m_player seekToTime:t];
        [m_impl->m_player setRate: m_impl->m_rate];
    }
    
    void MovieController::set_loop(bool b)
    {
        m_impl->m_loop = b;
    }
    
    bool MovieController::loop() const
    {
        return m_impl->m_loop;
    }
    
    void MovieController::set_rate(float r)
    {
        m_impl->m_rate = r;
        [m_impl->m_player setRate: r];
    }
    
    const std::string& MovieController::get_path() const
    {
        return m_impl->m_src_path;
    }
    
    void MovieController::set_on_load_callback(MovieCallback c)
    {
        m_impl->m_on_load_cb = c;
    }
}
