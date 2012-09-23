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
    m_colorMapType(COLORMAP_JET),
    m_useColor(Property_<bool>::create("Color Contrast (red-green / blue-yellow)", true)),
    m_useDoB(Property_<bool>::create("Difference of Box (spatial)", true)),
    m_useDoE(Property_<bool>::create("Difference of Exponential (motion)", true))
    {
        registerProperty(m_useColor);
        registerProperty(m_useDoB);
        registerProperty(m_useDoE);
    }
    
    SalienceNode::~SalienceNode()
    {
        
    }
    
    string SalienceNode::getDescription()
    {
        return "SalienceNode - FastSalience algorithm by Butko et al. (2008)";
    }
    
    vector<Mat> SalienceNode::doProcessing(const Mat &img)
    {
        m_salienceDetect.setUseColorInformation(m_useColor->val());
        m_salienceDetect.setUseDoBFeatures(m_useDoB->val());
        m_salienceDetect.setUseDoEFeatures(m_useDoE->val());
        
        cv::Mat downSized, colorSalience;
        
        double ratio = 240 * 1. / img.cols;
        resize(img, downSized, cv::Size(0,0), ratio, ratio, INTER_LINEAR);
        m_salienceDetect.updateSalience(downSized);
        m_salienceDetect.getSalImage(m_salienceImage);
        
        //threshold(m_salienceImage, m_salienceImage, .5, 1.0, CV_MINMAX);
        
        m_salienceImage.convertTo(colorSalience, CV_8UC1, 255.0);
        //applyColorMap(colorSalience, colorSalience, m_colorMapType) ;
        
        vector<Mat> outMats;
        outMats.push_back(m_salienceImage);
        //outMats.push_back(colorSalience);

        return outMats;
    }
}