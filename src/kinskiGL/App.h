#ifndef KINSKI_APP_H
#define KINSKI_APP_H

#include "kinskiGL/KinskiGL.h"
#include "AntTweakBarConnector.h"
#include <list>

namespace kinski
{
    
    class App : public boost::enable_shared_from_this<App>
    {
    public:
        typedef boost::shared_ptr<App> Ptr;
        typedef boost::weak_ptr<App> WeakPtr;
        
        static Ptr getInstance();
        
        App(const int width = 800, const int height = 600);
        virtual ~App();
        
        void resize(const int w, const int h);
        int run();
        
        // you are supposed to implement these in a subclass
        virtual void setup() = 0;
        virtual void update(const float timeDelta) = 0;
        virtual void draw() = 0;
        
        virtual void tearDown(){};
        
        inline float getWidth(){return m_windowSize[0];};
        inline float getHeight(){return m_windowSize[1];};
        inline float getAspectRatio(){return fabsf(m_windowSize[0]/(float)m_windowSize[1]);};
        inline const glm::ivec2 getSize(){return m_windowSize;};
        
        inline void setSize(uint32_t w, uint32_t h){setSize(glm::ivec2(w, h));};
        inline void setSize(const glm::ivec2 size){m_windowSize = size;};
        
        inline bool isFullSceen(){return m_fullscreen;};
        inline void setFullSceen(bool b){m_fullscreen = b;};
        
        double getApplicationTime();
        
        void setTweakBarVisible(bool b){m_displayTweakBar = b;};
        bool isTweaBarVisible(){return m_displayTweakBar;};
        TwBar* getTweakBar(){return m_tweakBar;};
        
    protected:   
        
        std::list<Property::Ptr> m_tweakProperties;
        
        template<typename T>
        void addValueToTweakBar(const std::string &theLabel, const T v)
        {
            Property::Ptr propertyPtr(new Property(theLabel, v));
            m_tweakProperties.push_back(propertyPtr);
            
            AntTweakBarConnector::connect(m_tweakBar, propertyPtr, theLabel);
        }
        
    private:
        
        static WeakPtr s_instance;
        
        // GLFW static callbacks
        static void s_resize(int w, int h){getInstance()->resize(w, h);};
        
        // internal initialization. performed when run is invoked
        void init();
        
        GLint m_running;
        double m_lastTimeStamp;
        uint64_t m_framesDrawn;
        
        //int m_width, m_height;
        glm::ivec2 m_windowSize;
        
        bool m_fullscreen;
        
        TwBar *m_tweakBar;
        bool m_displayTweakBar;
        float m_testFloat;
        
    };    
}
#endif // KINSKI_APP_H