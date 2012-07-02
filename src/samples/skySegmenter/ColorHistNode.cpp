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

}

ColorHistNode::~ColorHistNode()
{

}

Mat ColorHistNode::doProcessing(const Mat &img)
{
    Mat outMat = img.clone(), centerRoi;
    
    {
        int rectWidth = 50;
        Point centerPoint = Point(img.cols / 2, img.rows/ 2);
        Rect centerRect = Rect(centerPoint - Point(rectWidth, rectWidth),
                               centerPoint + Point(rectWidth, rectWidth));
        
        centerRoi = outMat(centerRect);
    }
    
    if(m_colorHist.empty())
        extractHist(img);
    
    // back projection
    //calcBackProject(<#InputArrayOfArrays images#>, <#const vector<int> &channels#>, <#InputArray hist#>, <#OutputArray dst#>, <#const vector<float> &ranges#>, <#double scale#>)
    
    return outMat;
}

void ColorHistNode::extractHist(const cv::Mat &img)
{
    Mat hsvImg, outMat;
    
    cvtColor(img, hsvImg, CV_BGR2HSV);
    
    int hsize = 16;
    float hranges[] = {0,180};
    const float* phranges = hranges;
    
    calcHist(&hsvImg, 1, 0, Mat(), m_colorHist, 1, &hsize, &phranges);
    
}