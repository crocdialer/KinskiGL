// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2011, ART+COM AG Berlin, Germany <www.artcom.de>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#import <AVFoundation/AVFoundation.h>
#import <CoreFoundation/CoreFoundation.h>
#include "kinskiGL/Texture.h"
#include "MovieController.h"

namespace kinski {
    
    struct MovieControllerImpl
    {
        AVAssetReaderTrackOutput *m_videoOut, *m_audioOut;
        AVAssetReader *m_assetReader;
        
        AVPlayer *m_player;
        AVPlayerItem *m_player_item;
        AVPlayerItemVideoOutput *m_output;
        
        AVAudioPlayer *m_audioPlayer;
        
        CMTime m_timestamp;
        
        std::string m_src_path;
        bool m_playing;
        bool m_loop;
        float m_rate;
        
        MovieControllerImpl(): m_videoOut(NULL),
                    m_audioOut(NULL),
                    m_assetReader(NULL),
                    m_player(NULL),
                    m_player_item(NULL),
                    m_output(NULL),
                    m_audioPlayer(NULL),
                    m_timestamp(CMTimeMake(0,1)),
                    m_playing(false),
                    m_loop(true),
                    m_rate(1.f){}
        ~MovieControllerImpl()
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
    m_impl(new MovieControllerImpl)
    {
        
    }
    
    MovieController::~MovieController()
    {

    }
        
    bool MovieController::isPlaying() const
    {
        return [m_impl->m_audioPlayer isPlaying];
    }
    
    void MovieController::load(const std::string &filePath)
    {
        m_impl.reset(new MovieControllerImpl);
        
        m_impl->m_src_path = kinski::searchFile(filePath);
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

                 if(!error)
                 {
                     play();
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
            LOG_DEBUG << "starting movie playback";
            [m_impl->m_player seekToTime:kCMTimeZero];
            [m_impl->m_player play];
            [m_impl->m_player setRate: m_impl->m_rate];
            
            m_impl->m_playing = true;
        }
    }
    
    void MovieController::stop()
    {
        m_impl.reset(new MovieControllerImpl);
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
    
    float MovieController::getVolume() const
    {
        return m_impl->m_player.volume;
    }
    
    void MovieController::setVolume(float newVolume)
    {
        float val = newVolume;
        if(val < 0.f) val = 0.f;
        if(val > 1.f) val = 1.f;
        
        m_impl->m_audioPlayer.volume = val;
        m_impl->m_player.volume = val;
        
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

            CVPixelBufferLockBaseAddress(buffer, 0);
            
            tex.update(CVPixelBufferGetBaseAddress(buffer), GL_UNSIGNED_BYTE, GL_BGRA, width, height, true);
            
            CVPixelBufferUnlockBaseAddress(buffer, 0);

            // do not forget to release the buffer
            CFRelease(buffer);
            
            return true;
        }
        return false;
    }
    
    double MovieController::duration()
    {
        AVPlayerItem *playerItem = [m_impl->m_player currentItem];
        
        if ([playerItem status] == AVPlayerItemStatusReadyToPlay)
            return CMTimeGetSeconds([[playerItem asset] duration]);

        return 0.f;
    }
    
    double MovieController::current_time()
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
    
    bool MovieController::loop()
    {
        return m_impl->m_loop;
    }
    
    void MovieController::set_rate(float r)
    {
        m_impl->m_rate = r;
        [m_impl->m_player setRate: r];
    }
    
    const std::string& MovieController::get_path()
    {
        return m_impl->m_src_path;
    }
}
