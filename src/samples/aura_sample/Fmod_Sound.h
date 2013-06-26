//
//  Fmod_Sound.h
//  kinskiGL
//
//  Created by Fabian on 6/26/13.
//
//

#ifndef __kinskiGL__Fmod_Sound__
#define __kinskiGL__Fmod_Sound__

#include "Sound.h"

namespace kinski{ namespace audio{
    
    class Fmod_Sound : public Sound
    {
    public:
        
        Fmod_Sound(){};
        virtual ~Fmod_Sound(){};
        
        bool load(const std::string &fileName, bool stream = false);
        void unload();
        void play();
        void stop();
        
        void set_volume(float vol);
        void set_pan(float vol); // -1 = left, 1 = right
        void set_speed(float spd);
        void set_paused(bool bP);
        void set_loop(bool bLp);
        void setMultiPlay(bool bMp);
        void setPosition(float pct); // 0 = start, 1 = end;
        void setPositionMS(int ms);
        
        bool playing();
        float volume();
        float pan();
        float speed();
        bool loop();
        bool loaded();
        float position();
        int position_ms();
    };
}}//namespace

#endif /* defined(__kinskiGL__Fmod_Sound__) */