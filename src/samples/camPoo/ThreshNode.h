//
//  ThreshNode.h
//  kinskiGL
//
//  Created by Fabian on 7/20/12.
//
//

#ifndef kinskiGL_ThreshNode_h
#define kinskiGL_ThreshNode_h

#include "kinskiCV/CVNode.h"

class ThreshNode : public kinski::CVProcessNode
{
public:
    ThreshNode(const int theThresh = 50):
    m_thresh(theThresh){}
    
    cv::Mat doProcessing(const cv::Mat &img)
    {
        cv::Mat grayImg, threshImg, outMat;
        cv::cvtColor(img, grayImg, CV_BGR2GRAY);
        uint64_t mode = CV_THRESH_BINARY;
        if(m_thresh < 0) mode |= CV_THRESH_OTSU;
        threshold(grayImg, threshImg, m_thresh, 255, mode);
        outMat = threshImg;
        cv::applyColorMap(outMat, outMat, cv::COLORMAP_BONE);
        
        return outMat;
    };
    
private:
    int m_thresh;
};


#endif
