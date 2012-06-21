/*
 *  CVThread.cpp
 *  TestoRAma
 *
 *  Created by Fabian on 9/13/10.
 *  Copyright 2010. All rights reserved.
 *
 */

#include "CVThread.h"
#include <fstream>

#include "Colormap.h"

using namespace std;
using namespace cv;

using namespace boost::timer;

namespace kinski {

    class ThreshNode : public CVProcessNode
    {
    public:
        ThreshNode():m_colorMap(Colormap::BONE){};
        virtual ~ThreshNode(){};
        
        cv::Mat doProcessing(const cv::Mat &img)
        {
            Mat grayImg, threshImg, out;
            cvtColor(img, grayImg, CV_BGR2GRAY);
            threshold(grayImg, threshImg, 50, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
            out = threshImg;
            
            return m_colorMap.apply(out);
        };
        
    private:
        Colormap m_colorMap;
    };
    
    CVThread::CVThread():m_currentFileIndex(0),m_sequenceStep(1),
    m_stopped(true), m_newFrame(false),
    //m_processNode(new ThreshNode),
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
        
        m_currentFileIndex = 0 ;
        m_filesToStream = files;
        m_numVideoFrames = files.size();
        
        //m_bufferThread->setSeq(files);
        
        //TODO: implement a SequenceSourceNode
    }
    
    bool CVThread::saveCurrentFrame(const std::string& savePath)
    {
        return imwrite(savePath,hasProcessing()? m_procImage : m_procImage);
        
    }
    
    void CVThread::skipFrames(int num)
    {	
        jumpToFrame( m_currentFileIndex + num );
        
    }
    
    void CVThread::jumpToFrame(int newIndex)
    {	
        m_currentFileIndex = newIndex < 0 ? 0 : newIndex;
        
//        switch (m_streamType) 
//        {
//            case STREAM_VIDEOFILE:
//                
//                if(m_currentFileIndex >= m_numVideoFrames)
//                    m_currentFileIndex = m_numVideoFrames-1;
//                
//                //m_capture.set(CV_CAP_PROP_POS_FRAMES,m_currentFileIndex);
//                
//                break;
//                
//            default:
//            case STREAM_FILELIST:
//                
//                if(m_filesToStream.empty())
//                    return;
//                
//                if(m_currentFileIndex >= (int)m_filesToStream.size())
//                    m_currentFileIndex = m_filesToStream.size()-1;
//                
//                break;
//                
//        }
        
        if(m_stopped)
        {	
            cpu_timer timer;
            
            m_procImage = grabNextFrame();
            
            if(m_procImage.size() != Size(0,0)) 
            {	
                //m_lastGrabTime = m_timer.elapsed();
                
                timer.start();
                
                if(hasProcessing())
                {
                    m_procImage = m_processNode->doProcessing(m_procImage);
                    
                    cpu_times t = timer.elapsed();
                    t = timer.elapsed();
                    m_lastProcessTime = (t.user + t.system) / 1000000000.0;
                }
                
            }
        }

    }
    
    void CVThread::streamVideo(const std::string& path2Video)
    {
        CvCaptureNode::Ptr capNode (new CvCaptureNode(path2Video));
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
        
        if(! (m_sourceNode && m_sourceNode->hasImage()) )
            throw NoInputSourceException();

        return m_sourceNode->getNextImage(); 
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
            catch (NoInputSourceException e) 
            {
                std::cerr<<e.what();
                break;
            }
            
            //skip iteration when invalid frame is returned (eg. from camera)
            if(m_procImage.empty()) continue;
            
            cpu_times t = cpuTimer.elapsed();
            m_lastGrabTime = (t.user + t.system) / 1000000000.0;
            
            // image processing
            {   
                boost::mutex::scoped_lock lock(m_mutex);
                cpuTimer.start();
                
                if(hasProcessing())
                {   
                    m_procImage = m_processNode->doProcessing(m_procImage);
                }
                
                m_newFrame = true;
                
                t = cpuTimer.elapsed();
                m_lastProcessTime = (t.user + t.system) / 1000000000.0;
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
    
    int CVThread::getCurrentIndex()
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return m_currentFileIndex;
    }
    
}// namespace kinski