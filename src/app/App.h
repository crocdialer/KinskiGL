// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef _KINSKI_APP_IS_INCLUDED_
#define _KINSKI_APP_IS_INCLUDED_

#include "gl/KinskiGL.h"
#include "core/Component.h"
#include "core/Logger.h"
#include "core/file_functions.h"

#include <boost/asio.hpp>
#include "core/networking.h"
#include "core/ThreadPool.h"

namespace kinski
{
    class MouseEvent;
    class KeyEvent;
    class JoystickState;
    
    class KINSKI_API App : public Component
    {
    public:
        typedef std::shared_ptr<App> Ptr;
        typedef std::weak_ptr<App> WeakPtr;
        
        App(const int width = 800, const int height = 600);
        virtual ~App();
        
        int run();
        
        // you are supposed to implement these in a subclass
        virtual void setup() = 0;
        virtual void update(float timeDelta) = 0;
        virtual void draw() = 0;
        virtual void tearDown(){};
        virtual void create_tweakbar_from_component(const Component::Ptr &the_component){};
        
        // these are optional overrides
        virtual void setWindowSize(const glm::vec2 &size);
        virtual void resize(int w, int h){};
        virtual void mousePress(const MouseEvent &e){};
        virtual void mouseRelease(const MouseEvent &e){};
        virtual void mouseMove(const MouseEvent &e){};
        virtual void mouseDrag(const MouseEvent &e){};
        virtual void mouseWheel(const MouseEvent &e){};
        virtual void keyPress(const KeyEvent &e){};
        virtual void keyRelease(const KeyEvent &e){};
        virtual void fileDrop(const MouseEvent &e, const std::vector<std::string> &files){};
        virtual void got_message(const std::vector<uint8_t> &the_data){};
        virtual void setCursorPosition(float x, float y){};
        virtual std::vector<JoystickState> get_joystick_states() const {return {};};

        bool running() const {return m_running;};
        void set_running(bool b){m_running = b;}
        
        inline float getWidth(){return m_windowSize[0];};
        inline float getHeight(){return m_windowSize[1];};
        inline void setWindowSize(uint32_t w, uint32_t h){setWindowSize(glm::vec2(w, h));};
        inline float getAspectRatio(){return fabsf(m_windowSize[0]/(float)m_windowSize[1]);};
        inline const glm::vec2 windowSize(){return m_windowSize;};
        
        inline float max_fps() const {return m_max_fps;};
        inline void set_max_fps(float fps){m_max_fps = fps;};
        
        virtual bool fullSceen() const {return m_fullscreen;};
        virtual void setFullSceen(bool b = true){m_fullscreen = b;};
        
        virtual bool cursorVisible() const { return m_cursorVisible;};
        virtual void setCursorVisible(bool b = true){ m_cursorVisible = b;};
        
        
        virtual double getApplicationTime() = 0;
        
        float framesPerSec() const {return m_framesPerSec;};
        
        boost::asio::io_service& io_service(){return m_thread_pool.io_service();};
        ThreadPool& thread_pool(){return m_thread_pool;}
        const ThreadPool& thread_pool() const {return m_thread_pool;}
        
    private:
        
        virtual void init() = 0;
        virtual void pollEvents() = 0;
        virtual void swapBuffers() = 0;
        
        void timing(double timeStamp);
        virtual void draw_internal();
        virtual bool checkRunning(){return m_running;};
        
        uint32_t m_framesDrawn;
        double m_lastTimeStamp;
        double m_lastMeasurementTimeStamp;
        float m_framesPerSec;
        double m_timingInterval;
        
        glm::vec2 m_windowSize;
        bool m_running;
        bool m_fullscreen;
        bool m_cursorVisible;
        float m_max_fps;
        
        kinski::ThreadPool m_thread_pool;
    };
    
    //! Base class for all Events
    class Event {
    protected:
        Event() {}
        
    public:
        virtual ~Event() {}
    };
    
    //! Represents a mouse event
    class KINSKI_API MouseEvent : public Event
    {
    public:
        MouseEvent() : Event() {}
        MouseEvent( int aInitiator, int aX, int aY, unsigned int aModifiers, glm::ivec2 aWheelIncrement )
		: Event(), mInitiator( aInitiator ), mX( aX ), mY( aY ), mModifiers( aModifiers ), mWheelIncrement( aWheelIncrement )
        {}
        
