//
//  SalienceNode.cpp
//  kinskiGL
//
//  Created by Fabian Schmidt on 7/27/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "SalienceNode.h"

using namespace cv;
namespace kinski
{
    
    SalienceNode::SalienceNode():
    m_colorMapType(COLORMAP_JET)
    {
        
        m_salienceDetect.setUseColorInformation(1);
        m_salienceDetect.setUseDoBFeatures(1);
        m_salienceDetect.setUseDoEFeatures(1);
        m_salienceDetect.setUseGGDistributionParams(1);
    }
    
    SalienceNode::~SalienceNode()
    {
        
    }
    
    vector<Mat> SalienceNode::doProcessing(const Mat &img)
    {
        vector<Mat> outMats;
        
        cv::Mat downSized, colorSalience;
        
        double ratio = 240 * 1. / img.cols;
        resize(img, downSized, cv::Size(0,0), ratio, ratio, INTER_LINEAR);
        m_salienceDetect.updateSalience(downSized);
        m_salienceDetect.getSalImage(m_salienceImage);
        
        //resize(salImg,salImg,inFrame.size(),CV_INTER_LINEAR);
        
        applyColorMap(m_salienceImage, colorSalience, m_colorMapType) ;
        
        outMats.push_back(m_salienceImage);
        outMats.push_back(colorSalience);

        return outMats;
    }
}