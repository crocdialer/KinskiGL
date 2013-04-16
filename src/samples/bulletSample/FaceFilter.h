//
//  DopeRecorder.h
//  kinskiGL
//
//  Created by Fabian on 4/4/13.
//
//

#ifndef __kinskiGL__FaceFilter__
#define __kinskiGL__FaceFilter__

#include "kinskiCV/CVNode.h"

namespace kinski
{
    class FaceFilter : public kinski::CVProcessNode
    {
    public:
        FaceFilter();
        std::vector<cv::Mat> doProcessing(const cv::Mat &img);
        void updateProperty(const Property::ConstPtr &theProperty);
        
    private:
        cv::CascadeClassifier m_cascade;
        cv::Mat m_small_img;
    };
    
}


#endif /* defined(__kinskiGL__FaceFilter__) */
