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
m_cannyLow(_Property<uint32_t>::create("Canny Low", 71)),
m_cannyHigh(_Property<uint32_t>::create("Cany High", 208)),
m_morphKernSize(_Property<uint32_t>::create("Morph kernel size", 9)),
m_threshVal(_Property<uint32_t>::create("Intesity threshold", 140)),
m_minPathLength(_Property<uint32_t>::create("Minimal path length", 200))
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
    Mat scaledImg, grayImg, threshImg,
    edgeImg, skyMask, outMat;
    
    float scale = 0.5f;
    
    // scale down input image
    cv::resize(img, scaledImg, Size(0,0), scale, scale);
    
    cv::cvtColor(scaledImg, grayImg, cv::COLOR_BGR2GRAY);
    
    // compute dynamic and static thresholds
    Mat tmpThresh;
    cv::threshold(grayImg, threshImg, m_threshVal->val(), 255,
                  THRESH_BINARY_INV | THRESH_OTSU);
    cv::threshold(grayImg, tmpThresh, m_threshVal->val(), 255,
                  THRESH_BINARY_INV);
    
    threshImg &= tmpThresh;
    
    // edge detection
    cv::Canny(grayImg, edgeImg, m_cannyLow->val(), m_cannyHigh->val());
    
    Mat workImg;
    Size kernSize = getValidKernSize(m_morphKernSize->val());
    cv::dilate(edgeImg,
               workImg,
               cv::getStructuringElement(MORPH_ELLIPSE, kernSize ),
               Point(-1,-1),
               2);
    
    morphologyEx(threshImg,
                 threshImg,
                 MORPH_CLOSE,
                 cv::getStructuringElement(MORPH_ELLIPSE,kernSize),
                 Point(-1,-1),
                 2);
    
    workImg |= threshImg;
    
    bitwise_not(workImg, skyMask);
    
    //
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    vector<vector<Point> > contours0;
    
    Mat contourImg = skyMask.clone();
    findContours( contourImg, contours0, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
    
    
    cv::cvtColor(skyMask, skyMask, CV_GRAY2BGR);
    
    outMat = scaledImg & skyMask;
    drawContours(outMat, contours0, -1, Scalar(128, 255, 255), 3);
    
    return outMat;
}

cv::Mat SkySegmentNode::combineImages(const cv::Mat &img1, const cv::Mat &img2)
{
    Mat outMat = img1.clone();
    
    Mat tmpLeft, tmpRight;
    
    Rect left(0, 0, img1.cols / 2, img1.rows);
    Rect right(img1.cols / 2 , 0, img1.cols / 2, img1.rows);
    
    outMat(right) = img2(right);
    
    return outMat;
}

cv::Size SkySegmentNode::getValidKernSize(const uint32_t sz)
{
    int dim = sz;
    
    if(! (dim%2))
        dim--;
    dim = std::max(3, dim);
    
    return Size(dim, dim);
}
