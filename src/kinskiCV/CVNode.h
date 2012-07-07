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

#include "Colormap.h"
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
    
    // inherited from INode
    virtual std::string getName(){return "Instance of CVSourceNode";};
    virtual std::string getDescription(){return "Generic Input-source";};
    
    virtual bool getNextImage(cv::Mat &img) = 0;
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

class CVBufferedSourceNode : public CVSourceNode 
{    
public:
    CVBufferedSourceNode(const CVSourceNode::Ptr srcNode):
    m_sourceNode(srcNode)
    {};
    
private:
    
    boost::thread m_thread;
    boost::mutex m_mutex;
    
    /*!
     * the number of frames the buffer can hold
     */
    uint32_t m_bufferSize;
    
    CVSourceNode::Ptr m_sourceNode;
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
    
class CvCaptureNode : public CVSourceNode
{
public:
    typedef boost::shared_ptr<CvCaptureNode> Ptr;
    
    CvCaptureNode(const int camId);
    CvCaptureNode(const std::string &movieFile);
    virtual ~CvCaptureNode();
    
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

class ThreshNode : public CVProcessNode
{
public:
    ThreshNode(const int theThresh = 50):
    m_thresh(theThresh),
    m_colorMap(Colormap::BONE){};
    
    cv::Mat doProcessing(const cv::Mat &img)
    {
        cv::Mat grayImg, threshImg, outMat;
        cvtColor(img, grayImg, CV_BGR2GRAY);
        uint64_t mode = CV_THRESH_BINARY;
        if(m_thresh < 0) mode |= CV_THRESH_OTSU;
        threshold(grayImg, threshImg, m_thresh, 255, mode);
        outMat = threshImg;
        
        return m_colorMap.apply(outMat);
    };
    
private:
    int m_thresh;
    Colormap m_colorMap;
};
}// namespace kinski