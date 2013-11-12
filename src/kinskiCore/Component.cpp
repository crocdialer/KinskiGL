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
    
void Component::observeProperties(bool b)
{
    observeProperties(m_propertyList, b);
}

};