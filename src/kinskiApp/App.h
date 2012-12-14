#ifndef _KINSKI_APP_IS_INCLUDED_
#define _KINSKI_APP_IS_INCLUDED_

#include "kinskiGL/KinskiGL.h"
#include "kinskiCore/Component.h"

#define GLFW_INCLUDE_GL3
#define GLFW_NO_GLU
#include <GL/glfw.h>
#include <AntTweakBar.h>

namespace kinski
{
    class MouseEvent;
    class KeyEvent;
    
    class App : public Component
    {
    public:
        typedef std::shared_ptr<App> Ptr;
        typedef std::weak_ptr<App> WeakPtr;
        
        static Ptr getInstance();
        
        App(const int width = 800, const int height = 600);
        virtual ~App();
        
        int run();
        
        // you are supposed to implement these in a subclass
        virtual void setup() = 0;
        virtual void update(const float timeDelta) = 0;
        virtual void draw() = 0;
        
        // these are optional overrides
        virtual void resize(int w, int h){};
        virtual void mousePress(const MouseEvent &e){};
        virtual void mouseRelease(const MouseEvent &e){};
        virtual void mouseMove(const MouseEvent &e){};
        virtual void mouseDrag(const MouseEvent &e){};
        virtual void mouseWheel(const MouseEvent &e){};
        virtual void keyPress(const KeyEvent &e);
        virtual void keyRelease(const KeyEvent &e){};
        
        virtual void tearDown(){};
        
        inline float getWidth(){return m_windowSize[0];};
        inline float getHeight(){return m_windowSize[1];};
        inline float getAspectRatio(){return fabsf(m_windowSize[0]/(float)m_windowSize[1]);};
        inline const glm::vec2 getWindowSize(){return m_windowSize;};
        
        inline void setWindowSize(uint32_t w, uint32_t h){setWindowSize(glm::ivec2(w, h));};
        void setWindowSize(const glm::ivec2 size);
        
        inline bool fullSceen() const {return m_fullscreen;};
        inline void setFullSceen(bool b = true){m_fullscreen = b;};
        
        bool cursorVisible() const { return m_cursorVisible;};
        void setCursorVisible(bool b = true){ m_cursorVisible = b;};
        
        double getApplicationTime();
        
        void setDisplayTweakBar(bool b){m_displayTweakBar = b;};
        bool displayTweakBar() const {return m_displayTweakBar;};
        
        void addPropertyToTweakBar(const Property::Ptr propPtr,
                                   const std::string &group = "",
                                   TwBar *theBar = NULL);
        
        void addPropertyListToTweakBar(const std::list<Property::Ptr> &theProps,
                                       const std::string &group = "",
                                       TwBar *theBar = NULL);
        
        void setBarPosition(const glm::ivec2 &thePos, TwBar *theBar = NULL);
        void setBarSize(const glm::ivec2 &theSize, TwBar *theBar = NULL);
        void setBarColor(const glm::vec4 &theColor, TwBar *theBar = NULL);
        void setBarTitle(const std::string &theTitle, TwBar *theBar = NULL);
        
        const std::map<TwBar*, Property::Ptr>& 
        getTweakProperties() const {return m_tweakProperties;};
        
    private:
        
        static WeakPtr s_instance;
        
        // GLFW static callbacks
        static void s_resize(int w, int h){getInstance()->__resize(w, h);};
        static void s_mouseMove(int x, int y){getInstance()->__mouseMove(x, y);};
        static void s_mouseButton(int button, int action){getInstance()->__mouseButton(button, action);};
        static void s_mouseWheel(int pos){getInstance()->__mouseWheel(pos);};
        
        static void s_keyFunc(int key, int action){getInstance()->__keyFunc(key, action);};
        static void s_charFunc(int key, int action){getInstance()->__charFunc(key, action);};
        
        void __resize(int w,int h);
        void __mouseMove(int x,int y);
        void __mouseButton(int button, int action);
        void __mouseWheel(int pos);
        
        void __keyFunc(int key, int action);
        void __charFunc(int key, int action);
        
        // return the current key and mouse button modifier mask
        void getModifiers(uint32_t &buttonModifiers, uint32_t &keyModifiers);
        
        // internal initialization. performed when run is invoked
        void init();
        
        GLint m_running;
        double m_lastTimeStamp;
        uint64_t m_framesDrawn;

        std::list<TwBar*> m_tweakBarList;
        bool m_displayTweakBar;
        
        std::map<TwBar*, Property::Ptr> m_tweakProperties;
        
        // might go into a settings struct
        glm::vec2 m_windowSize;
        
        bool m_fullscreen;
        
        bool m_cursorVisible;
        
    };
    
    //! Base class for all Events
    class Event {
    protected:
        Event() {}
        
    public:
        virtual ~Event() {}
    };
    
    //! Represents a mouse event
    class MouseEvent : public Event {
    public:
        MouseEvent() : Event() {}
        MouseEvent( int aInitiator, int aX, int aY, unsigned int aModifiers, int aWheelIncrement )
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
        int		getWheelIncrement() const { return mWheelIncrement; }
        
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
        int			mWheelIncrement;
    };
    
    //! Represents a keyboard event
    class KeyEvent : public Event
    {
    public:
        KeyEvent( int aCode, char aChar, unsigned int aModifiers)
		: Event(), mCode( aCode ), mChar( aChar ), mModifiers( aModifiers ){}
        
        //! Returns the key code associated with the event, which maps into the enum listed below
        int		getCode() const { return mCode; }
        //! Returns the ASCII character associated with the event.
        char	getChar() const { return mChar; }
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
        
