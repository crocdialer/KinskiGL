// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef __gl__SerializerGL__
#define __gl__SerializerGL__

#include "core/Serializer.h"
#include "gl/gl.h"

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
        
        virtual ~PropertyIO_GL(){};
        virtual bool readPropertyValue(const Property::ConstPtr &theProperty,
                                       Json::Value &theJsonValue) const;
        virtual bool writePropertyValue(Property::Ptr &theProperty,
                                        const Json::Value &theJsonValue) const;
    };
    
}//namespace

#endif /* defined(__gl__SerializerGL__) */
