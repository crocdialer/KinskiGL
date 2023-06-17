// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "Property.hpp"
#include <list>
#include <map>

namespace crocore {

DEFINE_CLASS_PTR(Component);

class Component : public std::enable_shared_from_this<Component>,
                  public Property::Observer
{
public:

    typedef std::function<void(const std::vector<std::string> &)> Functor;

    explicit Component(const std::string &theName = "Component");

    virtual ~Component();

    void set_name(const std::string &n) { m_name = n; };

    const std::string &name() const { return m_name; };

    const std::list<PropertyPtr> &get_property_list() const;

    PropertyPtr get_property_by_name(const std::string &thePropertyName);

    /*!
     * inherited from Property::Observer and called whenever a registered property changes.
     * override this in a subclass
     */
    void update_property(const PropertyConstPtr &theProperty) override {};

    void observe_properties(bool b = true);

    void observe_properties(const std::list<PropertyPtr> &theProps, bool b = true);

    void register_property(PropertyPtr theProperty);

    void unregister_property(PropertyPtr theProperty);

    void unregister_all_properties();

    bool call_function(const std::string &the_function_name,
                       const std::vector<std::string> &the_params = {});

    void register_function(const std::string &the_name, Functor the_functor);

    void unregister_function(const std::string &the_name);

    void unregister_all_functions();

    std::map<std::string, Functor> &function_map() { return m_function_map; };

private:

    std::string m_name;
    std::list<PropertyPtr> m_propertyList;
    std::map<std::string, Functor> m_function_map;

};

// Exception definition
class PropertyNotFoundException : public std::runtime_error
{
public:
    PropertyNotFoundException(std::string thePropertyName) :
            std::runtime_error(std::string("Property not found: ") + thePropertyName) {}
};
}
