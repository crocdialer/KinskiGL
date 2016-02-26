// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
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
    const std::string PropertyIO::PROPERTY_TYPE_STRING_ARRAY = "string_array";
    const std::string PropertyIO::PROPERTY_TYPE_UNKNOWN = "unknown";
    const std::string PropertyIO::PROPERTY_NAME = "name";
    const std::string PropertyIO::PROPERTIES = "properties";
    
    bool PropertyIO::readPropertyValue(const Property::ConstPtr &theProperty,
                                       Json::Value &theJsonValue) const
    {
        bool success = false;
        
        if (theProperty->isOfType<float>()) {
            theJsonValue[PROPERTY_TYPE]  = PROPERTY_TYPE_FLOAT;
            theJsonValue[PROPERTY_VALUE] = theProperty->getValue<float>();
            success = true;
            
        } else if (theProperty->isOfType<std::string>()) {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_STRING;
            theJsonValue[PROPERTY_VALUE] = theProperty->getValue<std::string>();
            success = true;
            
        } else if (theProperty->isOfType<int>()) {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_INT;
            theJsonValue[PROPERTY_VALUE] = theProperty->getValue<int>();
            success = true;
            
        } else if (theProperty->isOfType<uint32_t>()) {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_UINT;
            theJsonValue[PROPERTY_VALUE] = theProperty->getValue<uint32_t>();
            success = true;
            
        } else if (theProperty->isOfType<double>()) {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_DOUBLE;
            theJsonValue[PROPERTY_VALUE] = theProperty->getValue<double>();
            success = true;
            
        } else if (theProperty->isOfType<bool>()) {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_BOOLEAN;
            theJsonValue[PROPERTY_VALUE] = theProperty->getValue<bool>();
            success = true;
            
        } else if (theProperty->isOfType<std::vector<std::string> >()) {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_STRING_ARRAY;
            const std::vector<std::string>& vals = theProperty->getValue<std::vector<std::string> >();
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
            theProperty->setValue<float>(theJsonValue[PROPERTY_VALUE].asDouble());
            success = true;
            
        } else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_DOUBLE) {
            theProperty->setValue<double>(theJsonValue[PROPERTY_VALUE].asDouble());
            success = true;
            
        } else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_INT) {
            theProperty->setValue<int>(theJsonValue[PROPERTY_VALUE].asInt());
            success = true;
            
        } else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_UINT) {
            theProperty->setValue<uint32_t>(theJsonValue[PROPERTY_VALUE].asInt());
            success = true;
            
        } else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_STRING) {
            theProperty->setValue<std::string>(theJsonValue[PROPERTY_VALUE].asString());
            success = true;
            
        } else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_BOOLEAN) {
            theProperty->setValue<bool>(theJsonValue[PROPERTY_VALUE].asBool());
            success = true;
            
        }else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_STRING_ARRAY) {
            if(theJsonValue[PROPERTY_VALUE].isArray())
            {
                std::vector<std::string> vals;
                for (uint32_t i = 0; i < theJsonValue[PROPERTY_VALUE].size(); ++i)
                {
                    vals.push_back(theJsonValue[PROPERTY_VALUE][i].asString());
                }
                theProperty->setValue<std::vector<std::string> >(vals);
            }
            success = true;
            
        }else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_UNKNOWN) {
            // do nothing
        }
        
        return success;
    }
    
    void Serializer::add_to_json_object(const std::list<Component::Ptr> &theComponentList,
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
                json_val[myIndex][PropertyIO::PROPERTIES][myVIndex][PropertyIO::PROPERTY_NAME] = property->getName();
                
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
    
    std::string Serializer::serializeComponent(const Component::Ptr &theComponent,
                                               const PropertyIO &theIO)
    {
        return serializeComponents({theComponent}, theIO);
    }
    
    std::string Serializer::serializeComponents(const std::list<Component::Ptr> &theComponentList,
                                                const PropertyIO &theIO)
    {
        Json::Value myRoot;
        add_to_json_object(theComponentList, myRoot, theIO);
        Json::StyledWriter myWriter;
        return myWriter.write(myRoot);
    }
    
    void Serializer::applyStateToComponent(const Component::Ptr &theComponent,
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
            
            for (unsigned int i=0; i < myComponentNode[PropertyIO::PROPERTIES].size(); i++)
            {
                try
                {
                    std::string myName =
                    myComponentNode[PropertyIO::PROPERTIES][i][PropertyIO::PROPERTY_NAME].asString();
                    
                    Property::Ptr myProperty = theComponent->get_property_by_name(myName);
                    theIO.writePropertyValue(myProperty, myComponentNode[PropertyIO::PROPERTIES][i]);
                    
                } catch (PropertyNotFoundException &myException)
                {
                    LOG_WARNING << myException.what();
                }
            }
        }
    }
    
    void Serializer::applyStateToComponents(const std::list<Component::Ptr> &theComponentList,
                                            const std::string &theState,
                                            const PropertyIO &theIO)
    {
        for(auto c : theComponentList){ applyStateToComponent(c, theState, theIO);}
    }
    
    void Serializer::saveComponentState(const Component::Ptr &theComponent,
                                        const std::string &theFileName,
                                        const PropertyIO &theIO)
    {
        std::string state = serializeComponent(theComponent, theIO);
        std::ofstream myFileOut(theFileName.c_str());
        if(!myFileOut)
        {
            throw OutputFileException(theFileName);
        }
        myFileOut << state;
        myFileOut.close();
    }
    
    void Serializer::saveComponentState(const std::list<Component::Ptr> &theComponentList,
                                        const std::string &theFileName,
                                        const PropertyIO &theIO)
    {
        Json::Value myRoot;
        add_to_json_object(theComponentList, myRoot, theIO);
        Json::StyledWriter myWriter;
        std::string state = myWriter.write(myRoot);
        
        std::ofstream myFileOut(theFileName.c_str());
        if(!myFileOut)
        {
            throw OutputFileException(theFileName);
        }
        myFileOut << state;
        myFileOut.close();
    }
    
    void Serializer::loadComponentState(const Component::Ptr &theComponent,
                                        const std::string &theFileName,
                                        const PropertyIO &theIO)
    {
        if(!theComponent) return;
        std::string myState = read_file(theFileName);
        applyStateToComponent(theComponent, myState, theIO);
    }
    
    void Serializer::loadComponentState(const std::list<Component::Ptr> &theComponentList,
                                        const std::string &theFileName,
                                        const PropertyIO &theIO)
    {
        if(theComponentList.empty()) return;
        std::string myState = read_file(theFileName);
        
        for (auto &component : theComponentList){applyStateToComponent(component, myState, theIO);}
    }

}//namespace