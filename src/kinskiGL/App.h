#ifndef KINSKI_APP_H
#define KINSKI_APP_H

#include "kinskiGL/KinskiGL.h"

namespace kinski{
    
    class App
    {
    public:
        App(const int width = 640, const int height = 480);
        virtual ~App();
        
        static void resize(int w, int h);
        int run();
        
        // you are supposed to implement these
        virtual void setup() = 0;
        virtual void draw() = 0;
        virtual void update(const float delta) = 0;
        
        void setTweakBarVisible(bool b){m_displayTweakBar = b;};
        bool isTweaBarVisible(){return m_displayTweakBar;};
        TwBar* getTweakBar(){return m_tweakBar;};
        
    private:

        GLint m_running;
        
        int m_width, m_height;
        bool m_fullscreen;
        
        TwBar *m_tweakBar;
        bool m_displayTweakBar;
        float m_testFloat;
    };    
}
#endif // KINSKI_APP_H