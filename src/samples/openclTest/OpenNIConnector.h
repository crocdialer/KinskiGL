//
//  OpenNIConnector.h
//  kinskiGL
//
//  Created by Fabian on 5/5/13.
//
//

#ifndef __kinskiGL__OpenNIConnector__
#define __kinskiGL__OpenNIConnector__

#include "kinskiGL/KinskiGL.h"
#include "kinskiCore/Component.h"

namespace kinski{ namespace gl{
    
    class OpenNIConnector : public kinski::Component
    {
    public:
        typedef std::shared_ptr<OpenNIConnector> Ptr;
        
        OpenNIConnector();
        void update();
        void updateProperty(const Property::ConstPtr &theProperty);
        
    private:
        void init();
        struct Obj;
        typedef std::shared_ptr<Obj> ObjPtr;
        ObjPtr m_obj;
        
        //! Emulates shared_ptr-like behavior
        typedef ObjPtr OpenNIConnector::*unspecified_bool_type;
        operator unspecified_bool_type() const { return ( m_obj.get() == 0 ) ? 0 : &OpenNIConnector::m_obj; }
        void reset() { m_obj.reset(); }
        
        Property_<bool>::Ptr m_live_input;
        Property_<std::string>::Ptr m_config_path;
        Property_<std::string>::Ptr m_oni_path;
    };
    
    class OpenNIException: public Exception
    {
    public:
        OpenNIException() :
        Exception("got trouble with OpenNI"){}
    };
    
}}//namespace

#endif /* defined(__kinskiGL__OpenNIConnector__) */
