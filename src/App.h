#ifndef KINSKI_APP_H
#define KINSKI_APP_H

#include "KinskiGL.h"

namespace app{
    
    typedef boost::shared_ptr<class App> AppPtr;
    
    class App : public boost::enable_shared_from_this<App>
    {
    public:
        App(const int width = 640, const int height = 480);
        virtual ~App();
        
        void updateView(float offsetX, float offsetY, float offsetZ, 
                        float angleX, float angleY);
        
        static void resize(int w, int h);
        int run();
        
        virtual void init();
        virtual void draw();
        virtual void update(const float delta);
        
    private:
        
        //static AppPtr s_instance;
        
        GLint m_running;
        
        int m_width, m_height;
        
        TwBar *m_tweakBar;
        bool m_displayTweakBar;
    };
    
}
#endif // KINSKI_APP_H