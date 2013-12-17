//
//  LeapConnector.h
//  kinskiGL
//
//  Created by Fabian on 7/30/13.
//
//

#ifndef __kinskiGL__LeapConnector__
#define __kinskiGL__LeapConnector__

#include "kinskiGL/KinskiGL.h"
#include "boost/thread.hpp"
#include "Leap.h"


namespace kinski{ namespace leap{
    
    inline glm::vec3 type_cast(const Leap::Vector &v){ return glm::vec3(v.x, v.y, v.z); }
    
    struct hand_struct
    {
        glm::vec3 position;
        float sphere_size;
    };
    
    struct pointable_struct
    {
        glm::vec3 position;
    };
    
    class LeapConnector : public Leap::Listener
    {
    public:
        typedef std::list<hand_struct> HandList;
        typedef std::list<pointable_struct> PointableList;
        
        LeapConnector();
        
        void onFrame(const Leap::Controller &controller);
        
        Leap::Frame lastFrame() const;
        const HandList& hands() const;
        const PointableList& pointables() const;
        
        void start();
        void stop();
        
    private:
        
        std::shared_ptr<Leap::Controller> m_controller;
        mutable boost::mutex m_mutex;
        Leap::Frame m_last_frame;
        HandList m_hand_list;
        PointableList m_pointables;
    };
}}// namespaces

#endif /* defined(__kinskiGL__LeapConnector__) */
