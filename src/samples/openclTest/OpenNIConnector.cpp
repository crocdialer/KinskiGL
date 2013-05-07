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
#include <boost/timer/timer.hpp>
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
    m_running(false),
    m_live_input(Property_<bool>::create("Live input", false)),
    m_config_path(Property_<string>::create("Config path", "ni_config.xml")),
    m_oni_path(Property_<string>::create("Oni path", ""))
    {
        set_name("OpenNIConnector");
        registerProperty(m_live_input);
        registerProperty(m_config_path);
        registerProperty(m_oni_path);
    }
    
    OpenNIConnector::~OpenNIConnector()
    {
        stop();
    }
    
    void OpenNIConnector::init()
    {
        LOG_DEBUG<<"initializing OpenNI ...";
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
            
            if(ni_status != XN_STATUS_OK){throw OpenNIException(xnGetStatusString(ni_status));}
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
            if(ni_status != XN_STATUS_OK){throw OpenNIException(xnGetStatusString(ni_status));}
        }
        
        // create a depth generator
        ni_status = m_obj->m_context.FindExistingNode(XN_NODE_TYPE_DEPTH, m_obj->m_depthGenerator);
        if(ni_status != XN_STATUS_OK){throw OpenNIException(xnGetStatusString(ni_status));}
        
        // create a user generator
        ni_status = m_obj->m_context.FindExistingNode(XN_NODE_TYPE_USER, m_obj->m_userGenerator);
        if (ni_status != XN_STATUS_OK)
        {
            ni_status = m_obj->m_userGenerator.Create(m_obj->m_context);
            if(ni_status != XN_STATUS_OK){throw OpenNIException(xnGetStatusString(ni_status));}
        }
        
        if (!m_obj->m_userGenerator.IsCapabilitySupported(XN_CAPABILITY_SKELETON))
        {
            LOG_WARNING<<"Supplied user generator doesn't support skeleton";
        }
        XnCallbackHandle hUserCallbacks, hCalibrationStart, hCalibrationComplete;
        ni_status = m_obj->m_userGenerator.RegisterUserCallbacks(Obj::new_user, Obj::lost_user,
                                                                 m_obj.get(), hUserCallbacks);
        if(ni_status != XN_STATUS_OK){throw OpenNIException(xnGetStatusString(ni_status));}
        
        ni_status = m_obj->m_userGenerator.GetSkeletonCap().RegisterToCalibrationStart(Obj::calibration_start,
                                                                                       m_obj.get(),
                                                                                       hCalibrationStart);
        if(ni_status != XN_STATUS_OK){throw OpenNIException(xnGetStatusString(ni_status));}
        ni_status = m_obj->m_userGenerator.GetSkeletonCap().RegisterToCalibrationComplete(Obj::calibration_complete,
                                                                                          m_obj.get(),
                                                                                          hCalibrationComplete);
        if(ni_status != XN_STATUS_OK){throw OpenNIException(xnGetStatusString(ni_status));}
        
        ni_status = m_obj->m_context.StartGeneratingAll();
        if(ni_status != XN_STATUS_OK){throw OpenNIException(xnGetStatusString(ni_status));}
        LOG_DEBUG<<"init complete";
    }
    
    // Callback: New user was detected
    void XN_CALLBACK_TYPE OpenNIConnector::Obj::new_user(xn::UserGenerator& /*generator*/,
                                                         XnUserID nId, void* cookie)
    {
        Obj *myObj = static_cast<Obj*>(cookie);
        char str_pose[20];
        LOG_DEBUG<<"New User "<<nId;
        
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
        LOG_DEBUG<<"Lost user "<<nId;
    }
    
    void XN_CALLBACK_TYPE
    OpenNIConnector::Obj::pose_detected(xn::PoseDetectionCapability& /*capability*/,
                                        const XnChar* strPose, XnUserID nId, void* cookie)
    {
        Obj *myObj = static_cast<Obj*>(cookie);
        LOG_DEBUG<<"Pose "<<strPose<<" detected for user "<<nId;
        myObj->m_userGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
        myObj->m_userGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
    }
    // Callback: Started calibration
    void XN_CALLBACK_TYPE
    OpenNIConnector::Obj::calibration_start(xn::SkeletonCapability& /*capability*/, XnUserID nId,
                                            void* /*pCookie*/)
    {
        LOG_DEBUG<<"Calibration started for user "<<nId;
    }
    
    // Callback: Finished calibration
    void XN_CALLBACK_TYPE
    OpenNIConnector::Obj::calibration_complete(xn::SkeletonCapability& /*capability*/,
                                               XnUserID nId, XnCalibrationStatus eStatus,
                                               void* cookie)
    {
        Obj *myObj = static_cast<Obj*>(cookie);
        if (eStatus == XN_CALIBRATION_STATUS_OK)
        {
            // Calibration succeeded
            LOG_DEBUG<<"Calibration complete, start tracking user "<<nId;
            myObj->m_userGenerator.GetSkeletonCap().StartTracking(nId);
        }
        else
        {
            char str_pose[20];
            
            // Calibration failed
            LOG_DEBUG<<"Calibration failed for user "<<nId;
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
            stop();
            if(*m_live_input) start();
        }
        else if(theProperty == m_oni_path)
        {
            if(!*m_live_input)
            {
                stop();
                start();
            }
        }
    }
    
    void OpenNIConnector::start()
    {
        if(m_running) return;
        m_thread = boost::thread(boost::ref(*this));
        m_running = true;
    }
    
    void OpenNIConnector::stop()
    {
        m_running = false;
        try{m_thread.join();}
        catch(std::exception &e){LOG_ERROR<<e.what();}
    }
    
    void OpenNIConnector::operator()()
    {
        m_running = true; 
        try{init();}
        catch(OpenNIException &e){LOG_ERROR<<e.what(); m_running = false;}
        
        // measure elapsed time with these
        boost::timer::cpu_timer threadTimer, cpuTimer;
        xn::SceneMetaData sceneMD;
        xn::DepthMetaData depthMD;
        
        while(m_running)
        {
            // Read next available data
            m_obj->m_context.WaitOneUpdateAll(m_obj->m_userGenerator);
            
            // Process the data
            m_obj->m_depthGenerator.GetMetaData(depthMD);
            m_obj->m_userGenerator.GetUserPixels(0, sceneMD);
            
            XnPoint3D user_center;
            XnUInt16 num_users = m_obj->m_userGenerator.GetNumberOfUsers();
            uint32_t user_ids[num_users];
            m_obj->m_userGenerator.GetUsers(user_ids, num_users);
            
            //locked scope
            {
                boost::mutex::scoped_lock lock(m_mutex);
                m_user_list.clear();
                for (int i = 0; i < num_users; ++i)
                {
                    m_obj->m_userGenerator.GetCoM(user_ids[i], user_center);
                    glm::vec3 p = glm::make_vec3((float*)&user_center);
                    // zombie filter
                    if(p != glm::vec3(0))
                    {
                        m_user_list.push_back(make_pair(user_ids[i], p));
                    }
                }
            }
        }
        m_running = false;
    }
    
    std::list<std::pair<uint32_t, glm::vec3> > OpenNIConnector::get_user_positions() const
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return m_user_list;
    }
}}