        // Key codes
        enum {
            KEY_UNKNOWN		= 0,
            KEY_FIRST		= 0,
            KEY_BACKSPACE	= 8,
            KEY_TAB			= 9,
            KEY_CLEAR		= 12,
            KEY_RETURN		= 13,
            KEY_PAUSE		= 19,
            KEY_ESCAPE		= 27,
            KEY_SPACE		= 32,
            KEY_EXCLAIM		= 33,
            KEY_QUOTEDBL	= 34,
            KEY_HASH		= 35,
            KEY_DOLLAR		= 36,
            KEY_AMPERSAND	= 38,
            KEY_QUOTE		= 39,
            KEY_LEFTPAREN	= 40,
            KEY_RIGHTPAREN	= 41,
            KEY_ASTERISK	= 42,
            KEY_PLUS		= 43,
            KEY_COMMA		= 44,
            KEY_MINUS		= 45,
            KEY_PERIOD		= 46,
            KEY_SLASH		= 47,
            KEY_0			= 48,
            KEY_1			= 49,
            KEY_2			= 50,
            KEY_3			= 51,
            KEY_4			= 52,
            KEY_5			= 53,
            KEY_6			= 54,
            KEY_7			= 55,
            KEY_8			= 56,
            KEY_9			= 57,
            KEY_COLON		= 58,
            KEY_SEMICOLON	= 59,
            KEY_LESS		= 60,
            KEY_EQUALS		= 61,
            KEY_GREATER		= 62,
            KEY_QUESTION	= 63,
            KEY_AT			= 64,
            
            KEY_LEFTBRACKET	= 91,
            KEY_BACKSLASH	= 92,
            KEY_RIGHTBRACKET= 93,
            KEY_CARET		= 94,
            KEY_UNDERSCORE	= 95,
            KEY_BACKQUOTE	= 96,
            KEY_a			= 97,
            KEY_b			= 98,
            KEY_c			= 99,
            KEY_d			= 100,
            KEY_e			= 101,
            KEY_f			= 102,
            KEY_g			= 103,
            KEY_h			= 104,
            KEY_i			= 105,
            KEY_j			= 106,
            KEY_k			= 107,
            KEY_l			= 108,
            KEY_m			= 109,
            KEY_n			= 110,
            KEY_o			= 111,
            KEY_p			= 112,
            KEY_q			= 113,
            KEY_r			= 114,
            KEY_s			= 115,
            KEY_t			= 116,
            KEY_u			= 117,
            KEY_v			= 118,
            KEY_w			= 119,
            KEY_x			= 120,
            KEY_y			= 121,
            KEY_z			= 122,
            KEY_DELETE		= 127,
            
            KEY_KP0			= 256,
            KEY_KP1			= 257,
            KEY_KP2			= 258,
            KEY_KP3			= 259,
            KEY_KP4			= 260,
            KEY_KP5			= 261,
            KEY_KP6			= 262,
            KEY_KP7			= 263,
            KEY_KP8			= 264,
            KEY_KP9			= 265,
            KEY_KP_PERIOD	= 266,
            KEY_KP_DIVIDE	= 267,
            KEY_KP_MULTIPLY	= 268,
            KEY_KP_MINUS	= 269,
            KEY_KP_PLUS		= 270,
            KEY_KP_ENTER	= 271,
            KEY_KP_EQUALS	= 272,
            
            KEY_UP			= 273,
            KEY_DOWN		= 274,
            KEY_RIGHT		= 275,
            KEY_LEFT		= 276,
            KEY_INSERT		= 277,
            KEY_HOME		= 278,
            KEY_END			= 279,
            KEY_PAGEUP		= 280,
            KEY_PAGEDOWN	= 281,
            
            KEY_F1			= 282,
            KEY_F2			= 283,
            KEY_F3			= 284,
            KEY_F4			= 285,
            KEY_F5			= 286,
            KEY_F6			= 287,
            KEY_F7			= 288,
            KEY_F8			= 289,
            KEY_F9			= 290,
            KEY_F10			= 291,
            KEY_F11			= 292,
            KEY_F12			= 293,
            KEY_F13			= 294,
            KEY_F14			= 295,
            KEY_F15			= 296,
            
            KEY_NUMLOCK		= 300,
            KEY_CAPSLOCK	= 301,
            KEY_SCROLLOCK	= 302,
            KEY_RSHIFT		= 303,
            KEY_LSHIFT		= 304,
            KEY_RCTRL		= 305,
            KEY_LCTRL		= 306,
            KEY_RALT		= 307,
            KEY_LALT		= 308,
            KEY_RMETA		= 309,
            KEY_LMETA		= 310,
            KEY_LSUPER		= 311,		/* Left "Windows" key */
            KEY_RSUPER		= 312,		/* Right "Windows" key */
            KEY_MODE		= 313,		/* "Alt Gr" key */
            KEY_COMPOSE		= 314,		/* Multi-key compose key */
            
            KEY_HELP		= 315,
            KEY_PRINT		= 316,
            KEY_SYSREQ		= 317,
            KEY_BREAK		= 318,
            KEY_MENU		= 319,
            KEY_POWER		= 320,		/* Power Macintosh power key */
            KEY_EURO		= 321,		/* Some european keyboards */
            KEY_UNDO		= 322,		/* Atari keyboard has Undo */
            
            KEY_LAST
        };
        
    protected:
        int				mCode;
        char			mChar;
        unsigned int	mModifiers;
    };
}
#endif // _KINSKI_APP_IS_INCLUDED_