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

#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "boost/shared_ptr.hpp"
#include "boost/thread.hpp"
#include <boost/timer/timer.hpp>

#ifdef KINSKI_FREENECT
#include "KinectDevice.h"
#endif

namespace kinski {
    
    class ISourceNode
    {
    public:
        typedef boost::shared_ptr<ISourceNode> Ptr;
        
        virtual bool hasImage() = 0;
        virtual cv::Mat getNextImage() = 0;
    };
    
    class IBufferedSourceNode : public ISourceNode {};
    
    class IProcessNode
    {
    public:
        typedef boost::shared_ptr<IProcessNode> Ptr;
        
        virtual cv::Mat doProcessing(const cv::Mat &img) = 0;
    };
    
    class CVThread : public boost::noncopyable
    {
        
    public:
        
        typedef boost::shared_ptr<CVThread> Ptr;
        
        enum CVStreamType{NO_STREAM, STREAM_FILELIST,STREAM_IP_CAM,
            STREAM_CAPTURE,STREAM_KINECT, STREAM_VIDEOFILE};
        
        CVThread();
        virtual ~CVThread();
        
        void openImage(const std::string& imgPath);
        void openSequence(const std::vector<std::string>& files);
        
        void streamVideo(const std::string& path2Video);
        void streamUSBCamera(bool b,int camId = 0);
        void streamIPCamera(bool b);
        void streamKinect(bool b);
        
        bool saveCurrentFrame(const std::string& savePath);
        
        void playPause();
        void jumpToFrame(int index);
        void skipFrames(int num);
        
        void start();
        void stop();
        void operator()();
        
        const std::vector<std::string>& getSeq(){ return m_filesToStream;};
        
        bool isCameraActive(){return isUSBCameraActive() || isIPCameraActive();};
        
        bool isUSBCameraActive() const {return m_capture.isOpened();} ;
        bool isIPCameraActive() const {return false ;} ;
        
#ifdef KINSKI_FREENECT
        bool isKinectActive() const {return m_freenect.use_count() > 0 ;};
        void setKinectUseIR(bool b);
        bool isKinectUseIR(){return m_kinectUseIR;};
#endif
        
        void setImage(const cv::Mat& img);
        
        bool getImage(cv::Mat& img);
        
        void setDoProcessing(bool b){m_doProcessing=b;};
        bool hasProcessing(){return m_doProcessing;};
        
        int getCurrentIndex();
        inline int getNumFrames(){return m_numVideoFrames;};
        inline const std::string& getVideoPath(){return m_videoPath;};
        
        double getLastGrabTime();
        double getLastProcessTime();
        
        void setFPS(const double& fps){m_captureFPS=fps;};
        double getFPS(){return m_captureFPS;};
        
        CVStreamType getStreamType(){return m_streamType;};
        
        std::string getCurrentImgPath();
        
    private:
        
        CVStreamType m_streamType;
        
        std::vector<std::string> m_filesToStream ;
        int m_currentFileIndex;
        unsigned int m_sequenceStep;
        
        volatile bool m_stopped;
        volatile bool m_doProcessing;
        bool m_newFrame;
        
        // fetch next frame, depending on current m_streamType
        bool grabNextFrame();
        
        //-- OpenCV
        cv::VideoCapture m_capture ;
        
        IProcessNode::Ptr m_processNode;
        
        cv::Mat m_procImage;
        
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
        
        // handle for IP-camera
        //	AxisCamera* m_ipCamera;
        //	bool m_ipCameraActive;
        
        double m_lastGrabTime;
        double m_lastProcessTime;
        
        boost::thread m_thread;
        boost::mutex m_mutex;
        
        virtual cv::Mat doProcessing(const cv::Mat &img) 
        {return img;};//= 0;
    };
} // namespace kinski
#endif // CHTHREAD_H
