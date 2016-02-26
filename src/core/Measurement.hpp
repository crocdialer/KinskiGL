//
//  Measurement.hpp
//  gl
//
//  Created by Fabian on 19/07/14.
//
//

#pragma once

#include "core/core.hpp"

namespace kinski
{
    
    /*
     * simple generic filter interface
     */
    template <typename T>
    class Filter
    {
    public:
        typedef std::shared_ptr<Filter> Ptr;
        
        virtual const T filter(const T &theValue) = 0;
    };

    /////////////////////////////////////////////////////////////////
    
    template <typename T>
    class PassThroughFilter : public Filter<T>
    {
        inline const T filter(const T &the_value) override { return the_value; }
    };
    
    /////////////////////////////////////////////////////////////////
    
    template <typename T>
    class MovingAverage : public Filter<T>
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
            return ret / (float)m_values.size();
        }
        
        inline const T filter(const T &theValue) override
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

    /////////////////////////////////////////////////////////////////
    
    template <typename T>
    class FalloffFilter : public Filter<T>
    {
    public:
        FalloffFilter(float update_speed = .2):
        m_last_value(T(0)),
        m_speed_dec(kinski::clamp(update_speed, 0.f, 1.f)),
        m_speed_inc(1.f)
        {}
        
        inline const T filter(const T &the_value) override
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
        
        void set_increase_speed(float is){ m_speed_inc = kinski::clamp(is, 0.f, 1.f); };
        void set_decrease_speed(float ds){ m_speed_dec = kinski::clamp(ds, 0.f, 1.f); };
        
    private:
        T m_last_value;
        float m_speed_dec, m_speed_inc;
        
    };

    /////////////////////////////////////////////////////////////////
    
    template <typename T> class Measurement
    {
    public:
        explicit Measurement(uint32_t hist_size = 1000,
                             const std::string &description = "generic value"):
        m_description(description),
        m_min(std::numeric_limits<T>::max()),
        m_max(std::numeric_limits<T>::min()),
        m_filter(new PassThroughFilter<T>())
        {
            set_history_size(hist_size);
        }
        
        inline void push(const T &value)
        {
            m_values[m_current_index] = m_filter->filter(value);
            m_current_index = (m_current_index + 1) % m_values.size();
            m_min = std::min(value, m_min);
            m_max = std::max(value, m_max);
        }
        
        inline const std::vector<T>& history() const { return m_values;}
        inline std::size_t history_size() const { return m_values.size();}
        inline void set_history_size(std::size_t the_size)
        {
            m_current_index = clamp<std::size_t>(m_current_index, 0, the_size - 1);
            m_values.resize(the_size);
            reset();
        }
        inline const int current_index() const { return m_current_index;}
        inline const T& last_value() const
        {
            int previous_index = m_current_index - 1;
            return previous_index < 0 ? m_values.back() : m_values[previous_index];
        }
        
        inline const std::string& description() const {return m_description;}
        inline void description(const std::string& dsc) {m_description = dsc;}
        
        inline const T mean() const
        {
            T sum(0);
            for (const auto &val : m_values){ sum += val; }
            return static_cast<T>(sum / (float)m_values.size());
        }
        
        inline const T variance() const
        {
            T mean_val = mean();
            T sum(0);
            for (const auto &val : m_values){ sum += (val - mean_val) * (val - mean_val); }
            return static_cast<T>(sum / (float)m_values.size());
        }
        
        inline const T standard_deviation() const {return sqrtf(variance());}
        
        inline const T min() const {return m_min;}
        
        inline const T max() const {return m_max;}
        
        inline void reset()
        {
            std::fill(m_values.begin(), m_values.end(), T(0));
            m_current_index = 0;
            m_min = std::numeric_limits<T>::max();
            m_max = std::numeric_limits<T>::min();
        }
        
        typename Filter<T>::Ptr get_filter() const { return m_filter; }
        void set_filter(typename Filter<T>::Ptr the_filter){m_filter = the_filter;}
        
        inline T& operator[](std::size_t idx)
        {
            assert(idx < history_size());
            idx = (m_current_index + idx) % history_size();
            return m_values[idx];
        }
        
        inline const T& operator[](std::size_t idx) const { return (*this)[idx]; }
        
    private:
        std::string m_description;
        T m_min, m_max;
        std::shared_ptr<Filter<T>> m_filter;
        std::vector<T> m_values;
        std::size_t m_current_index;
    };
}// namespace kinski