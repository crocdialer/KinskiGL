// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "serializer.hpp"
#include "crocore/filesystem.hpp"
#include "nlohmann/json.hpp"

using namespace std;

namespace crocore {

const std::string PropertyIO::PROPERTY_TYPE = "type";
const std::string PropertyIO::PROPERTY_VALUE = "value";
const std::string PropertyIO::PROPERTY_TYPE_FLOAT = "float";
const std::string PropertyIO::PROPERTY_TYPE_STRING = "string";
const std::string PropertyIO::PROPERTY_TYPE_DOUBLE = "double";
const std::string PropertyIO::PROPERTY_TYPE_BOOLEAN = "bool";
const std::string PropertyIO::PROPERTY_TYPE_INT = "int";
const std::string PropertyIO::PROPERTY_TYPE_UINT = "uint";
const std::string PropertyIO::PROPERTY_TYPE_FLOAT_ARRAY = "float_array";
const std::string PropertyIO::PROPERTY_TYPE_STRING_ARRAY = "string_array";
const std::string PropertyIO::PROPERTY_TYPE_INT_ARRAY = "int_array";
const std::string PropertyIO::PROPERTY_TYPE_UINT_ARRAY = "uint_array";
const std::string PropertyIO::PROPERTY_TYPE_UNKNOWN = "unknown";
const std::string PropertyIO::PROPERTY_NAME = "name";
const std::string PropertyIO::PROPERTIES = "properties";

bool PropertyIO::read_property(const PropertyConstPtr &theProperty,
                               crocore::json &theJsonValue) const
{
    bool success = false;

    if(theProperty->is_of_type<float>())
    {
        theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_FLOAT;
        theJsonValue[PROPERTY_VALUE] = theProperty->get_value<float>();
        success = true;

    }else if(theProperty->is_of_type<std::string>())
    {
        theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_STRING;
        theJsonValue[PROPERTY_VALUE] = theProperty->get_value<std::string>();
        success = true;

    }else if(theProperty->is_of_type<int>())
    {
        theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_INT;
        theJsonValue[PROPERTY_VALUE] = theProperty->get_value<int>();
        success = true;

    }else if(theProperty->is_of_type<uint32_t>())
    {
        theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_UINT;
        theJsonValue[PROPERTY_VALUE] = theProperty->get_value<uint32_t>();
        success = true;

    }else if(theProperty->is_of_type<double>())
    {
        theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_DOUBLE;
        theJsonValue[PROPERTY_VALUE] = theProperty->get_value<double>();
        success = true;

    }else if(theProperty->is_of_type<bool>())
    {
        theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_BOOLEAN;
        theJsonValue[PROPERTY_VALUE] = theProperty->get_value<bool>();
        success = true;

    }else if(theProperty->is_of_type<std::vector<float>>())
    {
        theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_FLOAT_ARRAY;
        theJsonValue[PROPERTY_VALUE] = theProperty->get_value<std::vector<float>>();;
        success = true;
    }else if(theProperty->is_of_type<std::vector<int>>())
    {
        theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_INT_ARRAY;
        theJsonValue[PROPERTY_VALUE] = theProperty->get_value<std::vector<int>>();
        success = true;
    }else if(theProperty->is_of_type<std::vector<uint32_t>>())
    {
        theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_UINT_ARRAY;
        theJsonValue[PROPERTY_VALUE] = theProperty->get_value<std::vector<uint32_t>>();
        success = true;
    }else if(theProperty->is_of_type<std::vector<std::string>>())
    {
        theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_STRING_ARRAY;
        theJsonValue[PROPERTY_VALUE] = theProperty->get_value<std::vector<std::string>>();
        success = true;
    }
    return success;
}

bool PropertyIO::write_property(PropertyPtr &theProperty,
                                const crocore::json &theJsonValue) const
{
    bool success = false;

    if(theJsonValue[PROPERTY_TYPE] == PROPERTY_TYPE_FLOAT)
    {
        theProperty->set_value<float>(theJsonValue[PROPERTY_VALUE]);
        success = true;

    }else if(theJsonValue[PROPERTY_TYPE] == PROPERTY_TYPE_DOUBLE)
    {
        theProperty->set_value<double>(theJsonValue[PROPERTY_VALUE]);
        success = true;

    }else if(theJsonValue[PROPERTY_TYPE] == PROPERTY_TYPE_INT)
    {
        theProperty->set_value<int>(theJsonValue[PROPERTY_VALUE]);
        success = true;

    }else if(theJsonValue[PROPERTY_TYPE] == PROPERTY_TYPE_UINT)
    {
        theProperty->set_value<uint32_t>(theJsonValue[PROPERTY_VALUE]);
        success = true;

    }else if(theJsonValue[PROPERTY_TYPE] == PROPERTY_TYPE_STRING)
    {
        theProperty->set_value<std::string>(theJsonValue[PROPERTY_VALUE]);
        success = true;

    }else if(theJsonValue[PROPERTY_TYPE] == PROPERTY_TYPE_BOOLEAN)
    {
        theProperty->set_value<bool>(theJsonValue[PROPERTY_VALUE]);
        success = true;

    }else if(theJsonValue[PROPERTY_TYPE] == PROPERTY_TYPE_FLOAT_ARRAY)
    {
        if(theJsonValue.contains(PROPERTY_VALUE))
        {
            std::vector<float> vals = theJsonValue[PROPERTY_VALUE];

            theProperty->set_value<std::vector<float>>(vals);
        }
        success = true;

    }else if(theJsonValue[PROPERTY_TYPE] == PROPERTY_TYPE_STRING_ARRAY)
    {
        if(theJsonValue.contains(PROPERTY_VALUE))
        {
            const std::vector<std::string> &vals = theJsonValue[PROPERTY_VALUE];
            theProperty->set_value<std::vector<std::string>>(vals);
            success = true;
        }

    }else if(theJsonValue[PROPERTY_TYPE] == PROPERTY_TYPE_INT_ARRAY)
    {
        if(theJsonValue.contains(PROPERTY_VALUE))
        {
            std::vector<int> vals = theJsonValue[PROPERTY_VALUE];
            theProperty->set_value<std::vector<int>>(vals);
        }
        success = true;

    }else if(theJsonValue[PROPERTY_TYPE] == PROPERTY_TYPE_UINT_ARRAY)
    {
        if(theJsonValue.contains(PROPERTY_VALUE))
        {
            std::vector<uint32_t> vals = theJsonValue[PROPERTY_VALUE];
            theProperty->set_value<std::vector<uint32_t>>(vals);
        }
        success = true;

    }else if(theJsonValue[PROPERTY_TYPE] == PROPERTY_TYPE_UNKNOWN)
    {
        // do nothing
        spdlog::warn("property type unknown");
    }

    return success;
}

namespace serializer {

void add_to_json_object(const std::list<ComponentPtr> &theComponentList,
                        crocore::json &json_val,
                        const PropertyIO &theIO = PropertyIO(),
                        bool ignore_non_tweakable = false);

bool is_valid_json(const std::string &the_string)
{
    return json::parse(the_string) != json::value_t::discarded;
}

void add_to_json_object(const std::list<ComponentPtr> &theComponentList,
                        crocore::json &json_val,
                        const PropertyIO &theIO,
                        bool ignore_non_tweakable)
{
    int index = 0;
    int value_index = 0;

    for(const auto &theComponent : theComponentList)
    {
        json_val[index][PropertyIO::PROPERTY_NAME] = theComponent->name();

        for(const auto &property : theComponent->get_property_list())
        {
            // skip non-tweakable properties, if requested
            if(!property->tweakable() && ignore_non_tweakable){ continue; }

            json_val[index][PropertyIO::PROPERTIES][value_index][PropertyIO::PROPERTY_NAME] = property->name();

            // delegate reading to PropertyIO object
            if(!theIO.read_property(property, json_val[index][PropertyIO::PROPERTIES][value_index]))
            {
                json_val[index][PropertyIO::PROPERTIES][value_index][PropertyIO::PROPERTY_TYPE] =
                        PropertyIO::PROPERTY_TYPE_UNKNOWN;
            }
            value_index++;
        }
        index++;
        value_index = 0;
    }
}

std::string serialize(const ComponentPtr &theComponent,
                      const PropertyIO &theIO,
                      bool ignore_non_tweakable)
{
    json root;
    add_to_json_object({theComponent}, root, theIO, ignore_non_tweakable);
    return root.dump(2);
}

std::string serialize(const std::list<ComponentPtr> &theComponentList,
                      const PropertyIO &theIO,
                      bool ignore_non_tweakable)
{
    json root;
    add_to_json_object(theComponentList, root, theIO, ignore_non_tweakable);
    return root.dump(2);
}

void apply_state(const ComponentPtr &theComponent, const std::string &theState,
                 const PropertyIO &theIO)
{
    auto root = json::parse(theState);
    bool myParsingSuccessful = root != json::value_t::discarded;

    if(!myParsingSuccessful)
    {
        throw ParsingException(theState);
    }
    apply_state(theComponent, root, theIO);

}

void apply_state(const ComponentPtr &theComponent,
                 const crocore::json &state,
                 const PropertyIO &theIO)
{
    for(unsigned int i = 0; i < state.size(); i++)
    {
        json myComponentNode = state[i];
        if(myComponentNode[PropertyIO::PROPERTY_NAME] != theComponent->name()){ continue; }

        for(unsigned int j = 0; j < myComponentNode[PropertyIO::PROPERTIES].size(); j++)
        {
            try
            {
                std::string myName = myComponentNode[PropertyIO::PROPERTIES][j][PropertyIO::PROPERTY_NAME];

                PropertyPtr myProperty = theComponent->get_property_by_name(myName);
                theIO.write_property(myProperty, myComponentNode[PropertyIO::PROPERTIES][j]);

            }catch(PropertyNotFoundException &myException)
            {
                spdlog::warn(myException.what());
            }
        }
    }
}

void apply_state(const std::list<ComponentPtr> &theComponentList, const std::string &theState,
                 const PropertyIO &theIO)
{
    auto root = json::parse(theState);
    bool success = root != json::value_t::discarded;
    if(!success){ throw ParsingException(theState); }

    for(auto c : theComponentList){ apply_state(c, root, theIO); }
}

void save_state(const ComponentPtr &theComponent,
                const std::string &theFileName,
                const PropertyIO &theIO)
{
    std::string state = serialize(theComponent, theIO);
    fs::write_file(theFileName, state);
}

void save_state(const std::list<ComponentPtr> &theComponentList,
                const std::string &theFileName,
                const PropertyIO &theIO)
{
    json root;
    add_to_json_object(theComponentList, root, theIO);
    fs::write_file(theFileName, root.dump(2));
}

void load_state(const ComponentPtr &theComponent,
                const std::string &theFileName,
                const PropertyIO &theIO)
{
    if(!theComponent) return;
    std::string myState = fs::read_file(theFileName);
    apply_state(theComponent, myState, theIO);
}

void load_state(const std::list<ComponentPtr> &theComponentList,
                const std::string &theFileName,
                const PropertyIO &theIO)
{
    if(theComponentList.empty()) return;

    // read file
    std::string state_str = fs::read_file(theFileName);

    // parse string
    auto root = json::parse(state_str);
    bool success = root != json::value_t::discarded;
    if(!success){ throw ParsingException(state_str); }

    // apply state
    for(auto &component : theComponentList){ apply_state(component, root, theIO); }
}

}
}//namespace
