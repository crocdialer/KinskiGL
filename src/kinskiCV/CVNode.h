//
//  CVNodes.h
//  kinskiCV
//
//  Created by Fabian Schmidt on 6/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#pragma once

#include "opencv2/opencv.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/thread.hpp"

#include "kinskiCore/Component.h"

namespace kinski{
    
class CVNode : public Component
{
public:
    typedef boost::shared_ptr<CVNode> Ptr;
    virtual std::string getName() = 0;
    virtual std::string getDescription() = 0;
};
    

class CVSourceNode : public CVNode
{
public:
    typedef boost::shared_ptr<CVSourceNode> Ptr;
    
    // inherited from CVNode
    virtual std::string getName(){return "Instance of CVSourceNode";};
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
    
    std::string getName();
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

class BadInputSourceException : public std::runtime_error
{
public:
    BadInputSourceException(const std::string &msg):
    std::runtime_error(std::string("BadInputSourceException: ") + msg)
    {}
    BadInputSourceException(const CVSourceNode::Ptr srcPtr=CVSourceNode::Ptr()):
    std::runtime_error(std::string("BadInputSourceException: ")
                       + (srcPtr ? srcPtr->getName() : std::string("Null")))
    {}
};
    
class CVProcessNode : public CVNode
{
public:
    typedef boost::shared_ptr<CVProcessNode> Ptr;
    
    // inherited from INode
    virtual std::string getName(){return "Instance of CVProcessNode";};
    virtual std::string getDescription(){return "Generic processing node";};
    
    virtual cv::Mat doProcessing(const cv::Mat &img) = 0;
    
};
    
class CVCaptureNode : public CVSourceNode
{
public:
    typedef boost::shared_ptr<CVCaptureNode> Ptr;
    
    CVCaptureNode(const int camId = 0);
    CVCaptureNode(const std::string &movieFile);
    virtual ~CVCaptureNode();
    
    virtual std::string getName();
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

}// namespace kinski