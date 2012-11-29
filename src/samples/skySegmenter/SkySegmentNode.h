//
//  SkySegmentNode.h
//  kinskiGL
//
//  Created by Fabian Schmidt on 6/25/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#pragma once

#include "kinskiCV/CVNode.h"

class SkySegmentNode : public kinski::CVProcessNode
{
public:
    
    SkySegmentNode();
    virtual ~SkySegmentNode();
    
    std::vector<cv::Mat> doProcessing(const cv::Mat &img);
    
private:
    
    cv::Mat combineImages(const cv::Mat &img1, const cv::Mat &img2);
    cv::Size getValidKernSize(const uint32_t s);
    
    kinski::RangedProperty<int>::Ptr m_cannyLow;
    kinski::RangedProperty<int>::Ptr m_cannyHigh;
    kinski::RangedProperty<int>::Ptr m_morphKernSize;
    kinski::RangedProperty<int>::Ptr m_threshVal;
    kinski::RangedProperty<int>::Ptr m_minPathLength;
    
};

