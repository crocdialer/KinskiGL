/*
 *  CVThread.h
 *  TestoRAma
 *
 *  Created by Fabian on 9/13/10.
 *  Copyright 2010 LMU. All rights reserved.
 *
 */

#ifndef CHTHREAD_H
#define CHTHREAD_H

#include "opencv2/highgui/highgui.hpp"

#include "boost/shared_ptr.hpp"
#include "boost/thread.hpp"
#include <boost/timer/timer.hpp>

#ifdef KINSKI_FREENECT
#include "KinectDevice.h"
#endif

#include "CVNode.h"

namespace kinski {
    
    class CVThread : public boost::noncopyable
    {
        
    public:
        
        typedef boost::shared_ptr<CVThread> Ptr;
             
        CVThread();
        virtual ~CVThread();
        
        void openImage(const std::string& imgPath);
        void openSequence(const std::vector<std::string>& files);
        
        void streamVideo(const std::string& path2Video);
        void streamUSBCamera(int camId = 0);
        void streamIPCamera(bool b);
        
        bool saveCurrentFrame(const std::string& savePath);
        
        void playPause();
        void jumpToFrame(int index);
        void skipFrames(int num);
        
        // thread management
        void start();
        void stop();
        void operator()();
        
        const std::vector<std::string>& getSeq(){ return m_filesToStream;};
        
#ifdef KINSKI_FREENECT
        bool isKinectActive() const {return m_freenect.use_count() > 0 ;};
        void setKinectUseIR(bool b);
        bool isKinectUseIR(){return m_kinectUseIR;};
        void streamKinect(bool b);
#endif
        
        void setImage(const cv::Mat& img);
        bool getImage(cv::Mat& img);
        
        inline bool hasProcessing(){return m_processNode;};
        void setProcessingNode(const CVProcessNode::Ptr pn){m_processNode = pn;};
        
        int getCurrentIndex();
        inline int getNumFrames(){return m_numVideoFrames;};
        inline const std::string& getVideoPath(){return m_videoPath;};
        
        double getLastGrabTime();
        double getLastProcessTime();
        
        void setFPS(const double& fps){m_captureFPS=fps;};
        double getFPS(){return m_captureFPS;};
        
        std::string getCurrentImgPath();
        
    private:
        
        std::vector<std::string> m_filesToStream ;
        int m_currentFileIndex;
        unsigned int m_sequenceStep;
        
        volatile bool m_stopped;
        bool m_newFrame;
        
        cv::Mat m_procImage;
        
        //-- OpenCV
        CVSourceNode::Ptr m_sourceNode;
        CVProcessNode::Ptr m_processNode;
        
        // fetch next frame, depending on current sourceNode
        cv::Mat grabNextFrame();

        // Kinect
#ifdef KINSKI_FREENECT
        Freenect::Ptr m_freenect;
        KinectDevice* m_kinectDevice;
        bool m_kinectUseIR;
#endif
        
        //desired capturing / seconds  -> used to time threadmanagment
        double m_captureFPS;
        
        //number of frames in current videofile from VideoCapture
        int m_numVideoFrames;
        std::string m_videoPath;
        
        double m_lastGrabTime;
        double m_lastProcessTime;
        
        boost::thread m_thread;
        boost::mutex m_mutex;
        
    };
    
    class NoInputSourceException : public std::runtime_error
    {
    public:
        NoInputSourceException() : 
        std::runtime_error(std::string("No input source defined ..."))
        {}
    };
    
} // namespace kinski
#endif // CHTHREAD_H
