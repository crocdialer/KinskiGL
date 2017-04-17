// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include <fstream>
#include "file_functions.hpp"
#include "Logger.hpp"
#include "Serializer.hpp"

using namespace std;

namespace kinski {
    
    const std::string PropertyIO::PROPERTY_TYPE = "type";
    const std::string PropertyIO::PROPERTY_VALUE = "value";
    const std::string PropertyIO::PROPERTY_VALUES = "values";
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
    
    bool PropertyIO::readPropertyValue(const Property::ConstPtr &theProperty,
                                       Json::Value &theJsonValue) const
    {
        bool success = false;
        
        if(theProperty->is_of_type<float>())
        {
            theJsonValue[PROPERTY_TYPE]  = PROPERTY_TYPE_FLOAT;
            theJsonValue[PROPERTY_VALUE] = theProperty->get_value<float>();
            success = true;
            
        }
        else if(theProperty->is_of_type<std::string>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_STRING;
            theJsonValue[PROPERTY_VALUE] = theProperty->get_value<std::string>();
            success = true;
            
        }
        else if(theProperty->is_of_type<int>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_INT;
            theJsonValue[PROPERTY_VALUE] = theProperty->get_value<int>();
            success = true;
            
        }
        else if(theProperty->is_of_type<uint32_t>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_UINT;
            theJsonValue[PROPERTY_VALUE] = theProperty->get_value<uint32_t>();
            success = true;
            
        }
        else if(theProperty->is_of_type<double>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_DOUBLE;
            theJsonValue[PROPERTY_VALUE] = theProperty->get_value<double>();
            success = true;
            
        }
        else if(theProperty->is_of_type<bool>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_BOOLEAN;
            theJsonValue[PROPERTY_VALUE] = theProperty->get_value<bool>();
            success = true;
            
        }
        else if(theProperty->is_of_type<std::vector<float>>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_FLOAT_ARRAY;
            const auto& vals = theProperty->get_value<std::vector<float>>();
            for (uint32_t i = 0; i < vals.size(); ++i)
            {
                theJsonValue[PROPERTY_VALUE][i] = vals[i];
            }
            success = true;
        }
        else if(theProperty->is_of_type<std::vector<int>>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_INT_ARRAY;
            const auto& vals = theProperty->get_value<std::vector<int>>();
            for (uint32_t i = 0; i < vals.size(); ++i)
            {
                theJsonValue[PROPERTY_VALUE][i] = vals[i];
            }
            success = true;
        }
        else if(theProperty->is_of_type<std::vector<uint32_t>>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_UINT_ARRAY;
            const auto& vals = theProperty->get_value<std::vector<uint32_t>>();
            for (uint32_t i = 0; i < vals.size(); ++i)
            {
                theJsonValue[PROPERTY_VALUE][i] = vals[i];
            }
            success = true;
        }
        else if(theProperty->is_of_type<std::vector<std::string>>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_STRING_ARRAY;
            const std::vector<std::string>& vals = theProperty->get_value<std::vector<std::string> >();
            for (uint32_t i = 0; i < vals.size(); ++i)
            {
                theJsonValue[PROPERTY_VALUE][i] = vals[i];
            }
            success = true;
        }
        return success;
    }
    
    bool PropertyIO::writePropertyValue(Property::Ptr &theProperty,
                                        const Json::Value &theJsonValue) const
    {
        bool success = false;
        
        if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_FLOAT)
        {
            theProperty->set_value<float>(theJsonValue[PROPERTY_VALUE].asDouble());
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_DOUBLE)
        {
            theProperty->set_value<double>(theJsonValue[PROPERTY_VALUE].asDouble());
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_INT)
        {
            theProperty->set_value<int>(theJsonValue[PROPERTY_VALUE].asInt());
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_UINT)
        {
            theProperty->set_value<uint32_t>(theJsonValue[PROPERTY_VALUE].asInt());
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_STRING)
        {
            theProperty->set_value<std::string>(theJsonValue[PROPERTY_VALUE].asString());
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_BOOLEAN)
        {
            theProperty->set_value<bool>(theJsonValue[PROPERTY_VALUE].asBool());
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_FLOAT_ARRAY)
        {
            if(theJsonValue[PROPERTY_VALUE].isArray())
            {
                std::vector<float> vals;
                for (uint32_t i = 0; i < theJsonValue[PROPERTY_VALUE].size(); ++i)
                {
                    vals.push_back(theJsonValue[PROPERTY_VALUE][i].asDouble());
                }
                theProperty->set_value<std::vector<float>>(vals);
            }
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_STRING_ARRAY)
        {
            if(theJsonValue[PROPERTY_VALUE].isArray())
            {
                std::vector<std::string> vals;
                for (uint32_t i = 0; i < theJsonValue[PROPERTY_VALUE].size(); ++i)
                {
                    vals.push_back(theJsonValue[PROPERTY_VALUE][i].asString());
                }
                theProperty->set_value<std::vector<std::string> >(vals);
            }
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_INT_ARRAY)
        {
            if(theJsonValue[PROPERTY_VALUE].isArray())
            {
                std::vector<int> vals;
                for (uint32_t i = 0; i < theJsonValue[PROPERTY_VALUE].size(); ++i)
                {
                    vals.push_back(theJsonValue[PROPERTY_VALUE][i].asInt());
                }
                theProperty->set_value<std::vector<int>>(vals);
            }
            success = true;

        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_UINT_ARRAY)
        {
            if(theJsonValue[PROPERTY_VALUE].isArray())
            {
                std::vector<uint32_t> vals;
                for (uint32_t i = 0; i < theJsonValue[PROPERTY_VALUE].size(); ++i)
                {
                    vals.push_back(theJsonValue[PROPERTY_VALUE][i].asUInt());
                }
                theProperty->set_value<std::vector<uint32_t>>(vals);
            }
            success = true;

        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_UNKNOWN)
        {
            // do nothing
            LOG_WARNING << "property type unknown";
        }
        
