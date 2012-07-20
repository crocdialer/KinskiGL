//
//  CVNodes.cpp
//  kinskiCV
//
//  Created by Fabian Schmidt on 6/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "CVNode.h"
#include <boost/format.hpp>

using namespace std;
using namespace cv;

namespace kinski {
    
    CVCaptureNode::CVCaptureNode(const int camId):
    m_numFrames(-1),
    m_loop(false)
    {
        if(!m_capture.open(camId))
            throw std::runtime_error("could not open capture");
        
        m_captureFPS = m_capture.get(CV_CAP_PROP_FPS);
        m_numFrames = m_capture.get(CV_CAP_PROP_FRAME_COUNT);
    
        boost::format fmt("usb camera(%d)\n");
        fmt % camId;
        m_description = fmt.str();
    }
    
    CVCaptureNode::CVCaptureNode(const std::string &movieFile):
    m_videoSource(movieFile),
    m_loop(true)
    {
        if(!m_capture.open(movieFile))
            throw std::runtime_error("could not open capture");
        
        m_captureFPS = m_capture.get(CV_CAP_PROP_FPS);
        m_numFrames = m_capture.get(CV_CAP_PROP_FRAME_COUNT);
        
        boost::format fmt("movie file '%s'\n# frames: %d\nfps: %.2f\n");
        fmt % movieFile % m_numFrames % m_captureFPS;
        m_description = fmt.str();
    }
    
    CVCaptureNode::~CVCaptureNode(){ m_capture.release();};
    
    std::string CVCaptureNode::getName(){return "CVCaptureNode";};
    std::string CVCaptureNode::getDescription(){return m_description;};
    
    bool CVCaptureNode::getNextImage(cv::Mat &img)
    {
        cv::Mat capFrame;
        m_capture >> capFrame;
        
        if(capFrame.empty())
        {
            if(m_loop)
                m_capture.set(CV_CAP_PROP_POS_FRAMES,0);
            else
                return false;
            
        }
    
        // going safe, have a copy of our own of the data
        img = capFrame.clone();
        return true;
    }
    
    int CVCaptureNode::getNumFrames(){return m_numFrames;}
    
    float CVCaptureNode::getFPS(){return m_captureFPS;}
    
    void CVCaptureNode::jumpToFrame(const int newIndex)
    {
        int clampedIndex = std::max(0, newIndex);
        clampedIndex = std::min(clampedIndex ,m_numFrames - 1);
        
        m_capture.set(CV_CAP_PROP_POS_FRAMES,clampedIndex);
    }
    
    void CVCaptureNode::setLoop(bool b){m_loop = b;};
}

