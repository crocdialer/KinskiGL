//
//  Measurement.h
//  kinskiGL
//
//  Created by Fabian on 05/11/13.
//
//

#ifndef kinskiGL_Measurement_h
#define kinskiGL_Measurement_h

namespace kinski
{
    template <typename T> class Measurement
    {
    public:
        explicit Measurement(uint32_t hist = 1000):
        m_min(std::numeric_limits<T>::max()),
        m_max(std::numeric_limits<T>::min())
        {
            history_size(hist);
        }
        
        inline void push(const T &value)
        {
            m_measure_history[m_current_measure_index] = m_avg_filter.filter(value);
            m_current_measure_index = (m_current_measure_index + 1) % m_measure_history.size();
            m_min = std::min(value, m_min);
            m_max = std::max(value, m_max);
        }
        
        inline const std::vector<T>& history() const { return m_measure_history;}
        inline uint32_t history_size() const { return m_measure_history.size();}
        inline void history_size(uint32_t sz)
        {
            m_current_measure_index = clamp<uint32_t>(m_current_measure_index, 0, sz);
            m_measure_history.resize(sz);
            std::fill(m_measure_history.begin(), m_measure_history.end(), T(0));
        }
        inline const int current_index() const { return m_current_measure_index;}
        inline const T& last_value() const
        {
            int previous_index = m_current_measure_index - 1;
            return previous_index < 0 ? m_measure_history.back() : m_measure_history[previous_index];
        }
        
        inline uint32_t filter_window_size() const {return m_avg_filter.window_size();}
        inline void filter_window_size(uint32_t sz) {return m_avg_filter.window_size(sz);}
        
        inline const T mean() const
        {
            T sum(0);
            for (const auto &val : m_measure_history){ sum += val; }
            return static_cast<T>(sum / (float)m_measure_history.size());
        }
        
        inline const T variance() const
        {
            T mean_val = mean();
            T sum(0);
            for (const auto &val : m_measure_history){ sum += (val - mean_val) * (val - mean_val); }
            return static_cast<T>(sum / (float)m_measure_history.size());
        }
        
        inline const T standard_deviation() const {return sqrtf(variance());}
        
        inline const T min() const {return m_min;}
        
        inline const T max() const {return m_max;}
        
        inline void reset()
        {
            std::fill(m_measure_history.begin(), m_measure_history.end(), T(0));
            m_current_measure_index = 0;
            m_min = std::numeric_limits<T>::max();
            m_max = std::numeric_limits<T>::min();
        }
        
    private:
        T m_min, m_max;
        MovingAverage<T> m_avg_filter;
        std::vector<T> m_measure_history;
        int m_current_measure_index;
    };
}

#endif
