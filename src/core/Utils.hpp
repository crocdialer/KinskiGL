// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Utils.h
//
//  Created by Fabian on 11/11/13.

#pragma once

#include <iomanip>
#include <random>
#include "core/core.hpp"

namespace kinski
{
    template <typename T>
    inline std::string to_string(const T &theObj, int precision = 0)
    {
        std::stringstream ss;
        if(precision > 0)
            ss << std::fixed << std::setprecision(precision);
        ss << theObj;
        return ss.str();
    }
    
    template <typename T>
    inline T string_to(const std::string &str)
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
    
    inline std::string trim(const std::string& str,
                            const std::string& whitespace = " \t")
    {
        auto str_begin = str.find_first_not_of(whitespace);
        if(str_begin == std::string::npos){ str_begin = 0; }
        auto str_end = str.find_last_not_of(whitespace);
        if(str_end == std::string::npos){ str_end = str.size() - 1; }
        const auto str_range = str_end - str_begin + 1;
        return str.substr(str_begin, str_range);
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
    
    template <typename T> inline T swap_endian(T u)
    {
        union
        {
            T u;
            unsigned char u8[sizeof(T)];
        } source, dest;
        source.u = u;
        
        for (size_t k = 0; k < sizeof(T); k++){ dest.u8[k] = source.u8[sizeof(T) - k - 1]; }
        return dest.u;
    }
    
    inline void swap_endian(void* dest, const void* src, size_t num_bytes)
    {
        uint8_t* tmp = new uint8_t[num_bytes];
        const uint8_t *src_ptr = (const uint8_t*)src;
        
        for(size_t i = 0; i < num_bytes; ++i)
        {
            tmp[i] = src_ptr[num_bytes - 1 - i];
        }
        memcpy(dest, tmp, num_bytes);
        delete[](tmp);
    }
    
    template <typename T, typename C>
    inline bool contains(const C &container, const T &elem)
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
    
    template <typename T, typename C>
    inline const T median(const C &the_container)
    {
        std::vector<T> tmp_array(std::begin(the_container), std::end(the_container));
        size_t n = tmp_array.size() / 2;
        std::nth_element(tmp_array.begin(), tmp_array.begin() + n, tmp_array.end());
        return tmp_array[n];
    }
    
    template <typename T>
    inline int sgn(T val)
    {
        return (T(0) < val) - (val < T(0));
    }
    
    template <typename T>
    inline const T& clamp(const T &val, const T &min, const T &max)
    {
        return val < min ? min : (val > max ? max : val);
    }
    
    template <typename T, typename Real = float>
    inline T mix(const T &lhs, const T &rhs, Real ratio)
    {
        return lhs + ratio * (rhs - lhs);
    }
    
    template <typename T, typename Real = float>
    inline T mix_slow(const T &lhs, const T &rhs, Real ratio)
    {
        return lhs * (Real(1) - ratio) + rhs * ratio;
    }
    
    template <typename T>
    inline T map_value(const T &val, const T &src_min, const T &src_max,
                       const T &dst_min, const T &dst_max)
    {
        float mix_val = clamp<float>((val - src_min) / (float)(src_max - src_min), 0.f, 1.f);
        return mix<T>(dst_min, dst_max, mix_val);
    }
    
    template <typename T>
    inline T random(const T &min, const T &max)
    {
        // Seed with a real random value, if available
        std::random_device r;
        
        // random mean
        std::default_random_engine e1(r());
        std::uniform_int_distribution<uint64_t> uniform_dist(0, std::numeric_limits<uint64_t>::max());
        return mix_slow<T>(min, max, uniform_dist(e1) / (double) std::numeric_limits<uint64_t>::max());
    }
    
    template <typename T = int32_t>
    inline T random_int(const T &min, const T &max)
    {
        // Seed with a real random value, if available
        std::random_device r;
        
        // random mean
        std::default_random_engine e1(r());
        std::uniform_int_distribution<T> uniform_dist(min, max);
        return uniform_dist(e1);
    }
    
    inline std::string syscall(const std::string& cmd)
    {
        std::string ret;
        std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
        char buf[1024] = "\0";
        while (fgets(buf, sizeof(buf), pipe.get())){ ret.append(buf); }
        return ret;
    }
    
    template<typename T> class Area
    {
    public:
        T x1, y1, x2, y2;
        
        Area():x1(0), y1(0), x2(0), y2(0){};
        Area(const T &theX1, const T &theY1, const T &theX2, const T &theY2):
        x1(theX1), y1(theY1), x2(theX2), y2(theY2){};
        
        inline const T width() const { return std::fabs<T>(x1 - x2); };
        inline const T height() const { return std::fabs<T>(y1 - y2); };
        
        inline T size() const { return width() * height(); }
        
        bool operator<(const Area<T> &other) const
        {
            return size() < other.size();
        }
    };
    
    template<typename T>
    class CircularBuffer
    {
    public:
        
        CircularBuffer(uint32_t the_cap = 10):
        m_array_size(0),
        m_first(0),
        m_last(0),
        m_data(nullptr)
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
        inline void set_capacity(uint32_t the_cap)
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
        
        inline const T median() const
        {
            auto start = m_first, end = m_last;
            if(start > end){ std::swap(start, end); }
            return kinski::median(std::vector<T>(m_data + start, m_data + end));
        }
        
    private:
        
        int32_t m_array_size, m_first, m_last;
        T* m_data;
    };
}