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

Property::Property(std::string theName, boost::any theValue):
    m_name(theName), m_value(theValue), m_tweakable(true)
{
} 

boost::any Property::getValue() 
{
    return m_value;
}

std::string Property::getName() 
{
    return m_name;
}

void Property::setIsTweakable(bool isTweakableFlag) 
{
	m_tweakable = isTweakableFlag;
}

bool Property::getIsTweakable() 
{
	return m_tweakable;
}
