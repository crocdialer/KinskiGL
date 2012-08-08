//
//  KeyPointNode.h
//  kinskiGL
//
//  Created by Fabian on 8/8/12.
//
//

#ifndef __kinskiGL__FeatureDetector__
#define __kinskiGL__FeatureDetector__

#include "kinskiCV/CVNode.h"

namespace kinski
{
    class KeyPointNode : public CVProcessNode
    {
    public:
        
        KeyPointNode();
        std::string getDescription();
        std::vector<cv::Mat> doProcessing(const cv::Mat &img);
        
    private:
        
        cv::Ptr<cv::FeatureDetector> m_featureDetect;
        cv::Ptr<cv::FREAK> m_featureExtract;
        
    };

}
#endif /* defined(__kinskiGL__FeatureDetector__) */
