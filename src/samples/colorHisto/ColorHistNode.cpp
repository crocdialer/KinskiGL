//
//  SkySegmentNode.cpp
//  gl
//
//  Created by Fabian Schmidt on 6/25/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//
#include "core/Logger.h"
#include "ColorHistNode.h"

using namespace cv;
using namespace kinski;

ColorHistNode::ColorHistNode():
m_histExtraction(Property_<bool>::create("Extract Histogram", false))
{
    set_name("ColorHistogram");
    
    m_histSize[0] = 12;
    m_hueRanges[0] = 0;
    m_hueRanges[1] = 180;
    channels[0] = 0;
    m_ranges = m_hueRanges;
    m_roiWidth = 50;
    
    // properties
    registerProperty(m_histExtraction);
    
    // testhist
    try
    {
        Mat testPatch = imread("greenCut.png");
        Mat patchHSV;
        cvtColor(testPatch, patchHSV, CV_BGR2HSV);
        
        m_colorHist = extractHueHistogram(patchHSV);
        m_histImage = createHistImage();
    } catch (std::exception &e)
    {
        LOG_WARNING<<"ColorHistNode: could not initialize histogram: " << e.what();
    }

}

ColorHistNode::~ColorHistNode()
{
    
}

string ColorHistNode::getDescription()
{
    return "ColorHistNode - Histogram Backprojection";
}

vector<Mat> ColorHistNode::doProcessing(const Mat &img)
{
    // check for correct type (3 channel 8U)
    //TODO: implement check here
    //assert(img.type() == CV_8UC3);
    
    Mat hsvImg, backProj, roiImg, scaledImg;
    
    int maxWidth = 400;
    float scale = (float)maxWidth / img.cols;
    
    // scale down input image
    cv::resize(img, scaledImg, Size(0,0), scale, scale);
    
    
    //convert colorspace to hsv
    cvtColor(scaledImg, hsvImg, CV_BGR2HSV);
    
    if(*m_histExtraction || m_colorHist.empty())
    {
        Point centerPoint = Point(hsvImg.cols / 2, hsvImg.rows/ 2);
        Rect centerRect = Rect(centerPoint - Point(m_roiWidth/2, m_roiWidth/2),
                               centerPoint + Point(m_roiWidth/2, m_roiWidth/2));
        
        roiImg = hsvImg(centerRect);
        
        m_colorHist = extractHueHistogram(roiImg);
        m_histImage = createHistImage();
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
    
    vector<Mat> outMats;
    outMats.push_back(m_histImage);
    outMats.push_back(backProj);
    
    return outMats;
}

Mat ColorHistNode::extractHueHistogram(const cv::Mat &hsvImg)
{
    Mat outMat, mask;
    
    // extract 1 dimensional histogram
    calcHist(&hsvImg, 1, channels, mask, outMat, 1, m_histSize, &m_ranges);
    
    cv::normalize(outMat, outMat, 0, 255, NORM_MINMAX);
    
    return outMat;
    
}

Mat ColorHistNode::createHistImage()
{
    Mat histImg(300, 400, CV_8UC3);//m_histSize[0]
    
    histImg = Scalar::all(0);
    int binW = histImg.cols / m_histSize[0];
    Mat buf(1, m_histSize[0], CV_8UC3);
    
    for( int i = 0; i < m_histSize[0]; i++ )
        buf.at<Vec3b>(i) = Vec3b(saturate_cast<uchar>(i*180./m_histSize[0]), 255, 255);
    
    cvtColor(buf, buf, CV_HSV2BGR);
    
    for( int i = 0; i < m_histSize[0]; i++ )
    {
        int val = saturate_cast<int>(m_colorHist.at<float>(i)*histImg.rows/255);
        rectangle( histImg, Point(i*binW,histImg.rows),
                  Point((i+1)*binW,histImg.rows - val),
                  Scalar(buf.at<Vec3b>(i)), -1, 8 );
    }
    
    return histImg;
}

void ColorHistNode::setHistExtraction(bool b)
{
    *m_histExtraction = b;
}

bool ColorHistNode::hasHistExtraction()
{
    return *m_histExtraction;
}