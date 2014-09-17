//
//  Fmod_Sound.h
//  gl
//
//  Created by Fabian on 6/26/13.
//
//

#ifndef __gl__Fmod_Sound__
#define __gl__Fmod_Sound__

#include "fmod.hpp"
#include "fmod_errors.h"
#include "Sound.h"

namespace kinski{ namespace audio{
    
    class Fmod_Sound : public Sound
    {
    public:
        
        Fmod_Sound(const std::string &file_name = "", int device = 0);
        virtual ~Fmod_Sound();
        
        void load(const std::string &fileName, bool stream = false);
        void unload();
        void play();
        void stop();
        void record(float num_secs = 5.f, int device = 0);
        void get_spectrum(std::vector<float> &buffer, int num_buckets = 0);
        void get_pcm_buffer(float *buffer, int num_samples);
        
        void set_volume(float vol);
        void set_pan(float pan); // -1 = left, 1 = right
        void set_speed(float speed);
        void set_paused(bool b);
        void set_loop(bool b);
        void set_multiplay(bool b);
        void set_position(float pct); // 0 = start, 1 = end;
        void set_positionMS(int ms);
        
        bool is_recording(int device = 0);
        bool playing();
        float volume();
        float pan();
        float speed();
        bool loop();
        bool multi_play();
        bool loaded();
        float position();
        int position_ms();
        
    private:
        int m_device;
        bool m_streaming;
		bool m_multiplay;
		bool m_loop;
		bool m_paused;
		float m_pan; // -1 to 1
		float m_volume; // 0 - 1
		float m_internal_freq; // 44100 ?
		float m_speed; // -n to n, 1 = normal, -1 backwards
		unsigned int m_length; // in samples;
        
        FMOD::Channel *m_channel;
        FMOD::Sound *m_sound;
        
    };
}}//namespace

#endif /* defined(__gl__Fmod_Sound__) */