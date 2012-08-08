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
    KeyPointNode::KeyPointNode():
    m_featureDetect(FeatureDetector::create("ORB")),
    m_featureExtract(new FREAK())
    {

    }
    
    string KeyPointNode::getDescription()
    {
        return "KeyPointNode: TODO description";
    }
    
    vector<Mat> KeyPointNode::doProcessing(const Mat &img)
    {
        vector<KeyPoint> keypoints;
        m_featureDetect->detect(img, keypoints);
        
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
}