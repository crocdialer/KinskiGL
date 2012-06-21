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
    m_numFrames(-1)
    {
        m_capture.open(camId);
        
        m_captureFPS = m_capture.get(CV_CAP_PROP_FPS);
        m_numFrames = m_capture.get(CV_CAP_PROP_FRAME_COUNT);
        
        stringstream sstream;
        sstream<<"usb camera ("<<camId<<")";
        m_description = sstream.str();
        
        boost::format fmt("usb camera(%d)\n");
        fmt % camId;
        m_description = fmt.str();
    }
    
    CvCaptureNode::CvCaptureNode(const std::string &movieFile)
    {
        m_capture.open(movieFile);
        
        m_captureFPS = m_capture.get(CV_CAP_PROP_FPS);
        m_numFrames = m_capture.get(CV_CAP_PROP_FRAME_COUNT);
        
        boost::format fmt("movie file '%s'\n# frames: %d\nfps: %.2f\n");
        fmt % movieFile % m_numFrames % m_captureFPS;
        m_description = fmt.str();
    }
    
    CvCaptureNode::~CvCaptureNode(){ m_capture.release();};
    
    std::string CvCaptureNode::getName(){return "CvCaptureNode";};
    std::string CvCaptureNode::getDescription(){return m_description;};
    
    bool CvCaptureNode::hasImage(){ return m_capture.isOpened();};
    
    cv::Mat CvCaptureNode::getNextImage()
    {
        cv::Mat capFrame;
        m_capture.grab();
        m_capture.retrieve(capFrame, 0) ;
        
        // going safe, have a copy of our own of the data
        capFrame = capFrame.clone();
        return capFrame;
    }
    
    int CvCaptureNode::getNumFrames(){return m_numFrames;}
    
    float CvCaptureNode::getFPS(){return m_captureFPS;}
}

