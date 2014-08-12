//
//  ThreshNode.h
//  gl
//
//  Created by Fabian on 7/20/12.
//
//

#ifndef gl_ThreshNode_h
#define gl_ThreshNode_h

#include "cv/CVNode.h"

namespace kinski
{
class ThreshNode : public kinski::CVProcessNode
{
public:
    ThreshNode(const int theThresh = 50):
    m_thresh(RangedProperty<int>::create("Thresh", theThresh, -1, 255))
    {
        set_name("ThreshNode");
        registerProperty(m_thresh);
    }
    
    std::vector<cv::Mat> doProcessing(const cv::Mat &img)
    {
        cv::Mat grayImg = img, threshImg;
        if(img.cols > 512) cv::resize(img, grayImg, cv::Size(512, 512));
        if(img.type() == CV_8UC3) cv::cvtColor(grayImg, grayImg, CV_BGR2GRAY);
        uint64_t mode = CV_THRESH_BINARY;
        if(*m_thresh < 0) mode |= CV_THRESH_OTSU;
        threshold(grayImg, threshImg, *m_thresh, 255, mode);
        cv::applyColorMap(threshImg, threshImg, cv::COLORMAP_BONE);
        std::vector<cv::Mat> outMats;
        outMats.push_back(threshImg);
        return outMats;
    };
    
private:
    RangedProperty<int>::Ptr m_thresh;
};

}
#endif
