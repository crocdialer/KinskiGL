//
//  SkySegmentNode.cpp
//  kinskiGL
//
//  Created by Fabian Schmidt on 6/25/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "SkySegmentNode.h"

using namespace cv;
SkySegmentNode::SkySegmentNode()
{

}

SkySegmentNode::~SkySegmentNode()
{

}

Mat SkySegmentNode::doProcessing(const Mat &img)
{
    Mat outMat;
    cv::bitwise_not(img, outMat);
    
    return outMat;
}