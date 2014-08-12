//
//  DopeRecorder.h
//  gl
//
//  Created by Fabian on 4/4/13.
//
//

#ifndef __gl__DopeRecorder__
#define __gl__DopeRecorder__

#include <queue>
#include "cv/CVNode.h"

namespace kinski
{
    class DopeRecorder : public kinski::CVProcessNode
    {
    public:
        DopeRecorder(const int buffer_size);
        std::vector<cv::Mat> doProcessing(const cv::Mat &img);
        void updateProperty(const Property::ConstPtr &theProperty);
        
    private:
        RangedProperty<int>::Ptr m_buffer_size;
        RangedProperty<int>::Ptr m_delay;
        RangedProperty<int>::Ptr m_blur_frames;
        Property_<bool>::Ptr m_record;
        Property_<bool>::Ptr m_randomize;
        std::vector<cv::Mat> m_buffer;
        std::vector<cv::Mat> m_blur_buffer;
        uint32_t m_current_index, m_current_blur_index;
    };
    
}


#endif /* defined(__gl__DopeRecorder__) */
