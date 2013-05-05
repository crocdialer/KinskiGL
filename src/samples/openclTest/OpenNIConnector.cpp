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
        
        XnStatus ni_status = XN_STATUS_OK;
        
        // hook up a attached camera
        if(*m_live_input)
        {
            xn::EnumerationErrors errors;
            try
            {
                ni_status = m_obj->m_context.InitFromXmlFile(searchFile(*m_config_path).c_str(),
                                                             m_obj->m_scriptNode, &errors);
            } catch (FileNotFoundException &e){LOG_ERROR<<e.what();}
            
            LOG_ERROR_IF(ni_status != XN_STATUS_OK)<<xnGetStatusString(ni_status);
        }
        // load oni file
        else
        {
            if(m_oni_path->value().empty()) return;
            
            m_obj->m_context.Init();
            try
            {
                ni_status = m_obj->m_context.OpenFileRecording(searchFile(*m_oni_path).c_str(),
                                                               m_obj->m_player);
            }catch (FileNotFoundException &e){LOG_ERROR<<e.what();}
            LOG_ERROR_IF(ni_status != XN_STATUS_OK)<<xnGetStatusString(ni_status);
        }
        
        // create a depth generator
        ni_status = m_obj->m_context.FindExistingNode(XN_NODE_TYPE_DEPTH, m_obj->m_depthGenerator);
        LOG_ERROR_IF(ni_status != XN_STATUS_OK)<<xnGetStatusString(ni_status);
        
        // create a user generator
        ni_status = m_obj->m_context.FindExistingNode(XN_NODE_TYPE_USER, m_obj->m_userGenerator);
        if (ni_status != XN_STATUS_OK)
        {
            ni_status = m_obj->m_userGenerator.Create(m_obj->m_context);
            LOG_ERROR_IF(ni_status != XN_STATUS_OK)<<xnGetStatusString(ni_status);
        }
    }
    
    void OpenNIConnector::updateProperty(const Property::ConstPtr &theProperty)
    {
        if(theProperty == m_live_input)
        {
            init();
        }
        else if(theProperty == m_oni_path)
        {
            if(!*m_live_input) init();
        }
    }
}}