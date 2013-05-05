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
        
        XnCallbackHandle hUserCallbacks, hCalibrationStart, hCalibrationComplete;
        
        Obj(){}
        
        ~Obj()
        {
            m_scriptNode.Release();
            m_depthGenerator.Release();
            m_userGenerator.Release();
            m_player.Release();
            m_context.Release();
            LOG_DEBUG<<"ciao OpenNIConnector::Obj";
        }
        
        static void XN_CALLBACK_TYPE new_user(xn::UserGenerator& /*generator*/, XnUserID nId,
                                              void* cookie);
        static void XN_CALLBACK_TYPE lost_user(xn::UserGenerator& /*generator*/, XnUserID nId,
                                               void* cookie);
        static void XN_CALLBACK_TYPE calibration_start(xn::SkeletonCapability& /*capability*/,
                                                       XnUserID nId, void* cookie);
        static void XN_CALLBACK_TYPE calibration_complete(xn::SkeletonCapability& /*capability*/,
                                                          XnUserID nId, XnCalibrationStatus eStatus,
                                                   void* cookie);
        static void XN_CALLBACK_TYPE pose_detected(xn::PoseDetectionCapability& /*capability*/,
                                                   const XnChar* strPose, XnUserID nId, void* cookie);
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
        LOG_DEBUG<<"init begin";
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
        
        if (!m_obj->m_userGenerator.IsCapabilitySupported(XN_CAPABILITY_SKELETON))
        {
            LOG_ERROR<<"Supplied user generator doesn't support skeleton";
            return;
        }
        
        ni_status = m_obj->m_userGenerator.RegisterUserCallbacks(Obj::new_user, Obj::lost_user,
                                                                 m_obj.get(), m_obj->hUserCallbacks);
        LOG_ERROR_IF(ni_status != XN_STATUS_OK)<<xnGetStatusString(ni_status);
        
        ni_status = m_obj->m_userGenerator.GetSkeletonCap().RegisterToCalibrationStart(Obj::calibration_start,
                                                                                       m_obj.get(),
                                                                                       m_obj->hCalibrationStart);
        LOG_ERROR_IF(ni_status != XN_STATUS_OK)<<xnGetStatusString(ni_status);
        ni_status = m_obj->m_userGenerator.GetSkeletonCap().RegisterToCalibrationComplete(Obj::calibration_complete,
                                                                                          m_obj.get(),
                                                                                          m_obj->hCalibrationComplete);
        LOG_ERROR_IF(ni_status != XN_STATUS_OK)<<xnGetStatusString(ni_status);
        
        ni_status = m_obj->m_context.StartGeneratingAll();
        LOG_ERROR_IF(ni_status != XN_STATUS_OK)<<xnGetStatusString(ni_status);
        LOG_DEBUG<<"init done";
    }
    
    void OpenNIConnector::update()
    {
        xn::SceneMetaData sceneMD;
        xn::DepthMetaData depthMD;
        m_obj->m_depthGenerator.GetMetaData(depthMD);
        
            // Read next available data
            m_obj->m_context.WaitOneUpdateAll(m_obj->m_userGenerator);

		// Process the data
		m_obj->m_depthGenerator.GetMetaData(depthMD);
		m_obj->m_userGenerator.GetUserPixels(0, sceneMD);
    }
    
    // Callback: New user was detected
    void XN_CALLBACK_TYPE OpenNIConnector::Obj::new_user(xn::UserGenerator& /*generator*/,
                                                         XnUserID nId, void* cookie)
    {
        Obj *myObj = static_cast<Obj*>(cookie);
        char str_pose[20];
        XnUInt32 epochTime = 0;
        xnOSGetEpochTime(&epochTime);
        LOG_DEBUG<<epochTime<<" New User "<<nId;
        
        // New user found
        if (false)//g_bNeedPose)
        {
            myObj->m_userGenerator.GetPoseDetectionCap().StartPoseDetection(str_pose, nId);
        }
        else
        {
            myObj->m_userGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
        }
    }
    
    // Callback: An existing user was lost
    void XN_CALLBACK_TYPE OpenNIConnector::Obj::lost_user(xn::UserGenerator& /*generator*/,
                                                          XnUserID nId, void* cookie)
    {
        //Obj *myObj = static_cast<Obj*>(cookie);
        XnUInt32 epochTime = 0;
        xnOSGetEpochTime(&epochTime);
        printf("%d Lost user %d\n", epochTime, nId);	
    }
    
    void XN_CALLBACK_TYPE
    OpenNIConnector::Obj::pose_detected(xn::PoseDetectionCapability& /*capability*/,
                                        const XnChar* strPose, XnUserID nId, void* cookie)
    {
        Obj *myObj = static_cast<Obj*>(cookie);
        XnUInt32 epochTime = 0;
        xnOSGetEpochTime(&epochTime);
        LOG_DEBUG<<epochTime<<" Pose "<<strPose<<" detected for user "<<nId;
        myObj->m_userGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
        myObj->m_userGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
    }
    // Callback: Started calibration
    void XN_CALLBACK_TYPE
    OpenNIConnector::Obj::calibration_start(xn::SkeletonCapability& /*capability*/, XnUserID nId,
                                            void* /*pCookie*/)
    {
        XnUInt32 epochTime = 0;
        xnOSGetEpochTime(&epochTime);
        LOG_DEBUG<<epochTime<<" Calibration started for user "<<nId;
    }
    
    // Callback: Finished calibration
    void XN_CALLBACK_TYPE
    OpenNIConnector::Obj::calibration_complete(xn::SkeletonCapability& /*capability*/,
                                               XnUserID nId, XnCalibrationStatus eStatus,
                                               void* cookie)
    {
        Obj *myObj = static_cast<Obj*>(cookie);
        XnUInt32 epochTime = 0;
        xnOSGetEpochTime(&epochTime);
        if (eStatus == XN_CALIBRATION_STATUS_OK)
        {
            // Calibration succeeded
            LOG_DEBUG<<epochTime<<" Calibration complete, start tracking user "<<nId;
            myObj->m_userGenerator.GetSkeletonCap().StartTracking(nId);
        }
        else
        {
            char str_pose[20];
            
            // Calibration failed
            LOG_DEBUG<<epochTime<<" Calibration failed for user "<<nId;
            if(eStatus==XN_CALIBRATION_STATUS_MANUAL_ABORT)
            {
                LOG_DEBUG<<"Manual abort occured, stop attempting to calibrate!";
                return;
            }
            if (false)//g_bNeedPose)
            {
                myObj->m_userGenerator.GetPoseDetectionCap().StartPoseDetection(str_pose, nId);
            }
            else
            {
                myObj->m_userGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
            }
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