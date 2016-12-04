//
//  Property.cpp
//  kinskiGL
//
//  Created by Fabian on 04/12/2016.
//
//

#include "Property.hpp"

namespace kinski
{
    template class Property_<int32_t>;
    template class Property_<uint32_t>;
    template class Property_<int64_t>;
    template class Property_<uint64_t>;
    template class Property_<float>;
    template class Property_<double>;
    template class Property_<bool>;
    
    template<> Property_<std::string>& Property_<std::string>::operator-=(const std::string &theVal)
    { return *this; };
    
    template<> Property_<std::string>& Property_<std::string>::operator*=(const std::string &theVal)
    { return *this; };
    
    template<> Property_<std::string>& Property_<std::string>::operator/=(const std::string &theVal)
    { return *this; };
    
    template class Property_<std::string>;
    
    template<>
    Property_<std::vector<std::string>>&
    Property_<std::vector<std::string>>::operator+=(const std::vector<std::string> &theVal)
    { return *this; };
    
    template<>
    Property_<std::vector<std::string>>&
    Property_<std::vector<std::string>>::operator-=(const std::vector<std::string> &theVal)
    { return *this; };
    
    template<> Property_<std::vector<std::string>>&
    Property_<std::vector<std::string>>::operator*=(const std::vector<std::string> &theVal)
    { return *this; };
    
    template<> Property_<std::vector<std::string>>&
    Property_<std::vector<std::string>>::operator/=(const std::vector<std::string> &theVal)
    { return *this; };
    
    template class Property_<std::vector<std::string>>;
    
    template class RangedProperty<int32_t>;
    template class RangedProperty<uint32_t>;
    template class RangedProperty<int64_t>;
    template class RangedProperty<uint64_t>;
    template class RangedProperty<float>;
    template class RangedProperty<double>;
}
