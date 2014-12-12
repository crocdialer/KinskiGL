//
//  Fmod_Sound.cpp
//  gl
//
//  Created by Fabian on 6/26/13.
//
//

#include "core/file_functions.h"
#include "Fmod_Sound.h"
#include <cmath>

namespace kinski{ namespace audio{

    ////////////////////// implementation internal /////////////////////////////
    
    static std::vector<FMOD::ChannelGroup*> g_channelgroups = {nullptr, nullptr, nullptr, nullptr};
    static std::vector<FMOD::System*> g_systems = {nullptr, nullptr, nullptr, nullptr};
    
    int g_max_num_fft_bands = 8192;
    static std::vector<float> g_fftValues;
    static std::vector<float> g_fftInterpValues;
    static std::vector<float> g_fftSpectrum;
    
    #define CHECK_ERROR(result) \
        if(result != FMOD_OK) \
            LOG_ERROR << "FMOD error! " << result << ": " << FMOD_ErrorString(result);
    
    void init_fmod(int device = 0)
    {
        if(!g_systems[device])
        {
            FMOD_RESULT result;
            result = FMOD::System_Create(&g_systems[device]);
            if(result != FMOD_OK) throw SystemException();
            
#ifdef TARGET_LINUX
            FMOD::System_SetOutput(sys,FMOD_OUTPUTTYPE_ALSA);
#endif
            g_systems[device]->setDriver(clamp<int>(device, 0, get_output_devices().size() - 1));
            
            g_systems[device]->setSpeakerMode(FMOD_SPEAKERMODE_5POINT1);
            g_systems[device]->init(32, FMOD_INIT_NORMAL, NULL);//do we want just 32 channels?
            g_systems[device]->getMasterChannelGroup(&g_channelgroups[device]);
            
            g_fftValues.assign(g_max_num_fft_bands, 0.f);
            g_fftInterpValues.assign(g_max_num_fft_bands, 0.f);
            g_fftSpectrum.assign(g_max_num_fft_bands, 0.f);
        }
    }
    
    /////////////////////////// global audio functions /////////////////////////

    void stop_all()
    {
        init_fmod();
        
        for(auto ch : g_channelgroups)
        {
            if(ch){ ch->stop(); }
        }
    }
    
    void set_volume(float vol)
    {
        init_fmod();
        for(auto ch : g_channelgroups)
        {
            if(ch){ ch->setVolume(vol); }
        }
    }
    
    void update()
    {
        for(auto sys : g_systems)
        {
            if(sys){ sys->update(); }
        }
    }
    
    void shutdown()
    {
        for(auto sys : g_systems)
        {
            if(sys){ sys->close(); }
        }
    }
    
    int nextPow2(int v)
    {
        int i = 1;
        while (i < v){i <<= 1;}
        return i;
    }
    
    std::vector<device> get_output_devices()
    {
        init_fmod(0);
        
        std::vector<device> ret;
        int num_drivers, num_channels;
        g_systems[0]->getNumDrivers(&num_drivers);
        g_systems[0]->getHardwareChannels(&num_channels);
		
		for(int i=0; i < num_drivers; i++)
		{
			char name[256];
			g_systems[0]->getDriverInfo(i, name, 256, nullptr);
            ret.push_back({name, 2});
		}
        return ret;
    }
    
    std::vector<audio::device> get_recording_devices()
    {
        init_fmod(0);
        
        std::vector<audio::device> ret;
        FMOD_RESULT result;
        int numdrivers;
        result = g_systems[0]->getRecordNumDrivers(&numdrivers);
        CHECK_ERROR(result);
        
        for (int count = 0; count < numdrivers; count++)
        {
            char name[256];
            
            result = g_systems[0]->getRecordDriverInfo(count, name, 256, 0);
            CHECK_ERROR(result);
            
            ret.push_back({name, 1});
        }
        return ret;
    }
    
