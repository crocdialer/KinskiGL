//
//  CVThread.h
//  kinskiCV
//
//  Created by Fabian Schmidt on 6/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef CHTHREAD_H
#define CHTHREAD_H

#include "boost/thread.hpp"
#include "CVNode.h"

namespace kinski
{
    
    typedef std::shared_ptr<class CVThread> CVThreadPtr;
    
    class CVThread : public Component
    {
    public:
        
        typedef std::shared_ptr<CVThread> Ptr;
             
        CVThread();
        virtual ~CVThread();
        
        static Ptr create();
        
        void openImage(const std::string& imgPath);
        
        void streamImageSequence(const std::vector<std::string>& files);
        void streamVideo(const std::string& path2Video, bool loop = false);
        void streamUSBCamera(int camId = 0);
        
        void playPause();
        
        void setImage(const cv::Mat& img);
        bool hasImage();
        void waitForImage();
        std::vector<cv::Mat> getImages();
        
        void setSourceNode(const CVSourceNode::Ptr sn){m_sourceNode = sn;};
        
        inline bool hasProcessing() const {return *m_processing && m_processNode;};
        inline void setProcessing(bool b){*m_processing = b;};
        
        inline void setProcessingNode(const CVProcessNode::Ptr pn){m_processNode = pn;};
        inline CVProcessNode::Ptr getProcessingNode() const {return m_processNode;};
        
        /*!
         *  @return the time in ms of the last execution of
         *  m_sourceNode->getNextImage()
         */
        float getLastGrabTime() const;
        
        /*!
         *  @return the time in ms of the last execution of
         *  m_processNode->doProcessing(...)
         */
        float getLastProcessTime() const;
        
        void setFPS(const double& fps){m_captureFPS=fps;};
        double getFPS() const {return m_captureFPS;};
        
        std::string getSourceInfo();
        
        // thread management
        void start();
        void stop();
        void operator()();
        
        void updateProperty(const Property::ConstPtr &theProperty);
        
    private:
        
        bool m_running;
        Property_<bool>::Ptr m_running_toggle;
        Property_<bool>::Ptr m_processing;
        bool m_newFrame;
        std::vector<cv::Mat> m_images;
        
        //-- OpenCV
        CVSourceNode::Ptr m_sourceNode;
        CVProcessNode::Ptr m_processNode;
        
        // fetch next frame, depending on current sourceNode
        cv::Mat grabNextFrame();

        //desired capturing / seconds  -> used to time threadmanagment
        double m_captureFPS;
        
        float m_lastGrabTime;
        float m_lastProcessTime;
        
        boost::thread m_thread;
        boost::mutex m_mutex;
        boost::condition_variable m_conditionVar;
    };
    
} // namespace kinski
#endif // CHTHREAD_H
