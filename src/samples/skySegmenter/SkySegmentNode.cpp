//
//  SkySegmentNode.cpp
//  kinskiGL
//
//  Created by Fabian Schmidt on 6/25/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "SkySegmentNode.h"

using namespace cv;
using namespace kinski;

SkySegmentNode::SkySegmentNode():
m_cannyLow(_Property<uint8_t>::create("Canny Low", 71)),
m_cannyHigh(_Property<uint8_t>::create("Cany High", 208)),
m_morphKernSize(_Property<cv::Size>::create("Morph kernel size", Size(9,9))),
m_threshVal(_Property<uint8_t>::create("Intesity threshold", 140)),
m_minPathLength(_Property<int>::create("Minimal path length", 200))
{
    registerProperty(m_cannyLow);
    registerProperty(m_cannyHigh);
    registerProperty(m_morphKernSize);
    registerProperty(m_threshVal);
    registerProperty(m_minPathLength);
}

SkySegmentNode::~SkySegmentNode()
{

}

Mat SkySegmentNode::doProcessing(const Mat &img)
{
    Mat scaledImg, thresImg, edgeImg, outMat;
    cv::bitwise_not(img, outMat);
    
    
    return outMat;
}