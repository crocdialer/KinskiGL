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
    
    protected:
        // forward declared Implementation object
        struct Obj;
        typedef std::shared_ptr<Obj> ObjPtr;
        ObjPtr m_Obj;
        
    public:
        //! Emulates shared_ptr-like behavior
        typedef ObjPtr Sound::*unspecified_bool_type;
        operator unspecified_bool_type() const { return ( m_Obj.get() == 0 ) ? 0 : &Sound::m_Obj; }
        void reset() { m_Obj.reset(); }
    };
    
    class SoundLoadException: public Exception
    {
    public:
        SoundLoadException(const std::string &theFilename) :
        Exception(std::string("Got trouble loading an audiofile: ") + theFilename){}
    };
}}//namespace

#endif /* defined(__kinskiGL__Sound__) */
