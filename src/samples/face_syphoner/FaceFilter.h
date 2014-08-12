//
//  DopeRecorder.h
//  gl
//
//  Created by Fabian on 4/4/13.
//
//

#ifndef __gl__FaceFilter__
#define __gl__FaceFilter__

#include "cv/CVNode.h"

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


#endif /* defined(__gl__FaceFilter__) */
