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
public:
    
    ColorHistNode();
    virtual ~ColorHistNode();
    
    cv::Mat doProcessing(const cv::Mat &img);
    
    void extractHist(const cv::Mat &roi);
    
private:

    cv::Mat m_colorHist;
};

