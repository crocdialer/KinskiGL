//
//  DopeRecorder.cpp
//  gl
//
//  Created by Fabian on 4/4/13.
//
//

#include "DopeRecorder.h"
#include <algorithm>

namespace kinski
{
    DopeRecorder::DopeRecorder(const int buffer_size):
    m_buffer_size(RangedProperty<int>::create("Buffer Size", buffer_size, 1, 100000)),
    m_delay(RangedProperty<int>::create("Delay in frames", 0, 0, 25)),
    m_blur_frames(RangedProperty<int>::create("Blur frames", 1, 1, 25)),
    m_record(Property_<bool>::create("Record", true)),
    m_randomize(Property_<bool>::create("Randomize", false)),
    m_current_index(0),
    m_current_blur_index(0)
    {
        set_name("DopeRecorder");
        registerProperty(m_record);
        registerProperty(m_buffer_size);
        registerProperty(m_delay);
        registerProperty(m_blur_frames);
        registerProperty(m_randomize);
        m_buffer.resize(buffer_size);
        m_blur_buffer.resize(*m_blur_frames);
        srand(time(0));
    }
    
    std::vector<cv::Mat> DopeRecorder::doProcessing(const cv::Mat &img)
    {
        std::vector<cv::Mat> outMats;
        m_delay->setRange(0, *m_buffer_size);
        
        if(*m_record)
        {
            m_buffer[m_current_index] = img;
        }
        cv::Mat ret;
        if(*m_randomize)
        {
            ret = m_buffer[random<int>(0, *m_buffer_size - 1)];
        }
        else
        {
            int delay_index = m_current_index - *m_delay;
            if(delay_index < 0) delay_index += *m_buffer_size;
            ret = m_buffer[delay_index];
        }
        
        // blur stage
        m_blur_buffer[m_current_blur_index] = ret;
        m_current_blur_index = (m_current_blur_index + 1) % *m_blur_frames;
        for (int i = 1; i < *m_blur_frames; i++)
        {
            if(ret.size == m_blur_buffer[i].size)
                cv::addWeighted(ret, 0.5, m_blur_buffer[i], 0.5, 0.0, ret);
        }
        
        m_current_index = (m_current_index + 1) % *m_buffer_size;
        outMats.push_back(ret);
        return outMats;
    }
    
    void DopeRecorder::updateProperty(const Property::ConstPtr &theProperty)
    {
        if(theProperty == m_buffer_size)
        {
            m_delay->setRange(0, *m_buffer_size - 1);
            m_blur_frames->setRange(1, *m_buffer_size);
            m_buffer.resize(*m_buffer_size);
            if(*m_blur_frames > *m_buffer_size) m_blur_frames->set(*m_buffer_size);
            
        }
        else if(theProperty == m_blur_frames)
        {
            m_blur_buffer.resize(*m_blur_frames);
        }
    }
}

