#ifndef KINSKI_APP_H
#define KINSKI_APP_H

#include "kinskiGL/KinskiGL.h"

namespace kinski
{
    
    class App : public boost::enable_shared_from_this<App>
    {
    public:
        typedef boost::shared_ptr<App> Ptr;
        typedef boost::weak_ptr<App> WeakPtr;
        
        static Ptr getInstance();
        
        App(const int width = 640, const int height = 480);
        virtual ~App();
        
        void resize(const int w, const int h);
        int run();
        
        // you are supposed to implement these in a subclass
        virtual void setup() = 0;
        virtual void draw() = 0;
        virtual void update(const float timeDelta) = 0;
        
        inline float getWidth(){return m_width;};
        inline float getHeight(){return m_height;};
        
        double getApplicationTime();
        
        void setTweakBarVisible(bool b){m_displayTweakBar = b;};
        bool isTweaBarVisible(){return m_displayTweakBar;};
        TwBar* getTweakBar(){return m_tweakBar;};
        
    private:
        
        static WeakPtr s_instance;
        
        // GLFW static callbacks
        static void s_resize(int w, int h){getInstance()->resize(w, h);};
        
        // internal initialization. performed when run is invoked
        void init();
        
        GLint m_running;
        double m_lastTimeStamp;
        
        int m_width, m_height;
        bool m_fullscreen;
        
        TwBar *m_tweakBar;
        bool m_displayTweakBar;
        float m_testFloat;
        
    };    
}
#endif // KINSKI_APP_H