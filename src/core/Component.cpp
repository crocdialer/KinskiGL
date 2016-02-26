// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Component.hpp"

namespace kinski{

Component::Component(const std::string &theName):
m_name(theName)
{
    
}

Component::~Component() 
{
}

Property::Ptr Component::get_property_by_name(const std::string & thePropertyName) 
{
    std::list<Property::Ptr>::iterator it = m_propertyList.begin();
    for (; it != m_propertyList.end(); ++it)
    {
        Property::Ptr &property = *it;
        if (property->getName() == thePropertyName) 
            return property;
    }
    throw PropertyNotFoundException(thePropertyName);
}

const std::list<Property::Ptr>& 
Component::get_property_list() const
{
    return m_propertyList;
}

void Component::register_property(Property::Ptr theProperty) 
{
    m_propertyList.push_back(theProperty);
}

void Component::unregister_property(Property::Ptr theProperty)
{
    m_propertyList.remove(theProperty);
}
    
void Component::observe_properties(const std::list<Property::Ptr>& theProps,  bool b)
{
    std::list<Property::Ptr>::const_iterator it = theProps.begin();
    for (; it != theProps.end(); ++it)
    {
        const Property::Ptr &property = *it;
        if (b)
            property->addObserver(shared_from_this());
        else
            property->removeObserver(shared_from_this());
    }
}
    
void Component::observe_properties(bool b)
{
    observe_properties(m_propertyList, b);
}
    
void Component::unregister_all_properties()
{
    m_propertyList.clear();
}

bool Component::call_function(const std::string &the_function_name,
                              const std::vector<std::string> &the_params)
{
    auto iter = m_function_map.find(the_function_name);
    
    if(iter != m_function_map.end())
    {
        iter->second(the_params);
        return true;
    }
    return false;
}
    
void Component::register_function(const std::string &the_name, Functor the_functor)
{
    m_function_map[the_name] = the_functor;
}

void Component::unregister_function(const std::string &the_name, Functor the_functor)
{
    auto iter = m_function_map.find(the_name);
    if(iter != m_function_map.end()){ m_function_map.erase(iter); }
}

void Component::unregister_all_functions()
{
    m_function_map.clear();
}

}