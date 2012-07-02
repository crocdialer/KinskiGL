//
//  SkySegmentNode.cpp
//  kinskiGL
//
//  Created by Fabian Schmidt on 6/25/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "ColorHistNode.h"

using namespace cv;
using namespace kinski;

ColorHistNode::ColorHistNode()
{
    m_histSize[0] = 16;
    
    m_hueRanges[0] = 0; 
    m_hueRanges[1] = 180;
    
    channels[0] = 0;
    
    m_ranges = m_hueRanges;
    
    
}

ColorHistNode::~ColorHistNode()
{
    
}

Mat ColorHistNode::doProcessing(const Mat &img)
{
    // check for correct type (3 channel 8U)
    //TODO: implement check here
    assert(img.channels() == 3 &&
           img.type() & CV_8U);
    
    Mat outMat, hsvImg, backProj, roiImg;
    
    //convert colorspace to hsv
    cvtColor(img, hsvImg, CV_BGR2HSV);
    
    if(m_colorHist.empty())
    {
        int rectWidth = 50;
        Point centerPoint = Point(img.cols / 2, img.rows/ 2);
        Rect centerRect = Rect(centerPoint - Point(rectWidth, rectWidth),
                               centerPoint + Point(rectWidth, rectWidth));
        
        roiImg = hsvImg(centerRect);
        
        m_colorHist = extractHist(roiImg);
    }
    
    // back projection
    calcBackProject(&img, 1, channels, m_colorHist, backProj, &m_ranges);
    
    return outMat;
}

Mat ColorHistNode::extractHist(const cv::Mat &hsvImg)
{
    Mat outMat;
    
    // extract 1 dimensional histogram
    calcHist(&hsvImg, 1, channels, Mat(), outMat, 1, m_histSize, &m_ranges);
    
    cv::normalize(outMat, outMat, 0, 255, NORM_MINMAX);
    
    return outMat;
    
}