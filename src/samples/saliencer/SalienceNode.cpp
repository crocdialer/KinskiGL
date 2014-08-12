//
//  SalienceNode.cpp
//  gl
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
    m_salienceImageWidth(RangedProperty<uint32_t>::create("Salience map-width", 240, 120, 1024)),
    m_useColor(Property_<bool>::create("Color Contrast (red-green / blue-yellow)", true)),
    m_useDoB(Property_<bool>::create("Difference of Box (spatial)", true)),
    m_useDoE(Property_<bool>::create("Difference of Exponential (motion)", true))
    {
        registerProperty(m_salienceImageWidth);
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
        cv::Mat downSized, colorSalience;
        
        float ratio = (float)*m_salienceImageWidth / img.cols;
        resize(img, downSized, cv::Size(0,0), ratio, ratio, INTER_LINEAR);
        
        blur(downSized, downSized, Size(5, 5));
        m_salienceDetect.updateSalience(downSized);
        m_salienceDetect.getSalImage(m_salienceImage);
        
        //threshold(m_salienceImage, m_salienceImage, .5, 1.0, CV_MINMAX);
        
        m_salienceImage.convertTo(colorSalience, CV_8UC1, 255.0);
        
//        vector<Mat> outMats;
//        outMats.push_back(m_salienceImage);

        return {m_salienceImage};
    }
    
    // Property observer callback
    void SalienceNode::updateProperty(const Property::ConstPtr &theProperty)
    {
        // one of our porperties was changed
        if(theProperty == m_useColor)
        {
            m_salienceDetect.setUseColorInformation(*m_useColor);
        }
        else if(theProperty == m_useDoB)
        {
            m_salienceDetect.setUseDoBFeatures(*m_useDoB);
        }
        else if(theProperty == m_useDoE)
        {
            m_salienceDetect.setUseDoEFeatures(*m_useDoE);
        }
    }
}