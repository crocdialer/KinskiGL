#ifndef _KINSKI_APP_IS_INCLUDED_
#define _KINSKI_APP_IS_INCLUDED_

#include "KinskiGL.h"
#include "kinskiCore/Property.h"

#include <map>
#include <list>

namespace kinski
{
    
    class App : public std::enable_shared_from_this<App>
    {
    public:
        typedef std::shared_ptr<App> Ptr;
        typedef std::weak_ptr<App> WeakPtr;
        
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
        inline const glm::vec2 getWindowSize(){return m_windowSize;};
        
        inline void setWindowSize(uint32_t w, uint32_t h){setWindowSize(glm::ivec2(w, h));};
        inline void setWindowSize(const glm::ivec2 size){m_windowSize = size;};
        
        inline bool isFullSceen(){return m_fullscreen;};
        inline void setFullSceen(bool b){m_fullscreen = b;};
        
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
        static void s_resize(int w, int h){getInstance()->resize(w, h);};
        
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
}
#endif // _KINSKI_APP_IS_INCLUDED_