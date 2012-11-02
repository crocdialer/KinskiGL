#ifndef _KINSKI_APP_IS_INCLUDED_
#define _KINSKI_APP_IS_INCLUDED_

#include "KinskiGL.h"
#include "kinskiCore/Component.h"

#include <map>
#include <list>

namespace kinski
{
    class MouseEvent;
    
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
        
        virtual void tearDown(){};
        
        inline float getWidth(){return m_windowSize[0];};
        inline float getHeight(){return m_windowSize[1];};
        inline float getAspectRatio(){return fabsf(m_windowSize[0]/(float)m_windowSize[1]);};
        inline const glm::vec2 getWindowSize(){return m_windowSize;};
        
        inline void setWindowSize(uint32_t w, uint32_t h){setWindowSize(glm::ivec2(w, h));};
        void setWindowSize(const glm::ivec2 size);
        
        inline bool isFullSceen(){return m_fullscreen;};
        inline void setFullSceen(bool b = true){m_fullscreen = b;};
        
        double getApplicationTime();
        
        void setTweakBarVisible(bool b){m_displayTweakBar = b;};
        bool isTweaBarVisible(){return m_displayTweakBar;};
        //TwBar* getTweakBar(){return m_tweakBar;};
        
        void addPropertyToTweakBar(const Property::Ptr propPtr,
                                   const std::string &group = "",
                                   TwBar *theBar = NULL);
        
        void addPropertyListToTweakBar(const std::list<Property::Ptr> &theProps,
                                       const std::string &group = "",
                                       TwBar *theBar = NULL);
        
        const std::map<TwBar*, Property::Ptr>& 
        getTweakProperties() const {return m_tweakProperties;};
        
    private:
        
        static WeakPtr s_instance;
        
        // GLFW static callbacks
        static void s_resize(int w, int h){getInstance()->__resize(w, h);};
        static void s_mouseMove(int x, int y){getInstance()->__mouseMove(x, y);};
        static void s_mouseButton(int button, int action){getInstance()->__mouseButton(button, action);};
        static void s_mouseWheel(int pos){getInstance()->__mouseWheel(pos);};
        
        void __resize(int w,int h);
        void __mouseMove(int x,int y);
        void __mouseButton(int button, int action);
        void __mouseWheel(int pos);
        
        // internal initialization. performed when run is invoked
        void init();
        
        GLint m_running;
        double m_lastTimeStamp;
        uint64_t m_framesDrawn;
        
        glm::vec2 m_windowSize;
        
        bool m_fullscreen;
        
        std::list<TwBar*> m_tweakBarList;
        bool m_displayTweakBar;
        
        std::map<TwBar*, Property::Ptr> m_tweakProperties;
        
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
}
#endif // _KINSKI_APP_IS_INCLUDED_