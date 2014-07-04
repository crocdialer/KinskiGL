//
//  Utils.h
//  kinskiGL
//
//  Created by Fabian on 11/11/13.
//
//

#ifndef kinskiGL_Utils_h
#define kinskiGL_Utils_h

#include <sstream>
#include <iomanip>

namespace kinski
{
    template <typename T>
    inline std::string as_string(const T &theObj, int precision = 0)
    {
        std::stringstream ss;
        if(precision > 0)
            ss << std::fixed << std::setprecision(precision);
        ss << theObj;
        return ss.str();
    }
    
    template <typename T>
    inline T string_as(const std::string &str)
    {
        T ret;
        std::stringstream ss(str);
        ss >> ret;
        return ret;
    }
    
    inline std::vector<std::string>& split(const std::string &s, char delim,
                                           std::vector<std::string> &elems)
    {
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim))
        {
            if(!item.empty())
                elems.push_back(item);
        }
        return elems;
    }
    
    inline std::vector<std::string> split(const std::string &s, char delim = ' ')
    {
        std::vector<std::string> elems;
        split(s, delim, elems);
        return elems;
    }
    
    inline std::vector<std::string> split_by_string(const std::string &s,
                                                    const std::string &delim = " ")
    {
        std::vector<std::string> elems;
        std::string::size_type pos = s.find(delim), current_pos = 0;
        
        while(pos != std::string::npos)
        {
            elems.push_back(s.substr(current_pos, pos - current_pos));
            current_pos = pos + delim.size();
            
            // continue searching
            pos = s.find(delim, current_pos);
        }
        
        // remainder
        std::string remainder = s.substr(current_pos, s.size() - current_pos);
        if(!remainder.empty())
            elems.push_back(remainder);
        
        return elems;
    }
    
    inline std::string remove_whitespace(const std::string &input)
    {
        std::string ret(input);
        ret.erase(std::remove_if(ret.begin(),
                                 ret.end(),
                                 std::bind(std::isspace<char>,
                                           std::placeholders::_1,
                                           std::locale::classic())),
                  ret.end());
        return ret;
    }
    
    template <typename T>
    inline int sgn(T val)
    {
        return (T(0) < val) - (val < T(0));
    }
    
    template <typename T>
    inline T random(const T &min, const T &max)
    {
        return min + (max - min) * (rand() / (float) RAND_MAX);
    }
    
    template <typename T>
    inline const T& clamp(const T &val, const T &min, const T &max)
    {
        return val < min ? min : (val > max ? max : val);
    }
    
    template <typename T>
    inline T mix(const T &lhs, const T &rhs, float ratio)
    {
        return lhs + ratio * (rhs - lhs);
    }
    
    template <typename T>
    inline T map_value(const T &val, const T &src_min, const T &src_max,
                       const T &dst_min, const T &dst_max)
    {
        float mix_val = (float)val / (src_max - src_min);
        return mix<T>(dst_min, dst_max, mix_val);
    }
    
    template <typename T>
    class MovingAverage
    {
    public:
        explicit MovingAverage(uint32_t sz = 5):m_size(sz){}
        inline void push(const T &theValue)
        {
            m_values.push_back(theValue);
            if(m_values.size() > m_size) m_values.pop_front();
        }
        
        inline const T filter()
        {
            T ret(0);
            typename std::list<T>::const_iterator it = m_values.begin();
            for (; it != m_values.end(); ++it){ret += *it;}
            return ret / (float)m_values.size();;
        }
        
        inline const T filter(const T &theValue)
        {
            push(theValue);
            return filter();
        }
        
        uint32_t window_size() const {return m_size;}
        void window_size(uint32_t ws) {m_size = ws;}
        
    private:
        std::list<T> m_values;
        uint32_t m_size;
    };
    
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
