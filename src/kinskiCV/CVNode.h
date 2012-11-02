//
//  CVNodes.h
//  kinskiCV
//
//  Created by Fabian Schmidt on 6/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#pragma once

#include "opencv2/opencv.hpp"
#include "boost/thread.hpp"

#include "kinskiCore/Component.h"

namespace kinski{
    
    class CVNode : public Component
    {
    public:
        typedef std::shared_ptr<CVNode> Ptr;
        virtual std::string getDescription() = 0;
    };
    
    
    class CVSourceNode : public CVNode
    {
    public:
        typedef std::shared_ptr<CVSourceNode> Ptr;
        
        // inherited from CVNode
        virtual std::string getDescription(){return "Generic Input-source";};
        
        //virtual float getFramerate() = 0;
        
        virtual bool getNextImage(cv::Mat &img) = 0;
        
        
    };
    
    class CVBufferedSourceNode : public CVSourceNode
    {
    public:
        CVBufferedSourceNode(const CVSourceNode::Ptr srcNode, int bufSize = 20);
        virtual ~CVBufferedSourceNode();
        
        void operator()();
        
        // override
        
        bool getNextImage(cv::Mat &img);
        
        std::string getDescription();
        
    private:
        
        boost::thread m_thread;
        boost::mutex m_mutex;
        boost::condition_variable m_conditionVar;
        
        volatile bool m_running;
        
        /*!
         * the wrapped source node. we simply buffer its output in our thread
         */
        CVSourceNode::Ptr m_sourceNode;
        
        /*!
         * the number of frames the buffer can hold
         */
        uint32_t m_bufferSize;
        
        /*!
         * our buffer
         */
        std::list<cv::Mat> m_imgBuffer;
    };
    
    /*!
     * thrown to indicate a bad input source
     */
    class BadInputSourceException : public Exception
    {
    public:
        BadInputSourceException(const std::string &msg):
        Exception(std::string("BadInputSourceException: ") + msg)
        {}
        BadInputSourceException(const CVSourceNode::Ptr srcPtr):
        Exception(std::string("BadInputSourceException: ")
                  + (srcPtr ? srcPtr->getDescription() : std::string("Null")))
        {}
    };
    
    /*!
     * Basic interface for classes serving as processing Nodes,
     * meaning delegates doing images processing or arbitrary stuff 
     * like communicating results over network or writing images to disk, etc.
     */
    class CVProcessNode : public CVNode
    {
    public:
        typedef std::shared_ptr<CVProcessNode> Ptr;
        
        // inherited from CVNode
        virtual std::string getDescription(){return "Generic processing node";};
        
        virtual std::vector<cv::Mat> doProcessing(const cv::Mat &img) = 0;
    };
    
    /*!
     * CVCombinedProcessNode serves as a wrapper for CVProcessNode s,
     * executing their tasks sequentially and summing up their properties and results.
     */
    class CVCombinedProcessNode : public CVProcessNode
    {
    public:
        typedef std::shared_ptr<CVCombinedProcessNode> Ptr;
        
        // inherited from CVNode
        virtual std::string getDescription();
        
        /*!
         * add a ProcessNode to the processing chain
         */
        void addNode(const CVProcessNode::Ptr &theNode);
        const std::list<CVProcessNode::Ptr>& getNodes(){return m_processNodes;};
        
        std::vector<cv::Mat> doProcessing(const cv::Mat &img);
        
    private:
        std::list<CVProcessNode::Ptr> m_processNodes;
    };
    
    CVCombinedProcessNode::Ptr link(const CVProcessNode::Ptr &one,
                                    const CVProcessNode::Ptr &other);
    
    const CVCombinedProcessNode::Ptr operator<<(const CVProcessNode::Ptr &one,
                                                const CVProcessNode::Ptr &other);
    
    const CVCombinedProcessNode::Ptr operator>>(const CVProcessNode::Ptr &one,
                                                const CVProcessNode::Ptr &other);
    
    
    class CVCaptureNode : public CVSourceNode
    {
    public:
        typedef std::shared_ptr<CVCaptureNode> Ptr;
        
        CVCaptureNode(const int camId = 0);
        CVCaptureNode(const std::string &movieFile);
        virtual ~CVCaptureNode();
        
        virtual std::string getDescription();
        
        bool getNextImage(cv::Mat &img);
        
        // capture interface
        int getNumFrames();
        void jumpToFrame(const int newIndex);
        void setLoop(bool b);
        
        float getFPS();
        
    private:
        cv::VideoCapture m_capture;
        
        std::string m_videoSource;
        std::string m_description;
        
        int m_numFrames;
        float m_captureFPS;
        bool m_loop;
    };
    
    class CVDiskWriterNode : public CVProcessNode
    {
    public:
        
        CVDiskWriterNode(const std::string &theFile = "");
        virtual ~CVDiskWriterNode();
        
        std::string getDescription();
        std::vector<cv::Mat> doProcessing(const cv::Mat &img);
        
        void updateProperty(const Property::Ptr &theProperty);
        
    private:
        Property_<std::string>::Ptr m_videoSrc;
        int m_codec;
        cv::VideoWriter m_videoWriter;
    };
    
}// namespace kinski