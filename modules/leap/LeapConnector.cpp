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

void hand_found(const Leap::Hand &h)
{
    LOG_DEBUG<<"hand found: "<<h.id();
}

void hand_lost(int the_id)
{
    LOG_DEBUG<<"hand lost: "<<the_id;
}

void hand_updated(const Leap::Hand &h)
{
    LOG_DEBUG<<"hand updated: "<<h.id();
}

namespace kinski { namespace leap {
    
    LeapConnector::LeapConnector()
    {
        m_hand_found.connect(hand_found);
        m_hand_updated.connect(hand_updated);
        m_hand_lost.connect(hand_lost);
    }
    
    void LeapConnector::start()
    {
        m_controller.reset(new Leap::Controller());
        m_controller->addListener(*this);
        
        // enable gestures to be tracked
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
        
        Leap::Frame old_frame = m_frame;
        m_frame = controller.frame();
        
        // dispatch events
        dispatch_found_events(old_frame, m_frame);
        dispatch_update_events(old_frame, m_frame);
        dispatch_lost_events(old_frame, m_frame);        
        for(const auto &gesture : m_frame.gestures()){m_gesture_detected(gesture);}
        
        int num_hands = m_frame.hands().count();
        
        if (num_hands){ LOG_TRACE<< num_hands <<" hands detected"; }
        
        HandList tmp_hand_list;
        PointableList tmp_pointables;
        
        for (const Hand &hand : m_frame.hands())
        {
            if(!hand.isValid()) continue;
            hand_struct h;
            h.position = type_cast(hand.sphereCenter());
            h.sphere_size = hand.sphereRadius();
            tmp_hand_list.push_back(h);
        }
        
        for(const Pointable &pointable : m_frame.pointables())
        {
            if(!pointable.isValid()) continue;
            pointable_struct p;
            p.position = type_cast(pointable.tipPosition());
            tmp_pointables.push_back(p);
        }
        
        boost::mutex::scoped_lock lock(m_mutex);
        m_gesture_list.append(m_frame.gestures());
        m_hand_list = tmp_hand_list;
        m_pointables = tmp_pointables;
    }
    
    void LeapConnector::dispatch_found_events(const Leap::Frame &old_frame,
                                              const Leap::Frame &new_frame)
	{
		for(const Hand &h : new_frame.hands())
		{
			if(!h.isValid())
				continue;
			if( !old_frame.hand(h.id()).isValid())
				m_hand_found(h);
		}
		for(const Pointable &p : new_frame.pointables())
		{
			if(!p.isValid())
				continue;
			if(!old_frame.pointable(p.id()).isValid())
				m_pointable_found(p);
		}
	}
    
    void LeapConnector::dispatch_lost_events(const Leap::Frame &old_frame,
                                             const Leap::Frame &new_frame)
	{
		for(const Hand &h : old_frame.hands())
		{
			if(!h.isValid())
				continue;
			if( !new_frame.hand(h.id()).isValid())
				m_hand_lost(h.id());
		}
		for(const Pointable &p : old_frame.pointables())
		{
			if(!p.isValid())
				continue;
			if(!new_frame.pointable(p.id()).isValid())
				m_pointable_lost(p.id());
		}
	}
    
    void LeapConnector::dispatch_update_events(const Leap::Frame &old_frame,
                                               const Leap::Frame &new_frame)
	{
		for(const Hand &h : old_frame.hands())
		{
			if(!h.isValid())
				continue;
			if(old_frame.hand(h.id()).isValid())
				m_hand_updated(h);
		}
		for(const Pointable &p : old_frame.pointables())
		{
			if(!p.isValid())
				continue;
			if(old_frame.pointable(p.id()).isValid())
				m_pointable_updated(p);
		}
	}
    
    Leap::GestureList LeapConnector::poll_gestures()
    {
        boost::mutex::scoped_lock lock(m_mutex);
        GestureList ret = m_gesture_list;
        m_gesture_list = Leap::GestureList();
        return ret;
    }
    
    Leap::Frame LeapConnector::lastFrame() const
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return m_frame;
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
