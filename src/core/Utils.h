//
//  Utils.h
//  gl
//
//  Created by Fabian on 11/11/13.
//
//

#ifndef gl_Utils_h
#define gl_Utils_h

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
    
    template <typename X, typename C>
    inline bool is_in(const X &elem, const C &container)
    {
        return std::find(begin(container), end(container), elem) != end(container);
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
        float mix_val = clamp<float>(val / (src_max - src_min), 0.f, 1.f);
        return mix<T>(dst_min, dst_max, mix_val);
    }
}

#endif