    bool rebase_buffer(const std::vector<float> &in_buf,
                       std::vector<float> &out_buf,
                       int num_buckets)
    {
        // 	set to 0
        std::fill(g_fftInterpValues.begin(), g_fftInterpValues.end(), 0.f);
        
        // 	check what the user wants vs. what we can do:
        if (num_buckets > g_max_num_fft_bands)
        {
            LOG_ERROR << "error in audio::get_spectrum, the maximum number of bands is 8192";
            num_buckets = g_max_num_fft_bands;
        }
        else if (num_buckets <= 0)
        {
            LOG_ERROR << "error in audio::get_spectrum, minimum number of bands is 1";
            return false;
        }
        
        // 	FMOD needs pow2
        int nBandsToGet = std::max(nextPow2(num_buckets), 64);
        
        // resize our output buffer
        out_buf.resize(num_buckets);
        
        // 	convert to db scale
        for(int i = 0; i < nBandsToGet; i++)
        {
            g_fftValues[i] = 10.0f * (float)log10(1 + in_buf[i]) * 2.0f;
        }
        
        // 	try to put all of the values (nBandsToGet) into (nBands)
        //  in a way which is accurate and preserves the data:
        //
        if (nBandsToGet == num_buckets)
        {
            for(int i = 0; i < nBandsToGet; i++)
            {
                g_fftInterpValues[i] = g_fftValues[i];
            }
        }
        else
        {
            float step = (float)nBandsToGet / (float)num_buckets;
            //float pos 		= 0;
            // so for example, if nBands = 33, nBandsToGet = 64, step = 1.93f;
            int currentBand = 0;
            
            for(int i = 0; i < nBandsToGet; i++)
            {
                
                // if I am current band = 0, I care about (0+1) * step, my end pos
                // if i > endPos, then split i with me and my neighbor
                
                if (i >= ((currentBand+1)*step))
                {
                    
                    // do some fractional thing here...
                    float fraction = ((currentBand+1)*step) - (i-1);
                    float one_m_fraction = 1 - fraction;
                    g_fftInterpValues[currentBand] += fraction * g_fftValues[i];
                    currentBand++;
                    // safety check:
                    if (currentBand >= num_buckets)
                    {
                        LOG_ERROR<<"get_spectrum - currentBand >= nBands";
                    }
                    
                    g_fftInterpValues[currentBand] += one_m_fraction * g_fftValues[i];
                }
                else
                {
                    // do normal things
                    g_fftInterpValues[currentBand] += g_fftValues[i];
                }
            }
            
            // because we added "step" amount per band, divide to get the mean:
            for (int i = 0; i < num_buckets; i++)
            {
                g_fftInterpValues[i] /= step;
                if (g_fftInterpValues[i] > 1) g_fftInterpValues[i] = 1; 	// this seems "wrong"
            }
        }
        memcpy(&out_buf[0], &g_fftInterpValues[0], num_buckets * sizeof(g_fftInterpValues[0]));
        return true;
    }
    
