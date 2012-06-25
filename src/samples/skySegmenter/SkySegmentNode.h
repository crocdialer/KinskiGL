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
};

