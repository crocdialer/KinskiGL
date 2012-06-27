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
    
    cv::Mat doProcessing(const cv::Mat &img);
    
private:
    kinski::_Property<uint8_t>::Ptr m_cannyLow;
    kinski::_Property<uint8_t>::Ptr m_cannyHigh;
    kinski::_Property<cv::Size>::Ptr m_morphKernSize;
    kinski::_Property<uint8_t>::Ptr m_threshVal;
    kinski::_Property<int>::Ptr m_minPathLength;
    
};

