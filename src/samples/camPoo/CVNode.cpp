//
//  CVNodes.cpp
//  kinskiGL
//
//  Created by Fabian Schmidt on 6/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "CVNode.h"
#include <sstream>

using namespace std;
using namespace cv;

namespace kinski {
    
    CvCaptureNode::CvCaptureNode(const int camId)
    {
        m_capture.open(camId);
        stringstream sstream;
        sstream<<"usb camera ("<<camId<<")";
        m_description = sstream.str();
    }
    
    CvCaptureNode::CvCaptureNode(const std::string &movieFile)
    {
        m_capture.open(movieFile);
        stringstream sstream;
        sstream<<"movie file '"<<movieFile<<"'";
        m_description = sstream.str();
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
}

