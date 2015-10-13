//
//  CVThread.cpp
//  cv
//
//  Created by Fabian Schmidt on 6/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//
#include "CVThread.h"
#include "core/Logger.h"
#include <chrono>

typedef std::chrono::duration<float> float_second;

using namespace std;
using namespace cv;

namespace kinski
{
    CVThread::Ptr CVThread::create()
    {
        CVThread::Ptr ret(new CVThread());
        ret->observe_properties();
        return ret;
    }
    
    CVThread::CVThread():
    m_running(Property_<bool>::create("cvthread running", false)),
    m_processing(Property_<bool>::create("cvthread processing", true)),
    m_captureFPS(Property_<float>::create("max capture fps", 25.f)),
    m_newFrame(false)
    {
        register_property(m_running);
        register_property(m_processing);
        register_property(m_captureFPS);
        LOG_INFO<<"OpenCV-Version: " << CV_VERSION;
    }
    
    CVThread::~CVThread()
    {
        stop();
    }
    
    void CVThread::start()
    {
        if(*m_running) return;
        m_thread = std::thread(std::bind(&CVThread::run, this));
    }
    
    void CVThread::stop()
    {
        if(!*m_running) return;
        
        *m_running = false;
        try
        {
            m_thread.join();
        }catch(std::exception &e)
        {
            LOG_ERROR<<e.what();
        }
    }
    
    bool CVThread::is_running() const
    {
        return *m_running;
    }
    
    void CVThread::openImage(const std::string& imgPath)
    {	
        setImage(imread(imgPath));
    }
    
    void CVThread::playPause()
    {
        if(!*m_running)
            start();
        else 
            stop();
    }
    
    void CVThread::streamImageSequence(const std::vector<std::string>& files)
    {	
        
    }
    
    void CVThread::streamVideo(const std::string& path2Video, bool loop)
    {
        CVCaptureNode::Ptr cap_node (new CVCaptureNode(path2Video));
        cap_node->setLoop(loop);
        *m_captureFPS = cap_node->getFPS();
        m_sourceNode = cap_node;
        start(); 
    }
    
    void CVThread::streamUSBCamera(int camId)
    {
        CVCaptureNode::Ptr cap_node(new CVCaptureNode(0));
        m_sourceNode = cap_node;
        *m_captureFPS = cap_node->getFPS();
        start();
    }
    
    cv::Mat CVThread::grabNextFrame()
    {	
        //auto_cpu_timer t;
        Mat outMat;
        
        if(! (m_sourceNode && m_sourceNode->getNextImage(outMat)) )
            throw BadInputSourceException(m_sourceNode);

        return outMat; 
    }
    
    void CVThread::run()
    {	
        *m_running = true;
        
        while(*m_running)
        {
            //restart timer
            auto start_time = chrono::steady_clock::now();
            
            // fetch frame, cancel loop when not possible
            // this call is supposed to be fast and not block the thread too long
            Mat inFrame;
            try{ inFrame = grabNextFrame(); }
            catch (Exception &e)
            {
                LOG_ERROR<<e.what();
                break;
            }
            auto grab_duration = chrono::steady_clock::now() - start_time;
            
            //skip iteration when invalid frame is returned (eg. from camera)
            if(inFrame.empty()) continue;
        
            vector<Mat> tmpImages;
            tmpImages.push_back(inFrame);
            
            // image processing
            if(hasProcessing())
            {
                //auto_cpu_timer autoTimer;
                vector<Mat> procImages = m_processNode->doProcessing(inFrame);
                
                tmpImages.insert(tmpImages.end(),
                                 procImages.begin(),
                                 procImages.end());
            }
            auto img_proc_duration = chrono::steady_clock::now() - start_time - grab_duration;

            // locked scope
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_lastGrabTime = chrono::duration_cast<float_second>(grab_duration).count();
                m_lastProcessTime = chrono::duration_cast<float_second>(img_proc_duration).count();
                m_images = tmpImages;
                m_newFrame = true;
                m_conditionVar.notify_all();
            }
            // thread timing
            auto overall_duration = chrono::steady_clock::now() - start_time;
            
            double elapsed_msecs,sleep_msecs;
            elapsed_msecs = chrono::duration_cast<chrono::milliseconds>(overall_duration).count();
            sleep_msecs = max(0.0, (1000.0 / *m_captureFPS - elapsed_msecs));
            
            // set thread asleep for a time to achieve desired framerate
            this_thread::sleep_for(std::chrono::nanoseconds((long)(sleep_msecs * 1000000L)));
        }
        *m_running = false;
    }
    
    string CVThread::getSourceInfo()
    {
        string out;
        if(m_sourceNode) out = m_sourceNode->getDescription();
        return out;
    }
    
    // -- Getter / Setter
    bool CVThread::hasImage()
    {	
        //boost::mutex::scoped_lock lock(m_mutex);
        return m_newFrame;
    }
    
    void CVThread::waitForImage()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (!m_newFrame)
            m_conditionVar. wait(lock);
    }
    
    void CVThread::setImage(const cv::Mat& img)
    {
        vector<Mat> tmpImages;
        tmpImages.push_back(img);
        
        if(hasProcessing())
        {
            vector<Mat> procImages = m_processNode->doProcessing(img);
            tmpImages.insert(tmpImages.end(),
                             procImages.begin(),
                             procImages.end());
        }
        std::unique_lock<std::mutex> lock(m_mutex);
        m_images = tmpImages;
        m_newFrame = true;
    }
    
    std::vector<cv::Mat> CVThread::getImages()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if(m_newFrame)
            m_newFrame = false;
        
        return m_images;
    }
    
    float CVThread::getLastGrabTime() const
    {
        return m_lastGrabTime;
    }
    
    float CVThread::getLastProcessTime() const
    {
        return m_lastProcessTime;
    }
    
    void CVThread::update_property(const Property::ConstPtr &theProperty)
    {
        if(theProperty == m_running)
        {
            if(*m_running){ start(); }
        }
    }
    
}// namespace kinski
