//
//  SkySegmentNode.h
//  gl
//
//  Created by Fabian Schmidt on 6/25/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef _cv_CVNode_included_
#define _cv_CVNode_included_

#include "cv/CVNode.h"

namespace kinski
{
    class ColorHistNode : public CVProcessNode
    {
    public:
        
        ColorHistNode();
        virtual ~ColorHistNode();
        
        std::string getDescription();
        
        std::vector<cv::Mat> doProcessing(const cv::Mat &img);
        
        void setHistExtraction(bool b);
        bool hasHistExtraction();
        
     private:
        
        // histogram related variables
        
        /*!
         * number of histogram bins
         */
        int m_histSize[1];
        
        float m_hueRanges[2];
        const float *m_ranges;
        int channels[1];
        
        cv::Mat m_colorHist;
        cv::Mat m_histImage;
        
        int m_roiWidth;
        
        cv::Mat extractHueHistogram(const cv::Mat &roi);
        cv::Mat createHistImage();
        
        Property_<bool>::Ptr m_histExtraction;
    };
}//kinski
#endif