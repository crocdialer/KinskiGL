// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include <crocore/crocore.hpp>
#include <crocore/json.hpp>
#include "Component.hpp"

namespace crocore {

/**
 * @brief   PropertyIO is a delegate object to handle all known types
 *          Can be provided by user to add support for arbitrary data formats
 */
class PropertyIO
{
public:

    static const std::string PROPERTY_TYPE;
    static const std::string PROPERTY_VALUE;
    static const std::string PROPERTY_TYPE_FLOAT;
    static const std::string PROPERTY_TYPE_STRING;
    static const std::string PROPERTY_TYPE_DOUBLE;
    static const std::string PROPERTY_TYPE_BOOLEAN;
    static const std::string PROPERTY_TYPE_INT;
    static const std::string PROPERTY_TYPE_UINT;
    static const std::string PROPERTY_TYPE_FLOAT_ARRAY;
    static const std::string PROPERTY_TYPE_STRING_ARRAY;
    static const std::string PROPERTY_TYPE_INT_ARRAY;
    static const std::string PROPERTY_TYPE_UINT_ARRAY;
    static const std::string PROPERTY_TYPE_UNKNOWN;
    static const std::string PROPERTY_NAME;
    static const std::string PROPERTIES;

    virtual bool read_property(const PropertyConstPtr &theProperty, crocore::json &theJsonValue) const;

    virtual bool write_property(PropertyPtr &theProperty, const crocore::json &theJsonValue) const;
};

namespace serializer {

bool is_valid_json(const std::string &the_string);

/*!
 * Save a single component´s state to file using json file formatting
 */
void save_state(const ComponentPtr &theComponent,
                const std::string &theFileName,
                const PropertyIO &theIO = PropertyIO());

/*!
 * Save multiple component´s states to file using json file formatting
 */
void save_state(const std::list<ComponentPtr> &theComponentList,
                const std::string &theFileName,
                const PropertyIO &theIO = PropertyIO());

/*!
 * Read a component´s state from a json-file
 */
void load_state(const ComponentPtr &theComponent,
                const std::string &theFileName,
                const PropertyIO &theIO = PropertyIO());

void load_state(const std::list<ComponentPtr> &theComponentList,
                const std::string &theFileName,
                const PropertyIO &theIO = PropertyIO());

/*!
 * Serialize a component to a string in json format.
 * Supported Property types are determined by theIO object
 */
std::string serialize(const ComponentPtr &theComponent,
                      const PropertyIO &theIO = PropertyIO(),
                      bool ignore_non_tweakable = false);

/*!
 * Serialize a component to a string in json format.
 * Supported Property types are determined by theIO object
 */
std::string serialize(const std::list<ComponentPtr> &theComponentList,
                      const PropertyIO &theIO = PropertyIO(),
                      bool ignore_non_tweakable = false);

/*!
 * Read a component´s state from a string in json-format
 * Supported Property types are determined by theIO object
 */
void apply_state(const ComponentPtr &theComponent,
                 const std::string &theState,
                 const PropertyIO &theIO = PropertyIO());

/*!
 * Read a component´s state from a provided crocore::json object
 * Supported Property types are determined by theIO object
 */
void apply_state(const ComponentPtr &theComponent,
                 const crocore::json &state,
                 const PropertyIO &theIO = PropertyIO());

/*!
 * Read a component´s state from a string in json-format
 * Supported Property types are determined by theIO object
 */
void apply_state(const std::list<ComponentPtr> &theComponentList,
                 const std::string &theState,
                 const PropertyIO &theIO = PropertyIO());

class ParsingException : public std::runtime_error
{
public:
    ParsingException(const std::string &theContentString) :
            std::runtime_error(std::string("Error while parsing json string: ") + theContentString) {}
};

}// namespace serializer
}// namespace crocore
