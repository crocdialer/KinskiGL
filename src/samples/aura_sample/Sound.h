//
//  Sound.h
//  kinskiGL
//
//  Created by Fabian on 6/26/13.
//
//

#ifndef __kinskiGL__Sound__
#define __kinskiGL__Sound__

#include "kinskiCore/Definitions.h"
#include "kinskiCore/Exception.h"
#include "kinskiCore/Logger.h"

namespace kinski{ namespace audio{
    
    //! global audio functions
    void stop_all();
    void set_volume(float vol);
    void update();						
    float* get_spectrum(int nBands);
    void shutdown();
    
    typedef std::shared_ptr<class Sound> SoundPtr;
    
    class Sound
    {
    public:
        
        Sound(){};
        virtual ~Sound(){};
        
        virtual void load(const std::string &fileName, bool stream = false) = 0;
        virtual void unload() = 0;
        virtual void play() = 0;
        virtual void stop() = 0;
        
        virtual void set_volume(float vol) = 0;
        virtual void set_pan(float vol) = 0; // -1 = left, 1 = right
        virtual void set_speed(float spd) = 0;
        virtual void set_paused(bool bP) = 0;
        virtual void set_loop(bool bLp) = 0;
        virtual void set_multiplay(bool bMp) = 0;
        virtual void set_position(float pct) = 0; // 0 = start, 1 = end;
        virtual void set_positionMS(int ms) = 0;
        
        virtual bool playing() = 0;
        virtual float volume() = 0;
        virtual float pan() = 0;
        virtual float speed() = 0;
        virtual bool loop() = 0;
        virtual bool multi_play() = 0;
        virtual bool loaded() = 0;
        virtual float position() = 0;
        virtual int position_ms() = 0;
    };
    
    class SoundLoadException: public Exception
    {
    public:
        SoundLoadException(const std::string &theFilename) :
        Exception(std::string("Got trouble loading an audiofile: ") + theFilename){}
    };
    
    class SoundInput
    {
	public:
        virtual ~SoundInput() {};
        
		virtual void audio_in(float *input, int bufferSize, int nChannels, int deviceID,
                              long unsigned long tickCount)
        {
			//audioIn(input, bufferSize, nChannels);
		}
        
		virtual void audio_in(float *input, int bufferSize, int nChannels )
        {
			//audioReceived(input, bufferSize, nChannels);
		}
        
		virtual void audio_received(float *input, int bufferSize, int nChannels ){}
    };
    
    //----------------------------------------------------------
    //----------------------------------------------------------
    class SoundOutput
    {
	public:
        virtual ~SoundOutput() {};
        
		virtual void audio_out(float *output, int bufferSize, int nChannels, int deviceID,
                              long unsigned long tickCount)
        {
			audio_out(output, bufferSize, nChannels);
		}
        
		virtual void audio_out(float *output, int bufferSize, int nChannels)
        {
			audio_requested(output, bufferSize, nChannels);
		}
        
		//legacy
		virtual void audio_requested( float *output, int bufferSize, int nChannels )
        {
            
		}
    };
    
    class SoundStream
    {
	public:
		virtual ~SoundStream(){}
		
		virtual void listDevices() = 0;
		virtual void setDeviceID(int deviceID) = 0;
		virtual bool setup(int outChannels, int inChannels, int sampleRate, int bufferSize, int nBuffers) = 0;
		//virtual bool setup(ofBaseApp * app, int outChannels, int inChannels, int sampleRate, int bufferSize, int nBuffers) = 0;
		virtual void setInput(SoundInput * soundInput) = 0;
		virtual void setOutput(SoundOutput * soundOutput) = 0;
		
		virtual void start() = 0;
		virtual void stop() = 0;
		virtual void close() = 0;
        
		virtual long unsigned long getTickCount() = 0;
    };
    
}}//namespace

#endif /* defined(__kinskiGL__Sound__) */
