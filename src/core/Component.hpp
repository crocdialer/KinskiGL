// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "Property.hpp"

namespace kinski 
{
    
    class KINSKI_API Component : public std::enable_shared_from_this<Component>,
    public Property::Observer
    {
    public:
        typedef std::shared_ptr<Component> Ptr;
        typedef std::shared_ptr<const Component> ConstPtr;
        typedef std::weak_ptr<Component> WeakPtr;
        
        typedef std::function<void(const std::vector<std::string>&)> Functor;
        
        Component(const std::string &theName = "Component");
        virtual ~Component();
        
        void set_name(const std::string &n) { m_name = n; };
        const std::string& name() const { return m_name; };
        const std::list<Property::Ptr>& get_property_list() const;
        Property::Ptr get_property_by_name(const std::string & thePropertyName);

        /*!
         * called whenever a registered propterty changes
         * override this in a subclass
         */
        virtual void update_property(const Property::ConstPtr &theProperty){};
        
        void observe_properties(bool b = true);
        void observe_properties(const std::list<Property::Ptr>& theProps,  bool b = true);
        
        void register_property(Property::Ptr theProperty);
        void unregister_property(Property::Ptr theProperty);
        void unregister_all_properties();
        
        bool call_function(const std::string &the_function_name,
                           const std::vector<std::string> &the_params = {});
        void register_function(const std::string &the_name, Functor the_functor);
        void unregister_function(const std::string &the_name, Functor the_functor);
        void unregister_all_functions();
        
        std::map<std::string, Functor>& function_map(){ return m_function_map; };
        
    private:
        
        std::string m_name;
        std::list<Property::Ptr> m_propertyList;
        std::map<std::string, Functor> m_function_map;
        
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