//
//  KeyPointNode.h
//  gl
//
//  Created by Fabian on 8/8/12.
//
//

#ifndef __gl__KeyPointNode__
#define __gl__KeyPointNode__

#include "cv/CVNode.h"

namespace kinski
{
    class KeyPointNode : public CVProcessNode
    {
    public:
        
        KeyPointNode(const cv::Mat &refImage = cv::Mat());
        std::string getDescription();
        
        std::vector<cv::Mat> doProcessing(const cv::Mat &img);
        
        void setReferenceImage(const cv::Mat &theImg);
        
        // Property::Observer::update() overwritten here
        void update(const Property::Ptr &theProperty);
        
    private:
        
        cv::Ptr<cv::FeatureDetector> m_featureDetect;
        cv::Ptr<cv::DescriptorExtractor> m_featureExtract;
        cv::Ptr<cv::DescriptorMatcher> m_matcher;
        
        std::vector<cv::KeyPoint> m_trainKeypoints;
        cv::UMat m_outImg, m_referenceImage, m_trainDescriptors, m_homography;
        
        cv::KalmanFilter m_kalmanFilter;
        
        RangedProperty<uint32_t>::Ptr m_maxImageWidth;
        RangedProperty<uint32_t>::Ptr m_maxPatchWidth;
        RangedProperty<uint32_t>::Ptr m_maxFeatureDist;
        RangedProperty<uint32_t>::Ptr m_minMatchCount;
        
        
        //Converts matching indices to xy points
        void matches2points(const std::vector<cv::KeyPoint>& train,
                            const std::vector<cv::KeyPoint>& query,
                            const std::vector<cv::DMatch>& matches,
                            std::vector<cv::Point2f>& pts_train,
                            std::vector<cv::Point2f>& pts_query);
    };
}
#endif /* defined(__gl__FeatureDetector__) */
