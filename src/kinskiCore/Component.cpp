#include "Component.h"

namespace kinski{

Component::Component()
{
}

Component::~Component() 
{
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

const std::list<Property::Ptr>& 
Component::getPropertyList() const
{
    return m_propertyList;
}

void Component::registerProperty(Property::Ptr theProperty) 
{
    m_propertyList.push_back(theProperty);
}

};