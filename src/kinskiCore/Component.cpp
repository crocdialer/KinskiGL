// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Component.h"

namespace kinski{

Component::Component(const std::string &theName):
m_name(theName)
{
    
}

Component::~Component() 
{
}

Property::Ptr Component::getPropertyByName(const std::string & thePropertyName) 
{
    for (auto &property : m_propertyList )
    {
        if (property->getName() == thePropertyName) 
            return property;
    }
    throw PropertyNotFoundException(thePropertyName);
}

const std::list<Property::Ptr>& 
Component::getPropertyList() const
{
    return m_propertyList;
}

void Component::registerProperty(Property::Ptr theProperty) 
{
    m_propertyList.push_back(theProperty);
}

void Component::observeProperties(const std::list<Property::Ptr>& theProps,  bool b)
{
    for (const auto &property : theProps)
    {
        if (b)
            property->addObserver(shared_from_this());
        else
            property->removeObserver(shared_from_this());
    }
}
    
void Component::observeProperties(bool b)
{
    observeProperties(m_propertyList, b);
}

};