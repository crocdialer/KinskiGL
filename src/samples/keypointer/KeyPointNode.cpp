//
//  KeyPointNode.cpp
//  kinskiGL
//
//  Created by Fabian on 8/8/12.
//
//

#include "KeyPointNode.h"

using namespace std;
using namespace cv;

namespace kinski
{
    KeyPointNode::KeyPointNode(const cv::Mat &refImage):
    m_featureDetect(FeatureDetector::create("ORB")),
    m_featureExtract(DescriptorExtractor::create("ORB")),
    m_matcher(new BFMatcher(NORM_HAMMING)),
    m_maxFeatureDist(_RangedProperty<uint32_t>::create("Max feature distance",
                                                       30, 0, 150))
    {
        registerProperty(m_maxFeatureDist);
        setReferenceImage(refImage);
    }
    
    string KeyPointNode::getDescription()
    {
        return "KeyPointNode: TODO description";
    }
    
    vector<Mat> KeyPointNode::doProcessing(const Mat &img)
    {
        vector<KeyPoint> keypoints;
        vector<cv::DMatch> matches;
        Mat descriptors_scene;
        
        m_featureDetect->detect(img, keypoints);
        m_featureExtract->compute(img, keypoints, descriptors_scene);
        m_matcher->match(descriptors_scene, m_trainDescriptors, matches);
        
        Mat outImg = img.clone();
        
        //-- Quick calculation of max and min distances between keypoints
        double max_dist = 0; double min_dist = 200;
        for( int i = 0; i < matches.size(); i++ )
        {
            double dist = matches[i].distance;
            if( dist < min_dist ) min_dist = dist;
            if( dist > max_dist ) max_dist = dist;
        }
        
        //-- Leave only "good" matches (i.e. whose distance is less than x * min_dist )
        std::vector< DMatch > good_matches;
        
        for( int i = 0; i < matches.size(); i++ )
        {
            if( matches[i].distance < std::min((double)m_maxFeatureDist->val(),
                                               2 * min_dist))
                good_matches.push_back( matches[i]);
        }
        
        // draw good_matches
        for (int i=0; i<good_matches.size(); i++)
        {
            const DMatch &m = good_matches[i];

            KeyPoint &kp = keypoints[m.queryIdx];
            circle(outImg, kp.pt, kp.size, Scalar(0,255,0));
        }
        
        
//        printf("close matches: %ld (%.2f)\n",good_matches.size(),
//               100 * good_matches.size() / (float) matches.size());
        
        
        vector<Mat> outMats;
        outMats.push_back(m_referenceImage);
        outMats.push_back(outImg);
        
        return outMats;
    }
    
    void KeyPointNode::setReferenceImage(const cv::Mat &theImg)
    {
        m_referenceImage = theImg;
        
        
        GaussianBlur(theImg, m_referenceImage, Size(5, 5), 1.5);
        float scale = 640.f / m_referenceImage.cols;
        resize(m_referenceImage, m_referenceImage, Size(), scale, scale);
        
        vector<KeyPoint> keypoints;
        
        m_featureDetect->detect(m_referenceImage, keypoints);
        m_featureExtract->compute(m_referenceImage, keypoints, m_trainDescriptors);
    }
}