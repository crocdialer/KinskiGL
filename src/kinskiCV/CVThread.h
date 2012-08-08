//
//  CVThread.h
//  kinskiCV
//
//  Created by Fabian Schmidt on 6/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef CHTHREAD_H
#define CHTHREAD_H

#include "opencv2/highgui/highgui.hpp"

#include "boost/shared_ptr.hpp"
#include "boost/thread.hpp"

#include "CVNode.h"

namespace kinski
{
    
    class CVThread : public boost::noncopyable
    {
    public:
        
        typedef boost::shared_ptr<CVThread> Ptr;
             
        CVThread();
        virtual ~CVThread();
        
        void openImage(const std::string& imgPath);
        
        void streamImageSequence(const std::vector<std::string>& files);
        void streamVideo(const std::string& path2Video, bool loop = false);
        void streamUSBCamera(int camId = 0);
        
        void playPause();
        
        void setImage(const cv::Mat& img);
        bool hasImage();
        void waitForImage();
        const std::vector<cv::Mat>& getImages();
        
        void setSourceNode(const CVSourceNode::Ptr sn){m_sourceNode = sn;};
        
        inline bool hasProcessing(){return m_processing && m_processNode;};
        inline void setProcessing(bool b){m_processing = b;};
        
        inline void setProcessingNode(const CVProcessNode::Ptr pn){m_processNode = pn;};
        inline CVProcessNode::Ptr getProcessingNode(){return m_processNode;};
        
        /*!
         *  @return the time in ms of the last execution of
         *  m_sourceNode->getNextImage()
         */
        double getLastGrabTime();
        
        /*!
         *  @return the time in ms of the last execution of
         *  m_processNode->doProcessing(...)
         */
        double getLastProcessTime();
        
        void setFPS(const double& fps){m_captureFPS=fps;};
        double getFPS(){return m_captureFPS;};
        
        std::string getSourceInfo();
        
        // thread management
        void start();
        void stop();
        void operator()();
        
    private:
        
        volatile bool m_stopped;
        bool m_newFrame;

        std::vector<cv::Mat> m_images;
        
        bool m_processing;
        
        //-- OpenCV
        CVSourceNode::Ptr m_sourceNode;
        CVProcessNode::Ptr m_processNode;
        
        // fetch next frame, depending on current sourceNode
        cv::Mat grabNextFrame();

        //desired capturing / seconds  -> used to time threadmanagment
        double m_captureFPS;
        
        double m_lastGrabTime;
        double m_lastProcessTime;
        
        boost::thread m_thread;
        boost::mutex m_mutex;
        boost::condition_variable m_conditionVar;
        
    };
    
} // namespace kinski
#endif // CHTHREAD_H
