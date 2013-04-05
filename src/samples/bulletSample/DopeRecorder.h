//
//  DopeRecorder.h
//  kinskiGL
//
//  Created by Fabian on 4/4/13.
//
//

#ifndef __kinskiGL__DopeRecorder__
#define __kinskiGL__DopeRecorder__

#include <queue>
#include "kinskiCV/CVNode.h"

namespace kinski
{
    class DopeRecorder : public kinski::CVProcessNode
    {
    public:
        DopeRecorder(const int buffer_size):
        m_buffer_size(RangedProperty<int>::create("Buffer Size", buffer_size, 0, 100000)),
        m_randomize(Property_<bool>::create("Randomize", false)),
        m_current_index(0)
        {
            set_name("DopeRecorder");
            registerProperty(m_buffer_size);
            registerProperty(m_randomize);
            m_buffer.resize(buffer_size);
            srand(time(0));
        }
        
        std::vector<cv::Mat> doProcessing(const cv::Mat &img)
        {
            m_buffer[m_current_index] = img;
            m_current_index = (m_current_index + 1) % *m_buffer_size;
            
            std::vector<cv::Mat> outMats;
            if(*m_randomize) outMats.push_back(m_buffer[random<int>(0, *m_buffer_size - 1)]);
            else outMats.push_back(img);
            return outMats;
        };
    
        void updateProperty(const Property::ConstPtr &theProperty)
        {
            if(theProperty == m_buffer_size)
            {
                m_buffer.resize(*m_buffer_size);
            }
        }
        
    private:
        RangedProperty<int>::Ptr m_buffer_size;
        Property_<bool>::Ptr m_randomize;
        std::vector<cv::Mat> m_buffer;
        uint32_t m_current_index;
    };
    
}


#endif /* defined(__kinskiGL__DopeRecorder__) */
