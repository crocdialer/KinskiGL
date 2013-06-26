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
        
        virtual bool load(const std::string &fileName, bool stream = false) = 0;
        virtual void unload() = 0;
        virtual void play() = 0;
        virtual void stop() = 0;
        
        virtual void set_volume(float vol) = 0;
        virtual void set_pan(float vol) = 0; // -1 = left, 1 = right
        virtual void set_speed(float spd) = 0;
        virtual void set_paused(bool bP) = 0;
        virtual void set_loop(bool bLp) = 0;
        virtual void setMultiPlay(bool bMp) = 0;
        virtual void setPosition(float pct) = 0; // 0 = start, 1 = end;
        virtual void setPositionMS(int ms) = 0;
        
        virtual bool playing() = 0;
        virtual float volume() = 0;
        virtual float pan() = 0;
        virtual float speed() = 0;
        virtual bool loop() = 0;
        virtual bool loaded() = 0;
        virtual float position() = 0;
        virtual int position_ms() = 0;
    };
}}//namespace

#endif /* defined(__kinskiGL__Sound__) */
