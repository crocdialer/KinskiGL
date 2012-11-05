//
//  SerializerGL.h
//  kinskiGL
//
//  Created by Fabian on 11/5/12.
//
//

#ifndef __kinskiGL__SerializerGL__
#define __kinskiGL__SerializerGL__

#include "kinskiCore/Serializer.h"
#include "KinskiGL.h"

namespace kinski
{
    /*!
     * Delegate object to handle all known types
     * Can be provided by user to add support for arbitrary data formats
     */
    class PropertyIO_GL : public PropertyIO
    {
    public:
        static const std::string PROPERTY_TYPE_VEC2;
        static const std::string PROPERTY_TYPE_VEC3;
        static const std::string PROPERTY_TYPE_VEC4;
        static const std::string PROPERTY_TYPE_QUAT;
        static const std::string PROPERTY_TYPE_MAT3;
        static const std::string PROPERTY_TYPE_MAT4;
        
        virtual ~PropertyIO_GL(){};
        virtual bool readPropertyValue(const Property::Ptr &theProperty,
                                       Json::Value &theJsonValue) const;
        virtual bool writePropertyValue(Property::Ptr &theProperty,
                                        const Json::Value &theJsonValue) const;
    };
    
}//namespace

#endif /* defined(__kinskiGL__SerializerGL__) */
