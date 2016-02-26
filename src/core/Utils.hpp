//
//  Utils.h
//  gl
//
//  Created by Fabian on 11/11/13.
//
//

#pragma once

//#include <sstream>
#include <iomanip>
#include "core/core.hpp"

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
                                           std::vector<std::string> &elems,
                                           bool remove_empty_splits = true)
    {
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim))
        {
            if(!item.empty() || !remove_empty_splits)
                elems.push_back(item);
        }
        return elems;
    }
    
    inline std::vector<std::string> split(const std::string &s,
                                          char delim = ' ',
                                          bool remove_empty_splits = true)
    {
        std::vector<std::string> elems;
        split(s, delim, elems, remove_empty_splits);
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
    
    inline std::string to_lower(const std::string &str)
    {
        std::string ret = str;
        std::transform(str.begin(), str.end(), ret.begin(), ::tolower);
        return ret;
    }
    
    inline std::string to_upper(const std::string &str)
    {
        std::string ret = str;
        std::transform(str.begin(), str.end(), ret.begin(), ::toupper);
        return ret;
    }
    
    template <typename T, typename C>
    inline bool is_in(const T &elem, const C &container)
    {
        return std::find(std::begin(container), std::end(container), elem) != std::end(container);
    }
    
    namespace details
    {
        // allow in-order expansion of parameter packs.
        struct do_in_order
        {
            template<typename T> do_in_order(std::initializer_list<T>&&) { }
        };
        
        template<typename C1, typename C2> void concat_helper(C1& l, const C2& r)
        {
            l.insert(std::end(l), std::begin(r), std::end(r));
        }
//        template<typename C1, typename C2> void concat_helper(C1& l, C2&& r)
//        {
//            l.insert(std::end(l), std::make_move_iterator(std::begin(r)),
//                     std::make_move_iterator(std::end(r)));
//        }
    } // namespace details
    
    template<typename T, typename... C>
    inline std::vector<T> concat_containers(C&&... the_containers)
    {
        std::vector<T> ret;
        details::do_in_order{(details::concat_helper(ret, std::forward<C>(the_containers)), 0)...};
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
        float mix_val = clamp<float>((val - src_min) / (float)(src_max - src_min), 0.f, 1.f);
        return mix<T>(dst_min, dst_max, mix_val);
    }
    
    template<typename T>
    class CircularBuffer
    {
    public:
        
        CircularBuffer(uint32_t the_cap = 10):
        m_array_size(0),
        m_first(0),
        m_last(0),
        m_data(NULL)
        {
            set_capacity(the_cap);
        }
        
        virtual ~CircularBuffer()
        {
            if(m_data){ delete[](m_data); }
        }
        
        inline void clear()
        {
            m_first = m_last = 0;
        }
        
        inline void push(const T &the_val)
        {
            m_data[m_last] = the_val;
            m_last = (m_last + 1) % m_array_size;
            
            if(m_first == m_last){ m_first = (m_first + 1) % m_array_size; }
        }
        
        inline const T pop()
        {
            if(!empty())
            {
                const T ret = m_data[m_first];
                m_first = (m_first + 1) % m_array_size;
                return ret;
            }
            else{ return T(0); }
        }
        
        inline uint32_t capacity() const { return m_array_size - 1; };
        void set_capacity(uint32_t the_cap)
        {
            if(m_data){ delete[](m_data); }
            m_data = new T[the_cap + 1];
            m_array_size = the_cap + 1;
            clear();
        }
        
        inline uint32_t size() const
        {
            int ret = m_last - m_first;
            if(ret < 0){ ret += m_array_size; }
            return ret;
        };
        
        inline bool empty() const { return m_first == m_last; }
        
        inline const T operator[](uint32_t the_index) const
        {
            if(the_index < size()){ return m_data[(m_first + the_index) % m_array_size]; }
            else{ return T(0); }
        };
        
    private:
        
        int32_t m_array_size, m_first, m_last;
        T* m_data;
    };
}