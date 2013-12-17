//
//  LeapConnector.cpp
//  kinskiGL
//
//  Created by Fabian on 7/30/13.
//
//

#include "LeapConnector.h"
#include "kinskiCore/Logger.h"

using namespace Leap;

namespace kinski { namespace leap {
    
    LeapConnector::LeapConnector()
    {
        
    }
    
    void LeapConnector::start()
    {
        m_controller.reset(new Leap::Controller());
        m_controller->addListener(*this);
        
        // enable gestures should be tracked
        m_controller->enableGesture(Leap::Gesture::TYPE_SWIPE);
    }
    
    void LeapConnector::stop()
    {
        m_controller->removeListener(*this);
        m_controller.reset();
    }
    
    void LeapConnector::onFrame(const Leap::Controller &controller)
    {
        if(!controller.isConnected()) return;
        
        Leap::Frame frame = controller.frame();
        int num_hands = frame.hands().count();
        
        if (num_hands){ LOG_TRACE<< num_hands <<" hands detected"; }
        
        HandList tmp_hand_list;
        PointableList tmp_pointables;
        
        for (const Hand &hand : frame.hands())
        {
            if(!hand.isValid()) continue;
            hand_struct h;
            h.position = type_cast(hand.sphereCenter());
            h.sphere_size = hand.sphereRadius();
            tmp_hand_list.push_back(h);
        }
        
        for(const Pointable &pointable : frame.pointables())
        {
            if(!pointable.isValid()) continue;
            pointable_struct p;
            p.position = type_cast(pointable.tipPosition());
            tmp_pointables.push_back(p);
        }
        
//        for(const auto &gesture : frame.gestures())
//        {
//            LOG_INFO<<"gesture: "<<gesture.toString();
//            if(gesture.type() == Leap::Gesture::TYPE_SWIPE)
//            {
//                const Leap::SwipeGesture &swipe = static_cast<const Leap::SwipeGesture&>(gesture);
//                glm::vec3 swipe_dir = leap::type_cast(swipe.direction());
//                LOG_INFO<<glm::to_string(swipe_dir);
//            }
//        }
        
        boost::mutex::scoped_lock lock(m_mutex);
        m_last_frame = frame;
        m_hand_list = tmp_hand_list;
        m_pointables = tmp_pointables;
    }
    
    Leap::Frame LeapConnector::lastFrame() const
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return m_last_frame;
    }
    
    const LeapConnector::HandList& LeapConnector::hands() const
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return m_hand_list;
    };
    
    const LeapConnector::PointableList& LeapConnector::pointables() const
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return m_pointables;
    }
}}// namespaces
