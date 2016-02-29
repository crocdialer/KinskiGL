// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "gl/gl.hpp"
#include "core/Component.hpp"
#include "core/ThreadPool.hpp"

namespace kinski
{
    class Window;
    typedef std::shared_ptr<Window> WindowPtr;
    class MouseEvent;
    class KeyEvent;
    class JoystickState;

    class KINSKI_API Window
    {
    public:
        typedef std::function<void()> DrawFunction;
        typedef std::function<void()> CloseFunction;

        virtual ~Window(){ if(m_close_function) m_close_function(); };

        virtual void draw() const = 0;
        virtual gl::vec2 framebuffer_size() const = 0;

        virtual gl::vec2 size() const = 0;
        virtual void set_size(const gl::vec2 &the_sz) = 0;
        virtual gl::vec2 position() const = 0;
        virtual void set_position(const gl::vec2 &the_pos) = 0;
        virtual std::string title(const std::string &the_name) const = 0;
        virtual void set_title(const std::string &the_name) = 0;

        void set_draw_function(DrawFunction the_draw_function){ m_draw_function = the_draw_function; }
        void set_close_function(CloseFunction the_close_function){ m_close_function = the_close_function; }

    protected:
        DrawFunction m_draw_function;
        CloseFunction m_close_function;
    };

    class KINSKI_API App : public Component
    {
    public:
        typedef std::shared_ptr<App> Ptr;
        typedef std::weak_ptr<App> WeakPtr;

        App(int argc = 0, char *argv[] = nullptr);
        virtual ~App();

        int run();

        // you are supposed to implement these in a subclass
        virtual void setup() = 0;
        virtual void update(float timeDelta) = 0;
        virtual void draw() = 0;
        virtual void tearDown() = 0;
        virtual double getApplicationTime() = 0;

        // these are optional overrides
        virtual void set_window_size(const glm::vec2 &size);
        virtual void set_window_title(const std::string &the_name){};
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
        virtual void add_window(WindowPtr the_window){};
        virtual void add_tweakbar_for_component(const Component::Ptr &the_component){};
        virtual void remove_tweakbar_for_component(const Component::Ptr &the_component){};
        virtual std::vector<JoystickState> get_joystick_states() const {return {};};

        inline bool running() const {return m_running;};
        inline void set_running(bool b){m_running = b;}

        inline void displayTweakBar(bool b) {m_displayTweakBar = b;};
        inline bool displayTweakBar() const {return m_displayTweakBar;};

        inline float getWidth(){return m_windowSize[0];};
        inline float getHeight(){return m_windowSize[1];};
        inline float getAspectRatio(){return fabsf(m_windowSize[0]/(float)m_windowSize[1]);};

        inline float max_fps() const {return m_max_fps;};
        inline void set_max_fps(float fps){m_max_fps = fps;};

        virtual bool fullscreen() const {return m_fullscreen;};
        virtual void set_fullscreen(bool b, int monitor_index){ m_fullscreen = b; };
        void set_fullscreen(bool b = true){ set_fullscreen(b, 0); };
        
        virtual void set_cursor_position(float x, float y){};
        virtual gl::vec2 cursor_position() const { return gl::vec2(); };
        virtual bool cursor_visible() const { return m_cursorVisible;};
        virtual void set_cursor_visible(bool b = true){ m_cursorVisible = b;};

        /*!
         * return current frames per second
         */
        float fps() const {return m_framesPerSec;};

        boost::asio::io_service& io_service(){return m_main_queue.io_service();};
        
        /*!
         * the commandline arguments provided at application start
         */
        const std::vector<std::string>& args() const{ return m_args; };
        
        /*!
         * this queue is being processed the main thread
         */
        ThreadPool& main_queue(){ return m_main_queue; }
        const ThreadPool& main_queue() const { return m_main_queue; }
        
        /*!
         * the background queue is being processed by a background threadpool
         */
        ThreadPool& background_queue(){ return m_background_queue; }
        const ThreadPool& background_queue() const { return m_background_queue; }
        
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
        bool m_displayTweakBar;
        bool m_cursorVisible;
        float m_max_fps;
        
        std::vector<std::string> m_args;
        
