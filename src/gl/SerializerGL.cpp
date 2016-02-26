// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "SerializerGL.hpp"

namespace kinski {
    
    using namespace gl;
    
    const std::string PropertyIO_GL::PROPERTY_TYPE_VEC2 = "vec2";
    const std::string PropertyIO_GL::PROPERTY_TYPE_VEC3 = "vec3";
    const std::string PropertyIO_GL::PROPERTY_TYPE_VEC4 = "vec4";
    const std::string PropertyIO_GL::PROPERTY_TYPE_QUAT = "quat";
    const std::string PropertyIO_GL::PROPERTY_TYPE_MAT3 = "mat3";
    const std::string PropertyIO_GL::PROPERTY_TYPE_MAT4 = "mat4";
    
    bool PropertyIO_GL::readPropertyValue(const Property::ConstPtr &theProperty,
                                          Json::Value &theJsonValue) const
    {
        bool success = PropertyIO::readPropertyValue(theProperty, theJsonValue);
        
        if(success) return true;
        
        if (theProperty->isOfType<vec2>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_VEC2;
            
            vec2 vec = theProperty->getValue<vec2>();
            float *ptr = &vec[0];

            for (int i = 0; i < 2; i++)
                theJsonValue[PROPERTY_VALUE][i] = ptr[i];

            success = true;
            
        }
        else if (theProperty->isOfType<vec3>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_VEC3;
            
            vec3 vec = theProperty->getValue<vec3>();
            float *ptr = &vec[0];
            
            for (int i = 0; i < 3; i++)
                theJsonValue[PROPERTY_VALUE][i] = ptr[i];
            
            success = true;
        }
        else if (theProperty->isOfType<vec4>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_VEC4;
            
            vec4 vec = theProperty->getValue<vec4>();
            float *ptr = &vec[0];
            
            for (int i = 0; i < 4; i++)
                theJsonValue[PROPERTY_VALUE][i] = ptr[i];

            
            success = true;
        }
        else if (theProperty->isOfType<quat>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_QUAT;
            
            quat vec = theProperty->getValue<quat>();
            float *ptr = &vec[0];
            
            for (int i = 0; i < 4; i++)
                theJsonValue[PROPERTY_VALUE][i] = ptr[i];
            
            success = true;
        }
        else if (theProperty->isOfType<mat3>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_MAT3;
            
            mat3 mat = theProperty->getValue<mat3>();
            float *ptr = &mat[0][0];
            
            for (int i = 0; i < 9; i++)
                theJsonValue[PROPERTY_VALUE][i] = ptr[i];
            
            success = true;
        }
        else if (theProperty->isOfType<mat4>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_MAT4;
            
            mat4 mat = theProperty->getValue<mat4>();
            float *ptr = &mat[0][0];
            
            for (int i = 0; i < 16; i++)
                theJsonValue[PROPERTY_VALUE][i] = ptr[i];
            
            success = true;
        }
        
        return success;
    }
    
    bool PropertyIO_GL::writePropertyValue(Property::Ptr &theProperty,
                                           const Json::Value &theJsonValue) const
    {
        bool success = PropertyIO::writePropertyValue(theProperty, theJsonValue);
        
        if(success) return true;
        
        if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_VEC2)
        {
            vec2 vec;
            float *ptr = &vec[0];
            
            for (int i = 0; i < 2; i++)
                ptr[i] = theJsonValue[PROPERTY_VALUE][i].asDouble();
   
            theProperty->setValue<vec2>(vec);
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_VEC3)
        {
            vec3 vec;
            float *ptr = &vec[0];
            
            for (int i = 0; i < 3; i++)
                ptr[i] = theJsonValue[PROPERTY_VALUE][i].asDouble();
            
            theProperty->setValue<vec3>(vec);
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_VEC4)
        {
            vec4 vec;
            float *ptr = &vec[0];
            
            for (int i = 0; i < 4; i++)
                ptr[i] = theJsonValue[PROPERTY_VALUE][i].asDouble();
            
            theProperty->setValue<vec4>(vec);
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_QUAT)
        {
            quat vec;
            float *ptr = &vec[0];
            
            for (int i = 0; i < 4; i++)
                ptr[i] = theJsonValue[PROPERTY_VALUE][i].asDouble();
            
            theProperty->setValue<quat>(vec);
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_MAT3)
        {
            mat3 mat;
            float *ptr = &mat[0][0];
            
            for (int i = 0; i < 9; i++)
                ptr[i] = theJsonValue[PROPERTY_VALUE][i].asDouble();
            
            theProperty->setValue<mat3>(mat);
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_MAT4)
        {
            mat4 mat;
            float *ptr = &mat[0][0];
            
            for (int i = 0; i < 16; i++)
                ptr[i] = theJsonValue[PROPERTY_VALUE][i].asDouble();
            
            theProperty->setValue<mat4>(mat);
            success = true;
            
        }
        
        return success;
    }
}