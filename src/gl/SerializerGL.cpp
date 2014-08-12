// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "SerializerGL.h"

namespace kinski {
    
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
        
        if (theProperty->isOfType<glm::vec2>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_VEC2;
            
            glm::vec2 vec = theProperty->getValue<glm::vec2>();
            float *ptr = &vec[0];

            for (int i = 0; i < 2; i++)
                theJsonValue[PROPERTY_VALUE][i] = ptr[i];

            success = true;
            
        }
        else if (theProperty->isOfType<glm::vec3>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_VEC3;
            
            glm::vec3 vec = theProperty->getValue<glm::vec3>();
            float *ptr = &vec[0];
            
            for (int i = 0; i < 3; i++)
                theJsonValue[PROPERTY_VALUE][i] = ptr[i];
            
            success = true;
        }
        else if (theProperty->isOfType<glm::vec4>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_VEC4;
            
            glm::vec4 vec = theProperty->getValue<glm::vec4>();
            float *ptr = &vec[0];
            
            for (int i = 0; i < 4; i++)
                theJsonValue[PROPERTY_VALUE][i] = ptr[i];

            
            success = true;
        }
        else if (theProperty->isOfType<glm::quat>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_QUAT;
            
            glm::quat vec = theProperty->getValue<glm::quat>();
            float *ptr = &vec[0];
            
            for (int i = 0; i < 4; i++)
                theJsonValue[PROPERTY_VALUE][i] = ptr[i];
            
            success = true;
        }
        else if (theProperty->isOfType<glm::mat3>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_MAT3;
            
            glm::mat3 mat = theProperty->getValue<glm::mat3>();
            float *ptr = &mat[0][0];
            
            for (int i = 0; i < 9; i++)
                theJsonValue[PROPERTY_VALUE][i] = ptr[i];
            
            success = true;
        }
        else if (theProperty->isOfType<glm::mat4>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_MAT4;
            
            glm::mat4 mat = theProperty->getValue<glm::mat4>();
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
            glm::vec2 vec;
            float *ptr = &vec[0];
            
            for (int i = 0; i < 2; i++)
                ptr[i] = theJsonValue[PROPERTY_VALUE][i].asDouble();
   
            theProperty->setValue<glm::vec2>(vec);
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_VEC3)
        {
            glm::vec3 vec;
            float *ptr = &vec[0];
            
            for (int i = 0; i < 3; i++)
                ptr[i] = theJsonValue[PROPERTY_VALUE][i].asDouble();
            
            theProperty->setValue<glm::vec3>(vec);
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_VEC4)
        {
            glm::vec4 vec;
            float *ptr = &vec[0];
            
            for (int i = 0; i < 4; i++)
                ptr[i] = theJsonValue[PROPERTY_VALUE][i].asDouble();
            
            theProperty->setValue<glm::vec4>(vec);
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_QUAT)
        {
            glm::quat vec;
            float *ptr = &vec[0];
            
            for (int i = 0; i < 4; i++)
                ptr[i] = theJsonValue[PROPERTY_VALUE][i].asDouble();
            
            theProperty->setValue<glm::quat>(vec);
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_MAT3)
        {
            glm::mat3 mat;
            float *ptr = &mat[0][0];
            
            for (int i = 0; i < 9; i++)
                ptr[i] = theJsonValue[PROPERTY_VALUE][i].asDouble();
            
            theProperty->setValue<glm::mat3>(mat);
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_MAT4)
        {
            glm::mat4 mat;
            float *ptr = &mat[0][0];
            
            for (int i = 0; i < 16; i++)
                ptr[i] = theJsonValue[PROPERTY_VALUE][i].asDouble();
            
            theProperty->setValue<glm::mat4>(mat);
            success = true;
            
        }
        
        return success;
    }
}