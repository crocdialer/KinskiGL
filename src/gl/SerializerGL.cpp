// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
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
    
    const std::string PropertyIO_GL::PROPERTY_TYPE_VEC2_ARRAY = "vec2_array";
    
    bool PropertyIO_GL::read_property(const Property::ConstPtr &theProperty,
                                          Json::Value &theJsonValue) const
    {
        bool success = PropertyIO::read_property(theProperty, theJsonValue);
        
        if(success) return true;
        
        if (theProperty->is_of_type<vec2>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_VEC2;
            
            vec2 vec = theProperty->get_value<vec2>();
            float *ptr = &vec[0];

            for (int i = 0; i < 2; i++)
                theJsonValue[PROPERTY_VALUE][i] = ptr[i];

            success = true;
            
        }
        else if (theProperty->is_of_type<vec3>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_VEC3;
            
            vec3 vec = theProperty->get_value<vec3>();
            float *ptr = &vec[0];
            
            for (int i = 0; i < 3; i++)
                theJsonValue[PROPERTY_VALUE][i] = ptr[i];
            
            success = true;
        }
        else if (theProperty->is_of_type<vec4>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_VEC4;
            
            vec4 vec = theProperty->get_value<vec4>();
            float *ptr = &vec[0];
            
            for (int i = 0; i < 4; i++)
                theJsonValue[PROPERTY_VALUE][i] = ptr[i];

            
            success = true;
        }
        else if (theProperty->is_of_type<quat>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_QUAT;
            
            quat vec = theProperty->get_value<quat>();
            float *ptr = &vec[0];
            
            for (int i = 0; i < 4; i++)
                theJsonValue[PROPERTY_VALUE][i] = ptr[i];
            
            success = true;
        }
        else if (theProperty->is_of_type<mat3>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_MAT3;
            
            mat3 mat = theProperty->get_value<mat3>();
            float *ptr = &mat[0][0];
            
            for (int i = 0; i < 9; i++)
                theJsonValue[PROPERTY_VALUE][i] = ptr[i];
            
            success = true;
        }
        else if (theProperty->is_of_type<mat4>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_MAT4;
            
            mat4 mat = theProperty->get_value<mat4>();
            float *ptr = &mat[0][0];
            
            for (int i = 0; i < 16; i++)
                theJsonValue[PROPERTY_VALUE][i] = ptr[i];
            
            success = true;
        }
        else if(theProperty->is_of_type<std::vector<vec2>>())
        {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_VEC2_ARRAY;
            
            const std::vector<vec2> &vec_array = theProperty->get_value<std::vector<vec2>>();
            
            for(uint32_t j = 0; j < vec_array.size(); ++j)
            {
                for (int i = 0; i < 2; ++i)
                    theJsonValue[PROPERTY_VALUE][j][i] = vec_array[j][i];
            }
            
            success = true;
            
        }
        return success;
    }
    
    bool PropertyIO_GL::write_property(Property::Ptr &theProperty,
                                           const Json::Value &theJsonValue) const
    {
        bool success = PropertyIO::write_property(theProperty, theJsonValue);
        
        if(success) return true;
        
        if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_VEC2)
        {
            vec2 vec;
            float *ptr = &vec[0];
            
            for (int i = 0; i < 2; i++)
                ptr[i] = theJsonValue[PROPERTY_VALUE][i].asDouble();
   
            theProperty->set_value<vec2>(vec);
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_VEC3)
        {
            vec3 vec;
            float *ptr = &vec[0];
            
            for (int i = 0; i < 3; i++)
                ptr[i] = theJsonValue[PROPERTY_VALUE][i].asDouble();
            
            theProperty->set_value<vec3>(vec);
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_VEC4)
        {
            vec4 vec;
            float *ptr = &vec[0];
            
            for (int i = 0; i < 4; i++)
                ptr[i] = theJsonValue[PROPERTY_VALUE][i].asDouble();
            
            theProperty->set_value<vec4>(vec);
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_QUAT)
        {
            quat vec;
            float *ptr = &vec[0];
            
            for (int i = 0; i < 4; i++)
                ptr[i] = theJsonValue[PROPERTY_VALUE][i].asDouble();
            
            theProperty->set_value<quat>(vec);
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_MAT3)
        {
            mat3 mat;
            float *ptr = &mat[0][0];
            
            for (int i = 0; i < 9; i++)
                ptr[i] = theJsonValue[PROPERTY_VALUE][i].asDouble();
            
            theProperty->set_value<mat3>(mat);
            success = true;
            
        }
        else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_MAT4)
        {
            mat4 mat;
            float *ptr = &mat[0][0];
            
            for (int i = 0; i < 16; i++)
                ptr[i] = theJsonValue[PROPERTY_VALUE][i].asDouble();
            
            theProperty->set_value<mat4>(mat);
            success = true;
            
        }
        else if(theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_VEC2_ARRAY)
        {            
            if(theJsonValue[PROPERTY_VALUE].isArray())
            {
                std::vector<vec2> vals;
                
                for (uint32_t i = 0; i < theJsonValue[PROPERTY_VALUE].size(); ++i)
                {
                    vec2 v;
                    float *ptr = &v[0];
                    
                    for (int j = 0; j < 2; ++j)
                        ptr[j] = theJsonValue[PROPERTY_VALUE][i][j].asDouble();
                    
                    vals.push_back(v);
                }
                theProperty->set_value<std::vector<vec2>>(vals);
            }
            success = true;
        }
        return success;
    }
}
