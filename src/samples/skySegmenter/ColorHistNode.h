//
//  SkySegmentNode.h
//  kinskiGL
//
//  Created by Fabian Schmidt on 6/25/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#pragma once

#include "kinskiCV/CVNode.h"

class ColorHistNode : public kinski::CVProcessNode
{
private:
    
    // histogram related variables
    
    /*!
     * number of histogram bins
     */
    int m_histSize[1];
    
    float m_hueRanges[2];
    const float *m_ranges;
    int channels[1];
    
    cv::Mat m_colorHist;
    
public:
    
    ColorHistNode();
    virtual ~ColorHistNode();
    
    cv::Mat doProcessing(const cv::Mat &img);
    
    cv::Mat extractHist(const cv::Mat &roi);
    

};

