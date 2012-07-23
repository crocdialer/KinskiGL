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
    m_histSize[0] = 12;
    
    m_hueRanges[0] = 0; 
    m_hueRanges[1] = 180;
    
    channels[0] = 0;
    
    m_ranges = m_hueRanges;
    
    m_roiWidth = 50;
    
    // testhist
    Mat testPatch = imread("/Users/Fabian/dev/testGround/python/cvScope/scopeFootage/greenCut.png");
    Mat patchHSV;
    cvtColor(testPatch, patchHSV, CV_BGR2HSV);
    
    m_colorHist = extractHueHistogram(patchHSV);
    
}

ColorHistNode::~ColorHistNode()
{
    
}

Mat ColorHistNode::doProcessing(const Mat &img)
{
    flush();
    
    // check for correct type (3 channel 8U)
    //TODO: implement check here
    assert(img.type() == CV_8UC3);
    
    Mat outMat, hsvImg, backProj, roiImg, scaledImg;
    
    int maxWidth = 800;
    float scale = (float)maxWidth / img.cols;
    
    // scale down input image
    cv::resize(img, scaledImg, Size(0,0), scale, scale);
    
    
    //convert colorspace to hsv
    cvtColor(scaledImg, hsvImg, CV_BGR2HSV);
    
    if(m_colorHist.empty())
    {
        Point centerPoint = Point(img.cols / 2, img.rows/ 2);
        Rect centerRect = Rect(centerPoint - Point(m_roiWidth/2, m_roiWidth/2),
                               centerPoint + Point(m_roiWidth/2, m_roiWidth/2));
        
        roiImg = hsvImg(centerRect);
        
        m_colorHist = extractHueHistogram(roiImg);
    }
    
    // back projection
    calcBackProject(&hsvImg, 1, channels, m_colorHist, backProj, &m_ranges);
    
    //threshold(backProj, backProj, 25, 255, CV_THRESH_BINARY);
    
    Mat kernel = cv::getStructuringElement(MORPH_ELLIPSE,Size(5, 5));
    
    // open + DILATE
    {
    morphologyEx(backProj,
                 backProj,
                 MORPH_OPEN,
                 kernel,
                 Point(-1,-1),
                 2);
    cv::dilate(backProj,
               backProj,
               kernel,
               Point(-1,-1),
               2);
    }
    
    blur(backProj, backProj, Size(5, 5));

//    Mat tmpProj, tmpSat;
//    // calculate mixture image
//    backProj.convertTo(tmpProj, CV_32F, 1.0 / 255.0);
//    
//    //return tmpProj;
//    
//    //split hsv image
//    vector<Mat> hsvSplit;
//    cv::split(hsvImg, hsvSplit);
//    hsvSplit[1].convertTo(tmpSat, CV_32F);
//    
//    multiply(tmpSat, tmpProj, tmpSat);
//    
//    tmpSat.convertTo(hsvSplit[1], CV_8U);
//    
//    merge(hsvSplit, outMat);
//    cvtColor(outMat, outMat, CV_HSV2BGR);
//    cv::resize(outMat, outMat, Size(0,0), 1.f / scale, 1.f / scale);
    
    //cvtColor(backProj, outMat, CV_GRAY2BGR);
    
    addImage(img);
    addImage(backProj);
    addImage(outMat);
    
    return outMat;
}

Mat ColorHistNode::extractHueHistogram(const cv::Mat &hsvImg)
{
    Mat outMat, mask;
    
    // extract 1 dimensional histogram
    calcHist(&hsvImg, 1, channels, mask, outMat, 1, m_histSize, &m_ranges);
    
    cv::normalize(outMat, outMat, 0, 255, NORM_MINMAX);
    
    return outMat;
    
}