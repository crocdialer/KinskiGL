// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "core/Serializer.hpp"
#include "gl/gl.hpp"

namespace kinski
{
    /*!
     * Delegate object to handle all known types
     * Can be provided by user to add support for arbitrary data formats
     */
    class KINSKI_API PropertyIO_GL : public PropertyIO
    {
    public:
        static const std::string PROPERTY_TYPE_VEC2;
        static const std::string PROPERTY_TYPE_VEC3;
        static const std::string PROPERTY_TYPE_VEC4;
        static const std::string PROPERTY_TYPE_QUAT;
        static const std::string PROPERTY_TYPE_MAT3;
        static const std::string PROPERTY_TYPE_MAT4;
        
        static const std::string PROPERTY_TYPE_VEC2_ARRAY;
        
        virtual ~PropertyIO_GL(){};
        virtual bool read_property(const Property::ConstPtr &theProperty,
                                       Json::Value &theJsonValue) const;
        virtual bool write_property(Property::Ptr &theProperty,
                                        const Json::Value &theJsonValue) const;
    };
    
}//namespace
