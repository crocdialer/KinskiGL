//
//  SalienceNode.h
//  gl
//
//  Created by Fabian Schmidt on 6/25/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#pragma once

#include "cv/CVNode.h"
#include "FastSalience.h"

namespace kinski
{
    class SalienceNode : public kinski::CVProcessNode
    {
    private:
        FastSalience m_salienceDetect;
        
        cv::Mat m_salienceImage;
        
        uint32_t m_colorMapType;
        
        RangedProperty<uint32_t>::Ptr m_salienceImageWidth;
        Property_<bool>::Ptr m_useColor;
        Property_<bool>::Ptr m_useDoB;
        Property_<bool>::Ptr m_useDoE;
        
    public:
        
        SalienceNode();
        virtual ~SalienceNode();
        
        std::string getDescription();
        void updateProperty(const Property::ConstPtr &theProperty);
        
        std::vector<cv::Mat> doProcessing(const cv::Mat &img);
    };
    
}