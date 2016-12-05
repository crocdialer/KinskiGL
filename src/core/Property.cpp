//
//  Property.cpp
//  kinskiGL
//
//  Created by Fabian on 04/12/2016.
//
//

#include <boost/signals2.hpp>
#include "Property.hpp"

namespace kinski
{
    typedef boost::signals2::signal<void(const Property::ConstPtr&)> signal_t;
    
    struct PropertyImpl
    {
        signal_t m_signal;
        std::string m_name;
        bool m_tweakable = true;
    };
    
    Property::Property():m_impl(new PropertyImpl)
    {
        
    }
    
    Property::Property(const std::string &theName, const boost::any &theValue):
    m_impl(new PropertyImpl), m_value(theValue)
    {
        m_impl->m_name = theName;
    };
    
    const std::string& Property::name() const
    {
        return m_impl->m_name;
    }
    
    void Property::set_name(const std::string& theName)
    {
        m_impl->m_name = theName;
    }
    
    void Property::set_tweakable(bool isTweakable)
    {
        m_impl->m_tweakable = isTweakable;
    }
    
    bool Property::tweakable() const
    {
        return m_impl->m_tweakable;
    }
    
    void Property::add_observer(const ObserverPtr &theObs)
    {
        m_impl->m_signal.connect(signal_t::slot_type(&Observer::update_property, theObs, _1).track_foreign(theObs));
    }
    
    void Property::remove_observer(const ObserverPtr &theObs)
    {
        m_impl->m_signal.disconnect(boost::bind(&Observer::update_property, theObs, _1));
    }
    
    void Property::clear_observers()
    {
        m_impl->m_signal.disconnect_all_slots();
    }
    
    void Property::notify_observers()
    {
        m_impl->m_signal(shared_from_this());
    }
    
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
