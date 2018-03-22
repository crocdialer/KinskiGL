//
//  Header.h
//  core
//
//  Created by Fabian on 22.03.18.
//

#pragma once
#include "core/core.hpp"

namespace kinski{

template<typename T> class Area_
{
public:
    T x0, y0, x1, y1;
    
    Area_():x0(0), y0(0), x1(0), y1(0){};
    Area_(const T &the_x0, const T &the_y0, const T &the_x1, const T &the_y1):
    x0(the_x0), y0(the_y0), x1(the_x1), y1(the_y1){};
    
    inline const T width() const { return std::fabs(((double)x1 - (double)x0)); };
    inline const T height() const { return std::fabs(((double)y1 - (double)y0)); };
    
    inline T size() const { return width() * height(); }
    
    inline bool operator<(const Area_<T> &other) const
    {
        return std::tie(x0, y0, x1, y1) < std::tie(other.x0, other.y0, other.x1, other.y1);
    }
    inline bool operator==(const Area_<T> &other) const
    {
        return !(*this != other);
    }
    inline bool operator!=(const Area_<T> &other) const
    {
        return x0 != other.x0 || y0 != other.y0 || x1 != other.x1 || y1 != other.y1;
    }
};
    
}// namespace kinski
