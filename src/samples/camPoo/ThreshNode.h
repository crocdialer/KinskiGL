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

namespace kinski
{
class ThreshNode : public kinski::CVProcessNode
{
public:
    ThreshNode(const int theThresh = 50):
    m_thresh(theThresh){}
    
    std::vector<cv::Mat> doProcessing(const cv::Mat &img)
    {
        std::vector<cv::Mat> outMats;
        
        cv::Mat grayImg, threshImg;
        cv::cvtColor(img, grayImg, CV_BGR2GRAY);
        uint64_t mode = CV_THRESH_BINARY;
        if(m_thresh < 0) mode |= CV_THRESH_OTSU;
        threshold(grayImg, threshImg, m_thresh, 255, mode);

        cv::applyColorMap(threshImg, threshImg, cv::COLORMAP_BONE);
        
        outMats.push_back(img);
        outMats.push_back(threshImg);
        
        return outMats;
    };
    
private:
    int m_thresh;
};

}
#endif
