//
//  KeyPointNode.cpp
//  kinskiGL
//
//  Created by Fabian on 8/8/12.
//
//

#include "KeyPointNode.h"
#include "boost/timer/timer.hpp"

using namespace std;
using namespace cv;
using namespace boost::timer;

namespace kinski
{
    KeyPointNode::KeyPointNode(const Mat &refImage):
    m_featureDetect(FeatureDetector::create("ORB")),
    m_featureExtract(DescriptorExtractor::create("ORB")),
    m_matcher(new BFMatcher(NORM_HAMMING)),
    m_maxFeatureDist(_RangedProperty<uint32_t>::create("Max feature distance",
                                                       35, 0, 150))
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
        vector<DMatch> matches;
        Mat descriptors_scene;
        
        {/*auto_cpu_timer t;*/ m_featureDetect->detect(img, keypoints);}
        {/*auto_cpu_timer t;*/ m_featureExtract->compute(img, keypoints, descriptors_scene);}
        {/*auto_cpu_timer t;*/ m_matcher->match(descriptors_scene, m_trainDescriptors, matches);}
        
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
        vector< DMatch > good_matches;
        
        for( int i = 0; i < matches.size(); i++ )
        {
            if( matches[i].distance < min((double)m_maxFeatureDist->val(),
                                               2 * min_dist))
                good_matches.push_back( matches[i]);
        }
        
        // a minimum size of matches is needed for calculation of a homography
        if(good_matches.size() > 5)
        {
            vector<Point2f> pts_train, pts_query;
            matches2points(m_trainKeypoints, keypoints, good_matches, pts_train,
                           pts_query);
            m_homography = findHomography(pts_train, pts_query, CV_RANSAC);

            //TODO: nicer criteria here, remove magic number            
            if(good_matches.size() > 16)
                printf("Detected!! -> %ld\n", good_matches.size());
            
        }

        // draw good_matches
        for (int i=0; i<good_matches.size(); i++)
        {
            const DMatch &m = good_matches[i];

            KeyPoint &kp = keypoints[m.queryIdx];
            circle(outImg, kp.pt, kp.size, Scalar(0,255,0));
        }
        
        // draw outline of object
        if(!m_homography.empty())
        {
            vector<Point2f> objPts, scenePts;
            objPts.push_back(Point2f(0,0));
            objPts.push_back(Point2f(0, m_referenceImage.rows));
            objPts.push_back(Point2f(m_referenceImage.cols, m_referenceImage.rows));
            objPts.push_back(Point2f(m_referenceImage.cols,0));
            
            perspectiveTransform(objPts, scenePts, m_homography);
            
            line(outImg, scenePts[0], scenePts[1], Scalar(0, 255, 0));
            line(outImg, scenePts[1], scenePts[2], Scalar(0, 255, 0));
            line(outImg, scenePts[2], scenePts[3], Scalar(0, 255, 0));
            line(outImg, scenePts[3], scenePts[0], Scalar(0, 255, 0));
        }
        vector<Mat> outMats;
        outMats.push_back(m_referenceImage);
        outMats.push_back(outImg);
        
        return outMats;
    }
    
    void KeyPointNode::setReferenceImage(const Mat &theImg)
    {
        m_referenceImage = theImg;
        
        GaussianBlur(theImg, m_referenceImage, Size(5, 5), 1.5);

        // scale down if necessary (ORB did not properly manage large ref-images)
        float scale = 640.f / m_referenceImage.cols;
        scale = min(scale, 1.f);
        resize(m_referenceImage, m_referenceImage, Size(), scale, scale);
        
        m_trainKeypoints.clear();
        
        m_featureDetect->detect(m_referenceImage, m_trainKeypoints);
        m_featureExtract->compute(m_referenceImage, m_trainKeypoints,
                                  m_trainDescriptors);
    }
    
    void KeyPointNode::matches2points(const vector<KeyPoint>& train,
                                      const vector<KeyPoint>& query,
                                      const vector<DMatch>& matches,
                                      vector<Point2f>& pts_train,
                                      vector<Point2f>& pts_query)
    {
        pts_train.clear();
        pts_query.clear();
        pts_train.reserve(matches.size());
        pts_query.reserve(matches.size());
        
        size_t i = 0;
        for (; i < matches.size(); i++)
        {
            const DMatch &dmatch = matches[i];
            pts_query.push_back(query[dmatch.queryIdx].pt);
            pts_train.push_back(train[dmatch.trainIdx].pt);
        }
    }
}