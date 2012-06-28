//
//  CVThread.cpp
//  kinskiCV
//
//  Created by Fabian Schmidt on 6/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "CVThread.h"
#include <fstream>

using namespace std;
using namespace cv;

using namespace boost::timer;

namespace kinski {

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
        m_stopped=true;
        m_thread.join();
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
    
    void CVThread::openSequence(const std::vector<std::string>& files)
    {	

    }
    
    bool CVThread::saveCurrentFrame(const std::string& savePath)
    {
        return imwrite(savePath,hasProcessing()? m_procImage : m_procImage);
        
    }
    
    void CVThread::streamVideo(const std::string& path2Video, bool loop)
    {
        CvCaptureNode::Ptr capNode (new CvCaptureNode(path2Video));
        capNode->setLoop(loop);
        
        m_captureFPS = capNode->getFPS();
        m_sourceNode = capNode;
        start(); 
    }
    
    void CVThread::streamUSBCamera(int camId)
    {
        m_sourceNode = CVSourceNode::Ptr(new CvCaptureNode(0));
        start();
    }
    
    cv::Mat CVThread::grabNextFrame()
    {	
        //auto_cpu_timer t;
        Mat outMat;
        
        if(! (m_sourceNode && m_sourceNode->getNextImage(outMat)) )
            throw NoInputSourceException();

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
            try 
            {
                m_procImage = grabNextFrame();
            } 
            catch (std::exception e) 
            {
                std::cerr<<e.what();
                break;
            }
            
            //skip iteration when invalid frame is returned (eg. from camera)
            if(m_procImage.empty()) continue;
            
            cpu_times t = cpuTimer.elapsed();
            m_lastGrabTime = (t.wall) / 1000000000.0;
            
            // image processing
            {   
                //auto_cpu_timer autoTimer;
                boost::mutex::scoped_lock lock(m_mutex);
                cpuTimer.start();
                
                if(hasProcessing())
                {   
                    m_procImage = m_processNode->doProcessing(m_procImage);
                }
                
                m_newFrame = true;
                
                t = cpuTimer.elapsed();
                m_lastProcessTime = (t.wall) / 1000000000.0;
            }
            
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
    bool CVThread::getImage(cv::Mat& img)
    {	
        boost::mutex::scoped_lock lock(m_mutex);
        if(m_newFrame)
        {
            img = m_procImage;
            m_newFrame = false;
            return true;
        }
        
        return false;
    }
    
    void CVThread::setImage(const cv::Mat& img)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        m_procImage = img.clone();
        m_newFrame = true;
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