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
        
        KeyPointNode(const cv::Mat &refImage = cv::Mat());
        std::string getDescription();
        
        std::vector<cv::Mat> doProcessing(const cv::Mat &img);
        
        void setReferenceImage(const cv::Mat &theImg);
        
    private:
        
        cv::Ptr<cv::FeatureDetector> m_featureDetect;
        cv::Ptr<cv::DescriptorExtractor> m_featureExtract;
        cv::Ptr<cv::DescriptorMatcher> m_matcher;
        
        std::vector<cv::KeyPoint> m_trainKeypoints;
        cv::Mat m_referenceImage, m_trainDescriptors;
        
        _RangedProperty<uint32_t>::Ptr m_maxFeatureDist;
        
        void matches2points(const std::vector<cv::KeyPoint>& train,
                       const std::vector<cv::KeyPoint>& query,
                       const std::vector<cv::DMatch>& matches,
                       std::vector<cv::Point2f>& pts_train,
                       std::vector<cv::Point2f>& pts_query);
    };

}
#endif /* defined(__kinskiGL__FeatureDetector__) */
