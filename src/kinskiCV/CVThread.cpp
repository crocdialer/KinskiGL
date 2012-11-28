//
//  CVThread.cpp
//  kinskiCV
//
//  Created by Fabian Schmidt on 6/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//
#include <fstream>
#include <boost/timer/timer.hpp>

#include "CVThread.h"

using namespace std;
using namespace cv;

using namespace boost::timer;

namespace kinski
{
    CVThread::CVThread():
    m_stopped(true), m_newFrame(false),
    m_processing(true),
    m_captureFPS(25.f)
    {	
        printf("CVThread -> OpenCV-Version: %s\n\n",CV_VERSION);
        //    cout<<cv::getBuildInformation()<<"\n";
    }
    
    CVThread::~CVThread()
    {
        stop();
    }
    
    void CVThread::start()
    {
        if(!m_stopped) return;
        
        m_thread = boost::thread(boost::ref(*this));
        m_stopped = false;
        
    }
    
    void CVThread::stop()
    {
        m_stopped = true;
        try
        {
            m_thread.join();
        }catch(std::exception &e)
        {
            cerr<<"CVThread: "<<e.what()<<endl;
        }
    }
    
    void CVThread::openImage(const std::string& imgPath)
    {	
        setImage(imread(imgPath));
    }
    
    void CVThread::playPause()
    {
        if(m_stopped)
            start();
        else 
            stop();
    }
    
    void CVThread::streamImageSequence(const std::vector<std::string>& files)
    {	
        
    }
    
    void CVThread::streamVideo(const std::string& path2Video, bool loop)
    {
        CVCaptureNode::Ptr capNode (new CVCaptureNode(path2Video));
        capNode->setLoop(loop);
        
        m_captureFPS = capNode->getFPS();
        m_sourceNode = capNode;
        start(); 
    }
    
    void CVThread::streamUSBCamera(int camId)
    {
        m_sourceNode = CVSourceNode::Ptr(new CVCaptureNode(0));
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
    
    void CVThread::operator()()
    {	
        m_stopped=false;
        
        // measure elapsed time with these
        boost::timer::cpu_timer threadTimer, cpuTimer;
        
        while( !m_stopped )
        {
            //restart timer
            threadTimer.start();        
            cpuTimer.start();
            
            // fetch frame, cancel loop when not possible
            // this call is supposed to be fast and not block the thread too long
            Mat inFrame;
            try 
            {
                inFrame = grabNextFrame();
            } 
            catch (std::exception &e)
            {
                std::cerr<<"CVThread: "<<e.what();
                break;
            }
            
            cpu_times grabTimes = cpuTimer.elapsed();
            
            //skip iteration when invalid frame is returned (eg. from camera)
            if(inFrame.empty()) continue;
            
            vector<Mat> tmpImages;
            tmpImages.push_back(inFrame);
            
            // image processing
            
            cpuTimer.start();
            
            if(hasProcessing())
            {
                //auto_cpu_timer autoTimer;
                vector<Mat> procImages = m_processNode->doProcessing(inFrame);
                
                tmpImages.insert(tmpImages.end(),
                                 procImages.begin(),
                                 procImages.end());
            }
            
            cpu_times processTimes = cpuTimer.elapsed();

            // locked scope
            {
                boost::mutex::scoped_lock lock(m_mutex);
                m_lastGrabTime = (grabTimes.wall) / 1.0e9;
                m_lastProcessTime = (processTimes.wall) / 1.0e9;
                m_images = tmpImages;
                m_newFrame = true;
                m_conditionVar.notify_all();
            }
            
            // thread timing
            
            double elapsed_msecs,sleep_msecs;
            
            elapsed_msecs = threadTimer.elapsed().wall / 1000000.0;
            sleep_msecs = max(0.0, (1000.0 / m_captureFPS - elapsed_msecs));
            
            // set thread asleep for a time to achieve desired framerate
            boost::posix_time::milliseconds msecs(sleep_msecs);
            boost::this_thread::sleep(msecs);
        }
        
        m_stopped = true;
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
        boost::mutex::scoped_lock lock(m_mutex);
        return m_newFrame;
    }
    
    void CVThread::waitForImage()
    {
        boost::mutex::scoped_lock lock(m_mutex);
        
        while (!m_newFrame)
            m_conditionVar.wait(lock);
    }
    
    void CVThread::setImage(const cv::Mat& img)
    {
        vector<Mat> tmpImages;
        
        if(hasProcessing())
        {
            vector<Mat> procImages = m_processNode->doProcessing(img);
            
            tmpImages.push_back(img);
            tmpImages.insert(tmpImages.end(),
                             procImages.begin(),
                             procImages.end());
        }
        
        boost::mutex::scoped_lock lock(m_mutex);
        m_images = tmpImages;
        m_newFrame = true;
    }
    
    const std::vector<cv::Mat>& CVThread::getImages()
    {
        boost::mutex::scoped_lock lock(m_mutex);
        if(m_newFrame)
            m_newFrame = false;
        
        return m_images;
    }
    
    double CVThread::getLastGrabTime()
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return m_lastGrabTime;
    }
    
    double CVThread::getLastProcessTime()
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return m_lastProcessTime;
    }
    
}// namespace kinski
