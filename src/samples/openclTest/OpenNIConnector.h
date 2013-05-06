//
//  OpenNIConnector.h
//  kinskiGL
//
//  Created by Fabian on 5/5/13.
//
//

#ifndef __kinskiGL__OpenNIConnector__
#define __kinskiGL__OpenNIConnector__

#include "boost/thread.hpp"
#include "kinskiGL/KinskiGL.h"
#include "kinskiCore/Component.h"

namespace kinski{ namespace gl{
    
    class OpenNIConnector : public kinski::Component
    {
    public:
        typedef std::shared_ptr<OpenNIConnector> Ptr;
        
        OpenNIConnector();
        ~OpenNIConnector();
        void updateProperty(const Property::ConstPtr &theProperty);
        
        void init();
        void start();
        void stop();
        
        //! thread runs here, do not fiddle around
        void operator()();
        
        std::list<std::pair<uint32_t, glm::vec3> > get_user_positions() const;
        
    private:
        
        struct Obj;
        typedef std::shared_ptr<Obj> ObjPtr;
        ObjPtr m_obj;
        
        //! Emulates shared_ptr-like behavior
        typedef ObjPtr OpenNIConnector::*unspecified_bool_type;
        operator unspecified_bool_type() const { return ( m_obj.get() == 0 ) ? 0 : &OpenNIConnector::m_obj; }
        void reset() { m_obj.reset(); }
        
        std::list<std::pair<uint32_t, glm::vec3> > m_user_list;
        bool m_running;
        boost::thread m_thread;
        mutable boost::mutex m_mutex;
        boost::condition_variable m_conditionVar;
        
        Property_<bool>::Ptr m_live_input;
        Property_<std::string>::Ptr m_config_path;
        Property_<std::string>::Ptr m_oni_path;
    };
    
    class OpenNIException: public Exception
    {
    public:
        OpenNIException(const std::string &theStr):
        Exception("OpenNI Exception: " + theStr){}
    };
    
}}//namespace

#endif /* defined(__kinskiGL__OpenNIConnector__) */
