//
//  CVNodes.cpp
//  kinskiGL
//
//  Created by Fabian Schmidt on 6/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "CVNode.h"
#include <boost/format.hpp>

using namespace std;
using namespace cv;

namespace kinski {
    
    CvCaptureNode::CvCaptureNode(const int camId):
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
    
    CvCaptureNode::CvCaptureNode(const std::string &movieFile):
    m_videoSource(movieFile),
    m_loop(false)
    {
        if(!m_capture.open(movieFile))
            throw std::runtime_error("could not open capture");
        
        m_captureFPS = m_capture.get(CV_CAP_PROP_FPS);
        m_numFrames = m_capture.get(CV_CAP_PROP_FRAME_COUNT);
        
        boost::format fmt("movie file '%s'\n# frames: %d\nfps: %.2f\n");
        fmt % movieFile % m_numFrames % m_captureFPS;
        m_description = fmt.str();
    }
    
    CvCaptureNode::~CvCaptureNode(){ m_capture.release();};
    
    std::string CvCaptureNode::getName(){return "CvCaptureNode";};
    std::string CvCaptureNode::getDescription(){return m_description;};
    
    bool CvCaptureNode::getNextImage(cv::Mat &img)
    {
        if(m_loop)
        {
            int currentFrame = m_capture.get(CV_CAP_PROP_POS_FRAMES);
            if(currentFrame > std::max(0,m_numFrames - 2)) 
            {
                //jumpToFrame(0);
                m_capture.release();
                m_capture.open(m_videoSource);
            }
        }
        
        if(!m_capture.grab()) return false;
        
        cv::Mat capFrame;
        m_capture.retrieve(capFrame, 0) ;
        
        // going safe, have a copy of our own of the data
        img = capFrame.clone();
        return true;
    }
    
    int CvCaptureNode::getNumFrames(){return m_numFrames;}
    
    float CvCaptureNode::getFPS(){return m_captureFPS;}
    
    void CvCaptureNode::jumpToFrame(const unsigned int newIndex)
    {
        int clampedIndex = newIndex > m_numFrames ? newIndex : (m_numFrames - 1);
        m_capture.set(CV_CAP_PROP_POS_FRAMES,clampedIndex);
    }
    
    void CvCaptureNode::setLoop(bool b){m_loop = b;};
}

