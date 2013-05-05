//
//  OpenNIConnector.cpp
//  kinskiGL
//
//  Created by Fabian on 5/5/13.
//
//

#include <XnOpenNI.h>
#include <XnCodecIDs.h>
#include <XnCppWrapper.h>
#include <XnPropNames.h>
#include "OpenNIConnector.h"

using namespace std;

namespace kinski{ namespace gl{
    
    struct OpenNIConnector::Obj
    {
        xn::Context m_context;
        xn::ScriptNode m_scriptNode;
        xn::DepthGenerator m_depthGenerator;
        xn::UserGenerator m_userGenerator;
        xn::Player m_player;
        
        Obj(){}
        
        ~Obj()
        {
            m_scriptNode.Release();
            m_depthGenerator.Release();
            m_userGenerator.Release();
            m_player.Release();
            m_context.Release();
        }
    };

    OpenNIConnector::OpenNIConnector():
    m_live_input(Property_<bool>::create("Live input", false)),
    m_config_path(Property_<string>::create("Config path", "ni_config.xml")),
    m_oni_path(Property_<string>::create("Oni path", ""))
    {
        set_name("OpenNIConnector");
        registerProperty(m_live_input);
        registerProperty(m_config_path);
        registerProperty(m_oni_path);
    }
    
    void OpenNIConnector::init()
    {
        m_obj = ObjPtr(new Obj);
        
    }
    
    void OpenNIConnector::updateProperty(const Property::ConstPtr &theProperty)
    {
        if(theProperty == m_live_input)
        {
        
        }
        else if(theProperty == m_oni_path)
        {
        
        }
    }
}}