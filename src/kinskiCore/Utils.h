//
//  Utils.h
//  kinskiGL
//
//  Created by Fabian on 11/11/13.
//
//

#ifndef kinskiGL_Utils_h
#define kinskiGL_Utils_h

namespace kinski
{
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
        while (std::getline(ss, item, delim)) { elems.push_back(item); }
        return elems;
    }
    
    
    inline std::vector<std::string> split(const std::string &s, char delim = ' ')
    {
        std::vector<std::string> elems;
        split(s, delim, elems);
        return elems;
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
}

#endif
