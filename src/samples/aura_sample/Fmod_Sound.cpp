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
    
    int g_max_num_fft_bands = 8192;
    static std::vector<float> g_fftValues;
    static std::vector<float> g_fftInterpValues;
    static std::vector<float> g_fftSpectrum;
    
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
            
            g_fftValues.assign(g_max_num_fft_bands, 0.f);
            g_fftInterpValues.assign(g_max_num_fft_bands, 0.f);
            g_fftSpectrum.assign(g_max_num_fft_bands, 0.f);
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
//        init_fmod();
//        
//        // 	set to 0
//        std::fill(g_fftInterpValues.begin(), g_fftInterpValues.end(), 0.f);
//        
//        // 	check what the user wants vs. what we can do:
//        if (nBands > g_max_num_fft_bands)
//        {
//            LOG_ERROR<<"error in audio::get_spectrum, the maximum number of bands is 8192";
//            nBands = g_max_num_fft_bands;
//        }
//        else if (nBands <= 0)
//        {
//            LOG_ERROR<<"error in audio::get_spectrum, minimum number of bands is 1";
//            return &g_fftInterpValues[0];
//        }
//        
//        // 	FMOD needs pow2
//        int nBandsToGet = ofNextPow2(nBands);
//        if (nBandsToGet < 64) nBandsToGet = 64;  // can't seem to get fft of 32, etc from fmodex
//        
//        // 	get the fft
//        FMOD_System_GetSpectrum(g_system, fftSpectrum_, nBandsToGet, 0, FMOD_DSP_FFT_WINDOW_HANNING);
//        
//        // 	convert to db scale
//        for(int i = 0; i < nBandsToGet; i++){
//            fftValues_[i] = 10.0f * (float)log10(1 + fftSpectrum_[i]) * 2.0f;
//        }
//        
//        // 	try to put all of the values (nBandsToGet) into (nBands)
//        //  in a way which is accurate and preserves the data:
//        //
//        
//        if (nBandsToGet == nBands){
//            
//            for(int i = 0; i < nBandsToGet; i++){
//                fftInterpValues_[i] = fftValues_[i];
//            }
//            
//        } else {
//            
//            float step 		= (float)nBandsToGet / (float)nBands;
//            //float pos 		= 0;
//            // so for example, if nBands = 33, nBandsToGet = 64, step = 1.93f;
//            int currentBand = 0;
//            
//            for(int i = 0; i < nBandsToGet; i++){
//                
//                // if I am current band = 0, I care about (0+1) * step, my end pos
//                // if i > endPos, then split i with me and my neighbor
//                
//                if (i >= ((currentBand+1)*step)){
//                    
//                    // do some fractional thing here...
//                    float fraction = ((currentBand+1)*step) - (i-1);
//                    float one_m_fraction = 1 - fraction;
//                    fftInterpValues_[currentBand] += fraction * fftValues_[i];
//                    currentBand++;
//                    // safety check:
//                    if (currentBand >= nBands){
//                        ofLog(OF_LOG_ERROR, "ofFmodSoundGetSpectrum - currentBand >= nBands");
//                    }
//                    
//                    fftInterpValues_[currentBand] += one_m_fraction * fftValues_[i];
//                    
//                } else {
//                    // do normal things
//                    fftInterpValues_[currentBand] += fftValues_[i];
//                }
//            }
//            
//            // because we added "step" amount per band, divide to get the mean:
//            for (int i = 0; i < nBands; i++){
//                fftInterpValues_[i] /= step;
//                if (fftInterpValues_[i] > 1)fftInterpValues_[i] = 1; 	// this seems "wrong"
//            }
//            
//        }
        
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