    float* get_spectrum(int num_buckets)
    {
        init_fmod(0);
        
        // 	check what the user wants vs. what we can do:
        if (num_buckets > g_max_num_fft_bands)
        {
            LOG_ERROR << "error in audio::get_spectrum, the maximum number of bands is 8192";
            num_buckets = g_max_num_fft_bands;
        }
        else if (num_buckets <= 0)
        {
            LOG_ERROR << "error in audio::get_spectrum, minimum number of bands is 1";
            return NULL;
        }
        
        // 	FMOD needs pow2
        int nBandsToGet = std::max(nextPow2(num_buckets), 64);
        
        // 	get the fft
        g_systems[0]->getSpectrum(&g_fftSpectrum[0], nBandsToGet, 0, FMOD_DSP_FFT_WINDOW_HANNING);
        
        // rebase buffer to desired bucket size
        rebase_buffer(g_fftSpectrum, g_fftInterpValues, nBandsToGet);
        
        return &g_fftInterpValues[0];
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    Fmod_Sound::Fmod_Sound(const std::string &file_name, int device, bool stream):
    m_device(device),
    m_streaming(false),
    m_multiplay(false),
    m_loop(false),
    m_paused(false),
    m_pan(0.f),
    m_speaker_mix({1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}),
    m_volume(1.f),
    m_internal_freq(44100),
    m_speed(1),
    m_channel(NULL),
    m_sound(NULL)
    {
        if(!file_name.empty())
            load(file_name, stream);
    }
    
    Fmod_Sound::~Fmod_Sound()
    {
        
    }
    
    void Fmod_Sound::load(const std::string &fileName, bool stream)
    {
        std::string path = searchFile(fileName);
        m_multiplay = false;
        init_fmod(m_device);
        unload();
        
        //choose if we want streaming
        int fmodFlags =  FMOD_SOFTWARE | FMOD_2D; // FMOD_HARDWARE !?
        if(stream)fmodFlags =  FMOD_SOFTWARE | FMOD_CREATESTREAM;
        
        FMOD_RESULT result;
        result = g_systems[m_device]->createSound(path.c_str(), fmodFlags, NULL, &m_sound);
        
        if (result != FMOD_OK){throw SoundLoadException(fileName);}

        m_sound->getLength(&m_length, FMOD_TIMEUNIT_PCM);
        m_streaming = stream;
        
        LOG_DEBUG<<"loaded sound (streaming: "<< (stream ? "on":"off") << "): "<<fileName;
    }
    
    void Fmod_Sound::unload()
    {
        if (loaded())
        {
            stop();
            if(!m_streaming) m_sound->release();
            m_sound = NULL;
        }
    }
    
    void Fmod_Sound::play()
    {
        // if it's a looping sound kill it
        if (m_loop){m_channel->stop();}
        
        // if the sound is not set to multiplay, then stop the current,
        // before we start another
        if (!m_multiplay){m_channel->stop();}
        
        g_systems[m_device]->playSound(FMOD_CHANNEL_FREE, m_sound, true, &m_channel);
        m_channel->getFrequency(&m_internal_freq);
        m_channel->setVolume(m_volume);
        set_pan(m_pan);
        
        m_channel->setFrequency(m_internal_freq * m_speed);
        m_channel->setMode(m_loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
        m_channel->setSpeakerMix(m_speaker_mix[0], m_speaker_mix[1], m_speaker_mix[2],
                                 m_speaker_mix[3], m_speaker_mix[4], m_speaker_mix[5],
                                 m_speaker_mix[6], m_speaker_mix[7]);
        m_channel->setPaused(m_paused);
        
        //fmod update() should be called every frame - according to the docs.
        //we have been using fmod without calling it at all which resulted in channels not being able
        //to be reused.  we should have some sort of global update function but putting it here
        //solves the channel bug
        g_systems[m_device]->update();
    }
    
    void Fmod_Sound::stop()
    {
        m_channel->stop();
    }
    
    void Fmod_Sound::record(float num_secs, int device)
    {
        FMOD_RESULT result;
        
        // init system
        init_fmod();
        unload();
        
        // list drivers
        //auto devices = get_recording_devices();

        // set driver
        int recorddriver = device;
        
        // create sound
        FMOD_CREATESOUNDEXINFO exinfo;
        memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
        
        exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
        exinfo.numchannels = 2;
        exinfo.defaultfrequency = 44100;
        
        exinfo.format = FMOD_SOUND_FORMAT_PCM16;
        exinfo.length = exinfo.defaultfrequency * sizeof(short) * exinfo.numchannels * num_secs;
        
        result = g_systems[m_device]->createSound(NULL, FMOD_2D | FMOD_SOFTWARE | FMOD_OPENUSER, &exinfo, &m_sound);
        CHECK_ERROR(result);

        // start recording
        result = g_systems[m_device]->recordStart(recorddriver, m_sound, m_loop);
        CHECK_ERROR(result);
        
        // just in case
        g_systems[m_device]->update();
    }
    
    bool Fmod_Sound::is_recording(int device)
    {
        init_fmod();
        bool ret;
        g_systems[m_device]->isRecording(0, &ret);
        return ret;
    }
    
    void Fmod_Sound::get_spectrum(std::vector<float> &buffer, int num_buckets)
    {
        // 	check what the user wants vs. what we can do:
        if (num_buckets > g_max_num_fft_bands)
        {
            LOG_ERROR << "error in audio::get_spectrum, the maximum number of bands is 8192";
            num_buckets = g_max_num_fft_bands;
        }
        else if (num_buckets <= 0)
        {
            LOG_ERROR << "error in audio::get_spectrum, minimum number of bands is 1";
            num_buckets = 1;
        }
        
        // 	FMOD needs pow2
        int nBandsToGet = std::max(nextPow2(num_buckets), 64);
        
        //TODO: only left channel returned here
        FMOD_RESULT result = m_channel->getSpectrum(&g_fftSpectrum[0], nBandsToGet, 0,
                                                    FMOD_DSP_FFT_WINDOW_HANNING);
        CHECK_ERROR(result);
        
        // interpolate and write values to output buffer
        rebase_buffer(g_fftSpectrum, buffer, num_buckets);
    }
    
    void Fmod_Sound::get_pcm_buffer(float *buffer, int num_samples)
    {
    
    }
    
    void Fmod_Sound::set_volume(float vol)
    {
        if (playing()){m_channel->setVolume(vol);}
        m_volume = vol;
    }
    
    void Fmod_Sound::set_pan(float pan)
    {
        m_pan = kinski::clamp(pan, -1.f, 1.f);
        if (playing()){m_channel->setPan(m_pan);}
    }
    
    void Fmod_Sound::set_speaker_mix(float frontleft, float frontright, float center, float lfe,
                                     float backleft, float backright, float sideleft, float sideright)
    {
        m_speaker_mix = {frontleft, frontright, center, lfe, backleft, backright, sideleft,
            sideright};
        if (playing())
        {
            m_channel->setSpeakerMix(frontleft, frontright, center, lfe, backleft, backright, sideleft,
                                     sideright);
        }
    }
    
    void Fmod_Sound::get_speaker_mix(float *frontleft, float *frontright, float *center, float *lfe,
                                     float *backleft, float *backright, float *sideleft, float *sideright)
    {
        *frontleft = m_speaker_mix[0];
        *frontright = m_speaker_mix[1];
        *center = m_speaker_mix[2];
        *lfe = m_speaker_mix[3];
        *backleft = m_speaker_mix[4];
        *backright = m_speaker_mix[5];
        *sideleft = m_speaker_mix[6];
        *sideright = m_speaker_mix[7];
    }
    
    void Fmod_Sound::set_speed(float speed)
    {
        if (playing()){m_channel->setFrequency(m_internal_freq * speed);}
        m_speed = speed;
    }
    
    void Fmod_Sound::set_paused(bool b)
    {
        if (playing())
        {
            m_channel->setPaused(b);
            m_paused = b;
        }
    }
    
    void Fmod_Sound::set_loop(bool b)
    {
        if (playing()){m_channel->setMode(b ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);}
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
            m_channel->setPosition(sample, FMOD_TIMEUNIT_PCM);
        }
    }
    
    void Fmod_Sound::set_positionMS(int ms)
    {
        if (playing()){m_channel->setPosition(ms, FMOD_TIMEUNIT_MS);}
    }
    
    bool Fmod_Sound::playing()
    {
        if (!loaded()) return false;
        
        bool playing = 0;
        m_channel->isPlaying(&playing);
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
            m_channel->getPosition(&sample, FMOD_TIMEUNIT_PCM);
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
            m_channel->getPosition(&sample, FMOD_TIMEUNIT_MS);
            return sample;
        }
        return 0;
    }
}}