        return success;
    }
    
    bool Serializer::is_json(const std::string &the_string)
    {
        Json::Reader json_reader;
        Json::Value root;
        return json_reader.parse(the_string, root);
    }
    
    void Serializer::add_to_json_object(const std::list<ComponentPtr> &theComponentList,
                                        Json::Value &json_val,
                                        const PropertyIO &theIO)
    {
        int myIndex = 0;
        int myVIndex = 0;
        
        for (const auto &theComponent : theComponentList)
        {
            json_val[myIndex][PropertyIO::PROPERTY_NAME] = theComponent->name();
            
            for ( const auto &property : theComponent->get_property_list() )
            {
                json_val[myIndex][PropertyIO::PROPERTIES][myVIndex][PropertyIO::PROPERTY_NAME] = property->name();
                
                // delegate reading to PropertyIO object
                if(! theIO.readPropertyValue(property, json_val[myIndex][PropertyIO::PROPERTIES][myVIndex]))
                {
                    json_val[myIndex][PropertyIO::PROPERTIES][myVIndex][PropertyIO::PROPERTY_TYPE] =
                    PropertyIO::PROPERTY_TYPE_UNKNOWN;
                }
                myVIndex++;
            }
            myIndex++;
            myVIndex = 0;
        }
    }
    
    std::string Serializer::serializeComponent(const ComponentPtr &theComponent,
                                               const PropertyIO &theIO)
    {
        return serializeComponents({theComponent}, theIO);
    }
    
    std::string Serializer::serializeComponents(const std::list<ComponentPtr> &theComponentList,
                                                const PropertyIO &theIO)
    {
        Json::Value myRoot;
        add_to_json_object(theComponentList, myRoot, theIO);
        Json::StyledWriter myWriter;
        return myWriter.write(myRoot);
    }
    
    void Serializer::applyStateToComponent(const ComponentPtr &theComponent,
                                           const std::string &theState,
                                           const PropertyIO &theIO)
    {
        Json::Reader myReader;
        Json::Value myRoot;
        bool myParsingSuccessful = myReader.parse(theState, myRoot);
        
        if (!myParsingSuccessful)
        {
            throw ParsingException(theState);
        }
        
        for (unsigned int i=0; i<myRoot.size(); i++)
        {
            Json::Value myComponentNode = myRoot[i];
            if(myComponentNode[PropertyIO::PROPERTY_NAME] != theComponent->name()){continue;}
            
            for (unsigned int j=0; j < myComponentNode[PropertyIO::PROPERTIES].size(); j++)
            {
                try
                {
                    std::string myName =
                    myComponentNode[PropertyIO::PROPERTIES][j][PropertyIO::PROPERTY_NAME].asString();
                    
                    Property::Ptr myProperty = theComponent->get_property_by_name(myName);
                    theIO.writePropertyValue(myProperty, myComponentNode[PropertyIO::PROPERTIES][j]);
                    
                } catch (PropertyNotFoundException &myException)
                {
                    LOG_WARNING << myException.what();
                }
            }
        }
    }
    
    void Serializer::applyStateToComponents(const std::list<ComponentPtr> &theComponentList,
                                            const std::string &theState,
                                            const PropertyIO &theIO)
    {
        for(auto c : theComponentList){ applyStateToComponent(c, theState, theIO);}
    }
    
    void Serializer::saveComponentState(const ComponentPtr &theComponent,
                                        const std::string &theFileName,
                                        const PropertyIO &theIO)
    {
        std::string state = serializeComponent(theComponent, theIO);
        fs::write_file(theFileName, state);
    }
    
    void Serializer::saveComponentState(const std::list<ComponentPtr> &theComponentList,
                                        const std::string &theFileName,
                                        const PropertyIO &theIO)
    {
        Json::Value myRoot;
        add_to_json_object(theComponentList, myRoot, theIO);
        Json::StyledWriter myWriter;
        std::string state = myWriter.write(myRoot);
        fs::write_file(theFileName, state);
    }
    
    void Serializer::loadComponentState(const ComponentPtr &theComponent,
                                        const std::string &theFileName,
                                        const PropertyIO &theIO)
    {
        if(!theComponent) return;
        std::string myState = fs::read_file(theFileName);
        applyStateToComponent(theComponent, myState, theIO);
    }
    
    void Serializer::loadComponentState(const std::list<ComponentPtr> &theComponentList,
                                        const std::string &theFileName,
                                        const PropertyIO &theIO)
    {
        if(theComponentList.empty()) return;
        std::string myState = fs::read_file(theFileName);
        
        for (auto &component : theComponentList){applyStateToComponent(component, myState, theIO);}
    }

}//namespace
