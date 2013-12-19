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
#include "boost/signals2.hpp"
#include "boost/asio.hpp"
#include "Leap.h"


namespace kinski{ namespace leap{
    
    inline glm::vec3 type_cast(const Leap::Vector &v){ return glm::vec3(v.x, v.y, v.z); }
    
    typedef boost::signals2::signal<void(const Leap::Pointable&)> signal_pointable;
    typedef boost::signals2::signal<void(const Leap::Hand&)> signal_hand;
    typedef boost::signals2::signal<void(int)> signal_object_lost;
    typedef boost::signals2::signal<void(const Leap::Gesture&)> signal_gesture;
    
    // callback definitions
    typedef signal_pointable::slot_type PointableFoundHandler;
	typedef signal_pointable::slot_type PointableUpdatedHandler;
    typedef signal_object_lost::slot_type PointableLostHandler;
	typedef signal_hand::slot_type HandFoundHandler;
    typedef signal_hand::slot_type HandUpdatedHandler;
	typedef signal_object_lost::slot_type HandLostHandler;
    typedef signal_gesture::slot_type GestureHandler;
    
    class EventListenerBase
    {
        EventListenerBase(){};
        
        virtual void hand_found(const Leap::Hand &p){};
        virtual void hand_updated(const Leap::Hand &p){};
        virtual void hand_lost(int the_id){};
        virtual void pointable_found(const Leap::Pointable &p){};
        virtual void pointable_updated(const Leap::Pointable &p){};
        virtual void pointable_lost(int the_id){};
        
    };
    
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
        
        std::shared_ptr<Leap::Controller> controller(){return m_controller;};
        Leap::Frame lastFrame() const;
        const HandList& hands() const;
        const PointableList& pointables() const;
        Leap::GestureList poll_gestures();
        
        void use_io_service(boost::asio::io_service &io);
        
        void start();
        void stop();
        
    private:
        
        void dispatch_found_events(const Leap::Frame &old_frame, const Leap::Frame &new_frame);
        void dispatch_update_events(const Leap::Frame &old_frame, const Leap::Frame &new_frame);
        void dispatch_lost_events(const Leap::Frame &old_frame, const Leap::Frame &new_frame);
        
        std::shared_ptr<Leap::Controller> m_controller;
        mutable boost::mutex m_mutex;
        Leap::Frame m_frame;
        
        signal_hand m_hand_found, m_hand_updated;
        signal_object_lost m_hand_lost, m_pointable_lost;
        signal_pointable m_pointable_found, m_pointable_updated;
        signal_gesture m_gesture_detected;
        
        Leap::GestureList m_gesture_list;
        HandList m_hand_list;
        PointableList m_pointables;
    };
}}// namespaces

#endif /* defined(__kinskiGL__LeapConnector__) */