        //! Returns the X coordinate of the mouse event
        int			getX() const { return mX; }
        //! Returns the Y coordinate of the mouse event
        int			getY() const { return mY; }
        //! Returns the coordinates of the mouse event
        glm::ivec2	getPos() const { return glm::ivec2( mX, mY ); }
        //! Returns whether the initiator for the event was the left mouse button
        bool		isLeft() const { return ( mInitiator & LEFT_DOWN ) ? true : false; }
        //! Returns whether the initiator for the event was the right mouse button
        bool		isRight() const { return ( mInitiator & RIGHT_DOWN ) ? true : false; }
        //! Returns whether the initiator for the event was the middle mouse button
        bool		isMiddle() const { return ( mInitiator & MIDDLE_DOWN ) ? true : false; }
        //! Returns whether the left mouse button was pressed during the event
        bool		isLeftDown() const { return (mModifiers & LEFT_DOWN) ? true : false; }
        //! Returns whether the right mouse button was pressed during the event
        bool		isRightDown() const { return (mModifiers & RIGHT_DOWN) ? true : false; }
        //! Returns whether the middle mouse button was pressed during the event
        bool		isMiddleDown() const { return (mModifiers & MIDDLE_DOWN) ? true : false; }
        //! Returns whether the Shift key was pressed during the event.
        bool		isShiftDown() const { return (mModifiers & SHIFT_DOWN) ? true : false; }
        //! Returns whether the Alt (or Option) key was pressed during the event.
        bool		isAltDown() const { return (mModifiers & ALT_DOWN) ? true : false; }
        //! Returns whether the Control key was pressed during the event.
        bool		isControlDown() const { return (mModifiers & CTRL_DOWN) ? true : false; }
        //! Returns whether the meta key was pressed during the event. Maps to the Windows key on Windows and the Command key on Mac OS X.
        bool		isMetaDown() const { return (mModifiers & META_DOWN) ? true : false; }
        //! Returns whether the accelerator key was pressed during the event. Maps to the Control key on Windows and the Command key on Mac OS X.
        bool		isAccelDown() const { return (mModifiers & ACCEL_DOWN) ? true : false; }
        //! Returns the number of detents the user has wheeled through. Positive values correspond to wheel-up and negative to wheel-down.
        glm::ivec2 getWheelIncrement() const { return mWheelIncrement; }
        
        enum {	LEFT_DOWN	= 0x0001,
                RIGHT_DOWN	= 0x0002,
                MIDDLE_DOWN = 0x0004,
                SHIFT_DOWN	= 0x0008,
                ALT_DOWN	= 0x0010,
                CTRL_DOWN	= 0x0020,
                META_DOWN	= 0x0040,
#if defined( KINSKI_MSW )
			ACCEL_DOWN	= CTRL_DOWN
#else
			ACCEL_DOWN	= META_DOWN
#endif
        };
        
    private:
        int				mInitiator;
        int				mX, mY;
        unsigned int	mModifiers;
        glm::ivec2		mWheelIncrement;
    };
    
    //! Represents a keyboard event
    class KINSKI_API KeyEvent : public Event
    {
    public:
        KeyEvent( int aCode, uint8_t aChar, unsigned int aModifiers)
		: Event(), mCode( aCode ), mChar( aChar ), mModifiers( aModifiers ){}
        
        //! Returns the key code associated with the event, which maps into the enum listed below
        int		getCode() const { return mCode; }
        //! Returns the ASCII character associated with the event.
        uint8_t	getChar() const { return mChar; }
        //! Returns whether the Shift key was pressed during the event.
        bool	isShiftDown() const { return (mModifiers & SHIFT_DOWN) ? true : false; }
        //! Returns whether the Alt (or Option) key was pressed during the event.
        bool	isAltDown() const { return (mModifiers & ALT_DOWN) ? true : false; }
        //! Returns whether the Control key was pressed during the event.
        bool	isControlDown() const { return (mModifiers & CTRL_DOWN) ? true : false; }
        //! Returns whether the meta key was pressed during the event. Maps to the Windows key on Windows and the Command key on Mac OS X.
        bool	isMetaDown() const { return (mModifiers & META_DOWN) ? true : false; }
        //! Returns whether the accelerator key was pressed during the event. Maps to the Control key on Windows and the Command key on Mac OS X.
        bool	isAccelDown() const { return (mModifiers & ACCEL_DOWN) ? true : false; }
        
        //! Maps a platform-native key-code to the key code enum
        static int		translateNativeKeyCode( int nativeKeyCode );
        
        enum {	SHIFT_DOWN	= 0x0008,
			ALT_DOWN	= 0x0010,
			CTRL_DOWN	= 0x0020,
			META_DOWN	= 0x0040,
#if defined( CINDER_MSW )
			ACCEL_DOWN	= CTRL_DOWN
#else
			ACCEL_DOWN	= META_DOWN
#endif
        };
        
    protected:
        int				mCode;
        char			mChar;
        unsigned int	mModifiers;
    };
    
    class JoystickState
    {
    public:
        JoystickState(const std::string &n,
                      const std::vector<uint8_t>& b,
                      const std::vector<float>& a):
        m_name(n),
        m_buttons(b),
        m_axis(a){}
        
        const std::string& name() const { return m_name; };
        const std::vector<uint8_t>& buttons() const { return m_buttons; };
        const std::vector<float>& axis() const { return m_axis; };
        
    private:
        std::string m_name;
        std::vector<uint8_t> m_buttons;
        std::vector<float> m_axis;
    };
}
#endif // _KINSKI_APP_IS_INCLUDED_
