// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "json/json.h"
#include "Component.hpp"

namespace kinski
{
    /*!
     * Delegate object to handle all known types
     * Can be provided by user to add support for arbitrary data formats
     */
    class KINSKI_API PropertyIO
    {
    public:
        
        static const std::string PROPERTY_TYPE;
        static const std::string PROPERTY_VALUE;
        static const std::string PROPERTY_VALUES;
        static const std::string PROPERTY_TYPE_FLOAT;
        static const std::string PROPERTY_TYPE_STRING;
        static const std::string PROPERTY_TYPE_DOUBLE;
        static const std::string PROPERTY_TYPE_BOOLEAN;
        static const std::string PROPERTY_TYPE_INT;
        static const std::string PROPERTY_TYPE_UINT;
        static const std::string PROPERTY_TYPE_STRING_ARRAY;
        static const std::string PROPERTY_TYPE_UNKNOWN;
        static const std::string PROPERTY_NAME;
        static const std::string PROPERTIES;
        
        virtual ~PropertyIO(){};
        virtual bool readPropertyValue(const Property::ConstPtr &theProperty,
                                       Json::Value &theJsonValue) const;
        virtual bool writePropertyValue(Property::Ptr &theProperty,
                                        const Json::Value &theJsonValue) const;
    };
    
    class KINSKI_API Serializer
    {
    public:
        /*!
         * Save a single component´s state to file using json file formatting
         */
        static void saveComponentState(const Component::Ptr &theComponent,
                                       const std::string &theFileName,
                                       const PropertyIO &theIO = PropertyIO());
        
        /*!
         * Save multiple component´s states to file using json file formatting
         */
        static void saveComponentState(const std::list<Component::Ptr> &theComponentList,
                                const std::string &theFileName,
                                const PropertyIO &theIO = PropertyIO());
        
        /*!
         * Read a component´s state from a json-file
         */
        static void loadComponentState(const Component::Ptr &theComponent,
                                       const std::string &theFileName,
                                       const PropertyIO &theIO = PropertyIO());
        
        static void loadComponentState(const std::list<Component::Ptr> &theComponentList,
                                       const std::string &theFileName,
                                       const PropertyIO &theIO = PropertyIO());
        
        /*!
         * Serialize a component to a string in json format.
         * Supported Property types are determined by theIO object
         */
        static std::string serializeComponent(const Component::Ptr &theComponent,
                                              const PropertyIO &theIO = PropertyIO());
        
        /*!
         * Serialize a component to a string in json format.
         * Supported Property types are determined by theIO object
         */
        static std::string serializeComponents(const std::list<Component::Ptr> &theComponentList,
                                               const PropertyIO &theIO = PropertyIO());
        
        /*!
         * Read a component´s state from a string in json-format
         * Supported Property types are determined by theIO object
         */
        static void applyStateToComponent(const Component::Ptr &theComponent,
                                          const std::string &theState,
                                          const PropertyIO &theIO = PropertyIO());
        
        /*!
         * Read a component´s state from a string in json-format
         * Supported Property types are determined by theIO object
         */
        static void applyStateToComponents(const std::list<Component::Ptr> &theComponentList,
                                           const std::string &theState,
                                           const PropertyIO &theIO = PropertyIO());
        
        class ParsingException: public Exception
        {
        public:
            ParsingException(const std::string &theContentString) :
            Exception(std::string("Error while parsing json string: ") + theContentString) {}
        };
    private:
        static void add_to_json_object(const std::list<Component::Ptr> &theComponentList,
                                       Json::Value &json_val,
                                       const PropertyIO &theIO = PropertyIO());
    };
    
    /************************ Exceptions ************************/
    
    class FileReadingException: public Exception
    {
    public:
        FileReadingException(const std::string &theFilename) :
        Exception(std::string("Error while reading file: ") + theFilename) {}
    };
    
    class OutputFileException: public Exception
    {
    public:
        OutputFileException(const std::string &theFilename) :
        Exception(std::string("Could not open file for writing configuration: ") + theFilename) {}
    };
}