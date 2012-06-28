//
//  Property.cpp
//  ATS
//
//  Created by Sebastian Heymann on 8/11/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "Property.h"

using namespace kinski;
using boost::any_cast;

Property::Property() : m_tweakable(true)
{
}

Property::Property(const std::string &theName, const boost::any &theValue,
                   const boost::any &min,
                   const boost::any &max):
    m_name(theName), m_value(theValue),
    m_min(min), m_max(max),
    m_tweakable(true)
{
} 

boost::any Property::getValue() const
{
    return m_value;
}

const std::string& Property::getName() const
{
    return m_name;
}

void Property::setName(const std::string& theName)
{
    m_name = theName;
}

void Property::setIsTweakable(bool isTweakableFlag) 
{
	m_tweakable = isTweakableFlag;
}

bool Property::getIsTweakable() const
{
	return m_tweakable;
}
