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

#include "gl/gl.h"
#include "core/Component.h"
#include "core/file_functions.h"
#include "core/ThreadPool.h"

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
        typedef std::shared_ptr<Window> Ptr;
        typedef std::function<void()> DrawFunction;
        
        virtual void draw() const = 0;
        virtual gl::vec2 framebuffer_size() const = 0;
        
        virtual gl::vec2 size() const = 0;
        virtual gl::vec2 position() const = 0;
        virtual void set_position(const gl::vec2 &the_pos) = 0;
        virtual std::string title(const std::string &the_name) const = 0;
        virtual void set_title(const std::string &the_name) = 0;
        
        void set_draw_function(DrawFunction the_draw_function){ m_draw_function = the_draw_function; }

    protected:
        DrawFunction m_draw_function;
    };
        
    class KINSKI_API App : public Component
    {
    public:
        typedef std::shared_ptr<App> Ptr;
        typedef std::weak_ptr<App> WeakPtr;
        
        App(const int width = 1280, const int height = 720, const std::string &the_name = "KinskiGL");
        virtual ~App();
        
        int run();
        
        // you are supposed to implement these in a subclass
        virtual void setup() = 0;
        virtual void update(float timeDelta) = 0;
        virtual void draw() = 0;
        virtual void tearDown() = 0;

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
        inline void set_window_size(uint32_t w, uint32_t h){set_window_size(glm::vec2(w, h));};
        inline float getAspectRatio(){return fabsf(m_windowSize[0]/(float)m_windowSize[1]);};
        inline const glm::vec2 windowSize(){return m_windowSize;};
        
        inline float max_fps() const {return m_max_fps;};
        inline void set_max_fps(float fps){m_max_fps = fps;};
        
        virtual bool fullscreen() const {return m_fullscreen;};
        virtual void set_fullscreen(bool b, int monitor_index){ m_fullscreen = b; };
        void set_fullscreen(bool b = true){ set_fullscreen(b, 0); };
        
        virtual bool cursorVisible() const { return m_cursorVisible;};
        virtual void setCursorVisible(bool b = true){ m_cursorVisible = b;};
        
        
        virtual double getApplicationTime() = 0;
        
        /*!
         * return current frames per second
         */
        float fps() const {return m_framesPerSec;};
        
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
        bool m_displayTweakBar;
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
    
    enum Key
    {
        /* The unknown key */
        KEY_UNKNOWN = -1,
        
        /* Printable keys */
        KEY_SPACE = 32,
        KEY_APOSTROPHE = 39,  /* ' */
        KEY_COMMA = 44,  /* , */
        KEY_MINUS = 45,  /* - */
        KEY_PERIOD = 46,  /* . */
        KEY_SLASH = 47,  /* / */
        KEY_0 = 48,
        KEY_1 = 49,
        KEY_2 = 50,
        KEY_3 = 51,
        KEY_4 = 52,
        KEY_5 = 53,
        KEY_6 = 54,
        KEY_7 = 55,
        KEY_8 = 56,
        KEY_9 = 57,
        KEY_SEMICOLON = 59,  /* ; */
        KEY_EQUAL = 61,  /* = */
        KEY_A = 65,
        KEY_B = 66,
        KEY_C = 67,
        KEY_D = 68,
        KEY_E = 69,
        KEY_F = 70,
        KEY_G = 71,
        KEY_H = 72,
        KEY_I = 73,
        KEY_J = 74,
        KEY_K = 75,
        KEY_L = 76,
        KEY_M = 77,
        KEY_N = 78,
        KEY_O = 79,
        KEY_P = 80,
        KEY_Q = 81,
        KEY_R = 82,
        KEY_S = 83,
        KEY_T = 84,
        KEY_U = 85,
        KEY_V = 86,
        KEY_W = 87,
        KEY_X = 88,
        KEY_Y = 89,
        KEY_Z = 90,
        KEY_LEFT_BRACKET = 91,  /* [ */
        KEY_BACKSLASH = 92,  /* \ */
        KEY_RIGHT_BRACKET = 93,  /* ] */
        KEY_GRAVE_ACCENT = 96,  /* ` */
        KEY_WORLD_1 = 161, /* non-US #1 */
        KEY_WORLD_2 = 162, /* non-US #2 */
        
        /* Function keys */
        KEY_ESCAPE = 256,
        KEY_ENTER = 257,
        KEY_TAB = 258,
        KEY_BACKSPACE = 259,
        KEY_INSERT = 260,
        KEY_DELETE = 261,
        KEY_RIGHT = 262,
        KEY_LEFT = 263,
        KEY_DOWN = 264,
        KEY_UP = 265,
        KEY_PAGE_UP = 266,
        KEY_PAGE_DOWN = 267,
        KEY_HOME = 268,
        KEY_END = 269,
        KEY_CAPS_LOCK = 280,
        KEY_SCROLL_LOCK = 281,
        KEY_NUM_LOCK = 282,
        KEY_PRINT_SCREEN = 283,
        KEY_PAUSE = 284,
        KEY_F1 = 290,
        KEY_F2 = 291,
        KEY_F3 = 292,
        KEY_F4 = 293,
        KEY_F5 = 294,
        KEY_F6 = 295,
        KEY_F7 = 296,
        KEY_F8 = 297,
        KEY_F9 = 298,
        KEY_F10 = 299,
        KEY_F11 = 300,
        KEY_F12 = 301,
        KEY_F13 = 302,
        KEY_F14 = 303,
        KEY_F15 = 304,
        KEY_F16 = 305,
        KEY_F17 = 306,
        KEY_F18 = 307,
        KEY_F19 = 308,
        KEY_F20 = 309,
        KEY_F21 = 310,
        KEY_F22 = 311,
        KEY_F23 = 312,
        KEY_F24 = 313,
        KEY_F25 = 314,
        KEY_KP_0 = 320,
        KEY_KP_1 = 321,
        KEY_KP_2 = 322,
        KEY_KP_3 = 323,
        KEY_KP_4 = 324,
        KEY_KP_5 = 325,
        KEY_KP_6 = 326,
        KEY_KP_7 = 327,
        KEY_KP_8 = 328,
        KEY_KP_9 = 329,
        KEY_KP_DECIMAL = 330,
        KEY_KP_DIVIDE = 331,
        KEY_KP_MULTIPLY = 332,
        KEY_KP_SUBTRACT = 333,
        KEY_KP_ADD = 334,
        KEY_KP_ENTER = 335,
        KEY_KP_EQUAL = 336,
        KEY_LEFT_SHIFT = 340,
        KEY_LEFT_CONTROL = 341,
        KEY_LEFT_ALT = 342,
        KEY_LEFT_SUPER = 343,
        KEY_RIGHT_SHIFT = 344,
        KEY_RIGHT_CONTROL = 345,
        KEY_RIGHT_ALT = 346,
        KEY_RIGHT_SUPER = 347,
        KEY_MENU = 348,
        KEY_LAST = KEY_MENU
    };
}
#endif // _KINSKI_APP_IS_INCLUDED_
