//
//  Fmod_Sound.cpp
//  kinskiGL
//
//  Created by Fabian on 6/26/13.
//
//

#include "kinskiCore/file_functions.h"
#include "Fmod_Sound.h"

namespace kinski{ namespace audio{

    ////////////////////// implementation internal /////////////////////////////
    
    static FMOD_CHANNELGROUP* g_channelgroup = NULL;
    static FMOD_SYSTEM* g_system = NULL;
    
    void init_fmod()
    {
        if(!g_system)
        {
            FMOD_System_Create(&g_system);
#ifdef TARGET_LINUX
			FMOD_System_SetOutput(sys,FMOD_OUTPUTTYPE_ALSA);
#endif
            FMOD_System_Init(g_system, 32, FMOD_INIT_NORMAL, NULL);  //do we want just 32 channels?
            FMOD_System_GetMasterChannelGroup(g_system, &g_channelgroup);
        }
    }
    
    /////////////////////////// global audio functions /////////////////////////

    void stop_all()
    {
        init_fmod();
        FMOD_ChannelGroup_Stop(g_channelgroup);
    }
    
    void set_volume(float vol)
    {
        init_fmod();
        FMOD_ChannelGroup_SetVolume(g_channelgroup, vol);
    }
    
    void update()
    {
        if(g_system)
        {
            FMOD_System_Update(g_system);
        }
    }
    
    float* get_spectrum(int nBands)
    {
        init_fmod();
        throw kinski::Exception("not implemented");
        
        return NULL;
    }
    
    void shutdown()
    {
        if(g_system)
        {
            FMOD_System_Close(g_system);
        }
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    Fmod_Sound::Fmod_Sound(const std::string &file_name):
    m_streaming(false),
    m_multiplay(false),
    m_loop(false),
    m_paused(false),
    m_pan(0.f),
    m_volume(1.f),
    m_internal_freq(44100),
    m_speed(1),
    m_channel(NULL),
    m_sound(NULL)
    {
        if(!file_name.empty())
            load(file_name);
    }
    
    Fmod_Sound::~Fmod_Sound()
    {
        
    }
    
    void Fmod_Sound::load(const std::string &fileName, bool stream)
    {
        std::string path = searchFile(fileName);
        m_multiplay = false;
        init_fmod();
        unload();
        
        //choose if we want streaming
        int fmodFlags =  FMOD_HARDWARE; // FMOD_HARDWARE !?
        if(stream)fmodFlags =  FMOD_HARDWARE | FMOD_CREATESTREAM;
        
        FMOD_RESULT result;
        result = FMOD_System_CreateSound(g_system, path.c_str(), fmodFlags, NULL, &m_sound);
        
        if (result != FMOD_OK){throw SoundLoadException(fileName);}

        FMOD_Sound_GetLength(m_sound, &m_length, FMOD_TIMEUNIT_PCM);
        m_streaming = stream;
        
        LOG_DEBUG<<"loaded sound (streaming: "<< (stream ? "on":"off") << "): "<<fileName;
    }
    
    void Fmod_Sound::unload()
    {
        if (loaded())
        {
            stop();
            if(!m_streaming) FMOD_Sound_Release(m_sound);
            m_sound = NULL;
        }
    }
    
    void Fmod_Sound::play()
    {
        // if it's a looping sound kill it
        if (m_loop){FMOD_Channel_Stop(m_channel);}
        
        // if the sound is not set to multiplay, then stop the current,
        // before we start another
        if (!m_multiplay){FMOD_Channel_Stop(m_channel);}
        
        FMOD_System_PlaySound(g_system, FMOD_CHANNEL_FREE, m_sound, m_paused, &m_channel);
        FMOD_Channel_GetFrequency(m_channel, &m_internal_freq);
        FMOD_Channel_SetVolume(m_channel, m_volume);
        set_pan(m_pan);
        FMOD_Channel_SetFrequency(m_channel, m_internal_freq * m_speed);
        FMOD_Channel_SetMode(m_channel, m_loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
        
        //fmod update() should be called every frame - according to the docs.
        //we have been using fmod without calling it at all which resulted in channels not being able
        //to be reused.  we should have some sort of global update function but putting it here
        //solves the channel bug
        FMOD_System_Update(g_system);
    }
    
    void Fmod_Sound::stop()
    {
        FMOD_Channel_Stop(m_channel);
    }
    
    void Fmod_Sound::set_volume(float vol)
    {
        if (playing()){FMOD_Channel_SetVolume(m_channel, vol);}
        m_volume = vol;
    }
    
    void Fmod_Sound::set_pan(float pan)
    {
        m_pan = kinski::clamp(pan, -1.f, 1.f);
        if (playing()){FMOD_Channel_SetPan(m_channel, m_pan);}
    }
    
    void Fmod_Sound::set_speed(float speed)
    {
        if (playing()){FMOD_Channel_SetFrequency(m_channel, m_internal_freq * speed);}
        m_speed = speed;
    }
    
    void Fmod_Sound::set_paused(bool b)
    {
        if (playing())
        {
            FMOD_Channel_SetPaused(m_channel, b);
            m_paused = b;
        }
    }
    
    void Fmod_Sound::set_loop(bool b)
    {
        if (playing()){FMOD_Channel_SetMode(m_channel, b ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);}
        m_loop = b;
    }
    
    void Fmod_Sound::set_multiplay(bool b)
    {
        m_multiplay = b;
    }
    
    void Fmod_Sound::set_position(float pct)
    {
        if (playing())
        {
            int sample = (int)(m_length * pct);
            FMOD_Channel_SetPosition(m_channel, sample, FMOD_TIMEUNIT_PCM);
        }
    }
    
    void Fmod_Sound::set_positionMS(int ms)
    {
        if (playing()){FMOD_Channel_SetPosition(m_channel, ms, FMOD_TIMEUNIT_MS);}
    }
    
    bool Fmod_Sound::playing()
    {
        if (!loaded()) return false;
        
        int playing = 0;
        FMOD_Channel_IsPlaying(m_channel, &playing);
        return playing;
    }
    
    float Fmod_Sound::volume()
    {
        return m_volume;
    }
    
    float Fmod_Sound::pan()
    {
        return m_pan;
    }
    
    float Fmod_Sound::speed()
    {
        return m_speed;
    }
    
    bool Fmod_Sound::loop()
    {
        return m_loop;
    }
    
    bool Fmod_Sound::multi_play()
    {
        return m_multiplay;
    }
    
    bool Fmod_Sound::loaded()
    {
        return m_sound;
    }
    
    float Fmod_Sound::position()
    {
        if (playing())
        {
            unsigned int sample;
            FMOD_Channel_GetPosition(m_channel, &sample, FMOD_TIMEUNIT_PCM);
            float percent = 0.0f;
            if (m_length > 0)
            {
                percent = sample / (float)m_length;
            }
            return percent;
        } 
        return 0;
    }
    
    int Fmod_Sound::position_ms()
    {
        if (playing())
        {
            unsigned int sample;
            FMOD_Channel_GetPosition(m_channel, &sample, FMOD_TIMEUNIT_MS);
            return sample;
        }
        return 0;
    }
}}