        kinski::ThreadPool m_main_queue, m_background_queue;
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

        enum
        {
            SHIFT_DOWN	= 0x0008,
			ALT_DOWN	= 0x0010,
			CTRL_DOWN	= 0x0020,
			META_DOWN	= 0x0040,
#if defined(KINSKI_MSW)
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

    struct Key
    {
        enum Type
        {
            /* The unknown key */
            _UNKNOWN = -1,

            /* Printable keys */
            _SPACE = 32,
            _APOSTROPHE = 39,  /* ' */
            _COMMA = 44,  /* , */
            _MINUS = 45,  /* - */
            _PERIOD = 46,  /* . */
            _SLASH = 47,  /* / */
            _0 = 48,
            _1 = 49,
            _2 = 50,
            _3 = 51,
            _4 = 52,
            _5 = 53,
            _6 = 54,
            _7 = 55,
            _8 = 56,
            _9 = 57,
            _SEMICOLON = 59,  /* ; */
            _EQUAL = 61,  /* = */
            _A = 65,
            _B = 66,
            _C = 67,
            _D = 68,
            _E = 69,
            _F = 70,
            _G = 71,
            _H = 72,
            _I = 73,
            _J = 74,
            _K = 75,
            _L = 76,
            _M = 77,
            _N = 78,
            _O = 79,
            _P = 80,
            _Q = 81,
            _R = 82,
            _S = 83,
            _T = 84,
            _U = 85,
            _V = 86,
            _W = 87,
            _X = 88,
            _Y = 89,
            _Z = 90,
            _LEFT_BRACKET = 91,  /* [ */
            _BACKSLASH = 92,  /* \ */
            _RIGHT_BRACKET = 93,  /* ] */
            _GRAVE_ACCENT = 96,  /* ` */
            _WORLD_1 = 161, /* non-US #1 */
            _WORLD_2 = 162, /* non-US #2 */

            /* Function keys */
            _ESCAPE = 256,
            _ENTER = 257,
            _TAB = 258,
            _BACKSPACE = 259,
            _INSERT = 260,
            _DELETE = 261,
            _RIGHT = 262,
            _LEFT = 263,
            _DOWN = 264,
            _UP = 265,
            _PAGE_UP = 266,
            _PAGE_DOWN = 267,
            _HOME = 268,
            _END = 269,
            _CAPS_LOCK = 280,
            _SCROLL_LOCK = 281,
            _NUM_LOCK = 282,
            _PRINT_SCREEN = 283,
            _PAUSE = 284,
            _F1 = 290,
            _F2 = 291,
            _F3 = 292,
            _F4 = 293,
            _F5 = 294,
            _F6 = 295,
            _F7 = 296,
            _F8 = 297,
            _F9 = 298,
            _F10 = 299,
            _F11 = 300,
            _F12 = 301,
            _F13 = 302,
            _F14 = 303,
            _F15 = 304,
            _F16 = 305,
            _F17 = 306,
            _F18 = 307,
            _F19 = 308,
            _F20 = 309,
            _F21 = 310,
            _F22 = 311,
            _F23 = 312,
            _F24 = 313,
            _F25 = 314,
            _KP_0 = 320,
            _KP_1 = 321,
            _KP_2 = 322,
            _KP_3 = 323,
            _KP_4 = 324,
            _KP_5 = 325,
            _KP_6 = 326,
            _KP_7 = 327,
            _KP_8 = 328,
            _KP_9 = 329,
            _KP_DECIMAL = 330,
            _KP_DIVIDE = 331,
            _KP_MULTIPLY = 332,
            _KP_SUBTRACT = 333,
            _KP_ADD = 334,
            _KP_ENTER = 335,
            _KP_EQUAL = 336,
            _LEFT_SHIFT = 340,
            _LEFT_CONTROL = 341,
            _LEFT_ALT = 342,
            _LEFT_SUPER = 343,
            _RIGHT_SHIFT = 344,
            _RIGHT_CONTROL = 345,
            _RIGHT_ALT = 346,
            _RIGHT_SUPER = 347,
            _MENU = 348,
            _LAST = _MENU
        };
    };
}