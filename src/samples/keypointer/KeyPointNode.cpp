//
//  KeyPointNode.cpp
//  gl
//
//  Created by Fabian on 8/8/12.
//
//

#include "KeyPointNode.h"
#include "boost/timer/timer.hpp"
#include "core/Logger.h"

using namespace std;
using namespace cv;
using namespace boost::timer;

namespace kinski
{
    KeyPointNode::KeyPointNode(const Mat &refImage):
    m_featureDetect(ORB::create(250, 1.2, 10)),//FeatureDetector::create("ORB")),
    m_featureExtract(m_featureDetect),
    m_matcher(new BFMatcher(NORM_HAMMING2)),
    m_maxImageWidth(RangedProperty<uint32_t>::create("Max image width",
                                                      1024, 100, 1920)),
    m_maxPatchWidth(RangedProperty<uint32_t>::create("Max patch width",
                                                      480, 50, 1024)),
    m_maxFeatureDist(RangedProperty<uint32_t>::create("Max feature distance",
                                                       110, 0, 150)),
    m_minMatchCount(RangedProperty<uint32_t>::create("Minimum match-count",
                                                                      8, 4, 64))
    {
        registerProperty(m_maxFeatureDist);
        registerProperty(m_maxImageWidth);
        registerProperty(m_maxPatchWidth);
        registerProperty(m_minMatchCount);
        
        setReferenceImage(refImage);
        
        //m_kalmanFilter = KalmanFilter(<#int dynamParams#>, <#int measureParams#>)
    }
    
    string KeyPointNode::getDescription()
    {
        return "\n--------- KeyPointNode ---------\n"
        "Object detection and pose estimation using stable image features";
    }
    
    vector<Mat> KeyPointNode::doProcessing(const Mat &img)
    {
        vector<KeyPoint> keypoints;
        vector<DMatch> matches;
        UMat descriptors_scene, downSized, out_img;
        
        float scale = (float)*m_maxImageWidth / img.cols;
        scale = min(scale, 1.f);
        resize(img.getUMat(ACCESS_READ), downSized, Size(), scale, scale);
        
        m_featureDetect->detect(downSized, keypoints);
        m_featureExtract->compute(downSized, keypoints,descriptors_scene);
        m_matcher->match(descriptors_scene, m_trainDescriptors, matches);
        
        m_outImg = img.getUMat(ACCESS_RW);
        
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
            if( matches[i].distance < min((double) *m_maxFeatureDist, 2 * min_dist))
                good_matches.push_back( matches[i]);
        }
        
        m_homography = UMat();
        
        Mat camMatrix;
        Mat camRotation;
        Mat camTranslation;
        
        // a minimum size of matches is needed for calculation of a homography
        if(good_matches.size() > *m_minMatchCount)
        {
            Mat inliers;
            
            vector<Point2f> pts_train, pts_query;
            matches2points(m_trainKeypoints, keypoints, good_matches, pts_train,
                           pts_query);
            m_homography = findHomography(pts_train, pts_query, CV_RANSAC, 3, inliers).getUMat(ACCESS_READ);
            
            vector<Point3f> trainPts3;
            for (int i=0; i<pts_train.size(); i++)
            {
                const Point2f p = pts_train[i];
                trainPts3.push_back(Point3f(p.x, 0, -p.y));
            }
            
            //
            static float bla[] = {  837.8487443,    0.,             388.558868,
                                    0.,             891.76507372,   305.75884143,
                                    0.,             0.,             1.};
            
            camMatrix = Mat(3,3,CV_32FC1, bla);
            
            
//            solvePnP(trainPts3, pts_query, camMatrix, noArray(),
//                     camRotation, camTranslation, false, CV_EPNP);
            
            solvePnPRansac(trainPts3, pts_query, camMatrix, noArray(), camRotation, camTranslation);
        }
        
        // draw keypoints and patch-borders
        if(true)
        {
            // draw good_matches
            for (int i=0; i<good_matches.size(); i++)
            {
                const DMatch &m = good_matches[i];
                
                KeyPoint &kp = keypoints[m.queryIdx];
                circle(m_outImg, kp.pt * (1.f / scale),kp.size * (1.f / scale),
                       Scalar(0,180,255));
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
                
                line(m_outImg, scenePts[0]* (1.f / scale), scenePts[1]* (1.f / scale),
                     Scalar(0, 255, 0), 3);
                line(m_outImg, scenePts[1]* (1.f / scale), scenePts[2]* (1.f / scale),
                     Scalar(0, 255, 0), 3);
                line(m_outImg, scenePts[2]* (1.f / scale), scenePts[3]* (1.f / scale),
                     Scalar(0, 255, 0), 3);
                line(m_outImg, scenePts[3]* (1.f / scale), scenePts[0]* (1.f / scale),
                     Scalar(0, 255, 0), 3);
            }
            
        }
        
        // iphone4s scaled
        //[[ 837.8487443     0.          388.558868  ]
        //[   0.          891.76507372  305.75884143]
        //[   0.            0.            1.        ]]
        
        //iSight
        //[[ 468.35129369    0.          543.09378208]
        //[   0.          465.94360054  323.30103704]
        //[   0.            0.            1.        ]]
        
        return { m_referenceImage.getMat(ACCESS_READ), m_outImg.getMat(ACCESS_READ) };
    }
    
    void KeyPointNode::setReferenceImage(const Mat &theImg)
    {
        m_referenceImage = theImg.getUMat(ACCESS_READ);
        
        GaussianBlur(theImg, m_referenceImage, Size(7, 7), 1.5);

        // scale down if necessary (ORB did not properly manage large ref-images)
        float scale = (float)*m_maxPatchWidth / m_referenceImage.cols;
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
    
    void KeyPointNode::update(const Property::Ptr &theProperty)
    {
        if(theProperty == m_maxFeatureDist)
        {
            //printf("max DISTANCE\n");
        }
        else if(theProperty == m_maxPatchWidth)
        {
            
        }
    
    }
}