// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
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
        static const std::string PROPERTY_TYPE_FLOAT_ARRAY;
        static const std::string PROPERTY_TYPE_STRING_ARRAY;
        static const std::string PROPERTY_TYPE_INT_ARRAY;
        static const std::string PROPERTY_TYPE_UINT_ARRAY;
        static const std::string PROPERTY_TYPE_UNKNOWN;
        static const std::string PROPERTY_NAME;
        static const std::string PROPERTIES;
        
        virtual ~PropertyIO(){};
        virtual bool read_property(const Property::ConstPtr &theProperty,
                                   Json::Value &theJsonValue) const;
        virtual bool write_property(Property::Ptr &theProperty,
                                    const Json::Value &theJsonValue) const;
    };
    
    namespace json
    {
        
        KINSKI_API bool is_valid(const std::string &the_string);
        
        /*!
         * Save a single component´s state to file using json file formatting
         */
        KINSKI_API void save_state(const ComponentPtr &theComponent,
                                   const std::string &theFileName,
                                   const PropertyIO &theIO = PropertyIO());
        
        /*!
         * Save multiple component´s states to file using json file formatting
         */
        KINSKI_API void save_state(const std::list<ComponentPtr> &theComponentList,
                                   const std::string &theFileName,
                                   const PropertyIO &theIO = PropertyIO());
        
        /*!
         * Read a component´s state from a json-file
         */
        KINSKI_API void load_state(const ComponentPtr &theComponent,
                                   const std::string &theFileName,
                                   const PropertyIO &theIO = PropertyIO());
        
        KINSKI_API void load_state(const std::list<ComponentPtr> &theComponentList,
                                   const std::string &theFileName,
                                   const PropertyIO &theIO = PropertyIO());
        
        /*!
         * Serialize a component to a string in json format.
         * Supported Property types are determined by theIO object
         */
        KINSKI_API std::string serialize(const ComponentPtr &theComponent,
                                         const PropertyIO &theIO = PropertyIO(),
                                         bool ignore_non_tweakable = false);
        
        /*!
         * Serialize a component to a string in json format.
         * Supported Property types are determined by theIO object
         */
        KINSKI_API std::string serialize(const std::list<ComponentPtr> &theComponentList,
                                         const PropertyIO &theIO = PropertyIO(),
                                         bool ignore_non_tweakable = false);
        
        /*!
         * Read a component´s state from a string in json-format
         * Supported Property types are determined by theIO object
         */
        KINSKI_API void apply_state(const ComponentPtr &theComponent,
                                    const std::string &theState,
                                    const PropertyIO &theIO = PropertyIO());
        
        /*!
         * Read a component´s state from a string in json-format
         * Supported Property types are determined by theIO object
         */
        KINSKI_API void apply_state(const std::list<ComponentPtr> &theComponentList,
                                    const std::string &theState,
                                    const PropertyIO &theIO = PropertyIO());
        
        class ParsingException: public Exception
        {
        public:
            ParsingException(const std::string &theContentString) :
            Exception(std::string("Error while parsing json string: ") + theContentString) {}
        };
    };
}
