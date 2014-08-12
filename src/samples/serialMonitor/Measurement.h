//
//  Measurement.h
//  gl
//
//  Created by Fabian on 05/11/13.
//
//

#ifndef gl_Measurement_h
#define gl_Measurement_h

namespace kinski
{
    template <typename T> class FalloffFilter
    {
    public:
        FalloffFilter(float update_speed = .2):
        m_speed_dec(kinski::clamp(update_speed, 0.f, 1.f)),
        m_speed_inc(1.f)
        {}
        
        inline const T filter(const T &the_value)
        {
            if(the_value > m_last_value)
            {
                m_last_value = kinski::mix(m_last_value, the_value, m_speed_inc);
            }
            else
            {
                m_last_value = kinski::mix(m_last_value, the_value, m_speed_dec);
            }
            return m_last_value;
        };
        
    private:
        T m_last_value;
        float m_speed_inc, m_speed_dec;
    
    };
    
    template <typename T> class Measurement
    {
    public:
        explicit Measurement(const std::string &description = "generic value",
                             uint32_t hist_size = 1000):
        m_description(description),
        m_min(std::numeric_limits<T>::max()),
        m_max(std::numeric_limits<T>::min())
        {
            history_size(hist_size);
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
            reset();
        }
        inline const int current_index() const { return m_current_measure_index;}
        inline const T& last_value() const
        {
            int previous_index = m_current_measure_index - 1;
            return previous_index < 0 ? m_measure_history.back() : m_measure_history[previous_index];
        }
        
        inline uint32_t filter_window_size() const {return m_avg_filter.window_size();}
        inline void filter_window_size(uint32_t sz) {return m_avg_filter.window_size(sz);}
        
        inline const std::string& description() const {return m_description;}
        inline void description(const std::string& dsc) {m_description = dsc;}
        
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
        std::string m_description;
        T m_min, m_max;
        MovingAverage<T> m_avg_filter;
        std::vector<T> m_measure_history;
        uint32_t m_current_measure_index;
    };
}

#endif
