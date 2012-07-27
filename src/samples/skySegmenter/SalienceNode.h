//
//  SalienceNode.h
//  kinskiGL
//
//  Created by Fabian Schmidt on 6/25/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#pragma once

#include "kinskiCV/CVNode.h"
#include "FastSalience.h"

namespace kinski
{
    class SalienceNode : public kinski::CVProcessNode
    {
    private:
        FastSalience m_salienceDetect;
        
        cv::Mat m_salienceImage;
        
        uint32_t m_colorMapType;
        
        _Property<bool>::Ptr m_useColor;
        _Property<bool>::Ptr m_useDoB;
        _Property<bool>::Ptr m_useDoE;
        
    public:
        
        SalienceNode();
        virtual ~SalienceNode();
        
        std::string getDescription();
        
        std::vector<cv::Mat> doProcessing(const cv::Mat &img);
    };
    
}