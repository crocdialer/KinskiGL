#include "Component.h"

using namespace kinski;

Component::Component()
{
}

Component::~Component() {
}

Property::Ptr Component::getPropertyByName(const std::string & thePropertyName) 
{
    
    std::list<Property::Ptr>::iterator it;
    for ( it = m_propertyList.begin() ; it != m_propertyList.end(); it++ ) 
    {
        if ((*it)->getName() == thePropertyName) 
            return (*it);
    }
    
    throw PropertyNotFoundException(thePropertyName);
}

std::list<Property::Ptr> 
Component::getPropertyList() 
{
    return std::list<Property::Ptr>(m_propertyList);
}

Property::Ptr
Component::registerProperty(Property * theProperty) 
{
    Property::Ptr myProperty(theProperty);
    m_propertyList.push_back(myProperty);
    
    return myProperty;
}

