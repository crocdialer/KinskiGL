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
    m_featureExtract(new FREAK())
    {
        setReferenceImage(refImage);
    }
    
    string KeyPointNode::getDescription()
    {
        return "KeyPointNode: TODO description";
    }
    
    vector<Mat> KeyPointNode::doProcessing(const Mat &img)
    {
        vector<KeyPoint> keypoints;
        Mat descriptors;
        
        m_featureDetect->detect(img, keypoints);
        m_featureExtract->compute(img, keypoints, descriptors);
        
        Mat outImg = img.clone();
        
        vector<KeyPoint>::const_iterator it = keypoints.begin();
        for (; it != keypoints.end(); it++)
        {
            circle(outImg, it->pt, it->size, Scalar(1,0,0));
        }
        
        vector<Mat> outMats;
        outMats.push_back(outImg);
        
        return outMats;
    }
    
    void KeyPointNode::setReferenceImage(const cv::Mat &theImg)
    {
        m_referenceImage = theImg;
        
        vector<KeyPoint> keypoints;
        
        m_featureDetect->detect(m_referenceImage, keypoints);
        m_featureExtract->compute(m_referenceImage, keypoints, m_descriptors);
    }
}