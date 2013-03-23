// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef __KINSKI_COMPONENT_INCLUDED__
#define __KINSKI_COMPONENT_INCLUDED__

#include "Property.h"

namespace kinski 
{
    
    class KINSKI_API Component : public std::enable_shared_from_this<Component>,
    public Property::Observer
    {
    public:
        typedef std::shared_ptr<Component> Ptr;
        typedef std::shared_ptr<const Component> ConstPtr;
        
        Component(const std::string &theName = "Component");
        virtual ~Component();
        
        void set_name(const std::string &n) { m_name = n; };
        const std::string& getName() const { return m_name; };
        const std::list<Property::Ptr>& getPropertyList() const;
        Property::Ptr getPropertyByName(const std::string & thePropertyName);

        virtual void updateProperty(const Property::ConstPtr &theProperty){};
        void observeProperties(bool b = true);
        
    protected:        
        
        void registerProperty(Property::Ptr theProperty);

    private:
        
        std::string m_name;
        std::list<Property::Ptr> m_propertyList;
        
    };

    // Exception definitions. TODO: put those to some neat macros
    class PropertyNotFoundException : public Exception
    {
    public:
        PropertyNotFoundException(std::string thePropertyName): 
        Exception(std::string("Named Property not found: ")+thePropertyName)
        {}
    }; 
    
    class ComponentError: public Exception
    {
    public:
        ComponentError(std::string theErrorString):
        Exception(std::string("ComponentError: ")+theErrorString)
        {}
    }; 
}

#endif // __KINSKI_COMPONENT_INCLUDED__
