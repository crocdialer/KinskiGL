// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "kinskiGL/KinskiGL.h"
#ifndef KINSKI_RASPI 
#include "AntTweakBarConnector.h"
#include <iostream>
#include <sstream>

using namespace glm;
using namespace std;

namespace kinski
{
    void AntTweakBarConnector::connect(TwBar * theTweakBar,
                                       const Property::Ptr &theProperty,
                                       const string &theGroup)
    {
            
        Property* myPProp = theProperty.get();
        if (!theProperty->isTweakable())
        {
            return;
        }

        string myPropName = theProperty->getName();
        
        string defString = " group='" + theGroup+ "'";
        defString += string(" label='") + myPropName + "'";
        
        if (theProperty->isOfType<int>()) 
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_INT32, 
                       AntTweakBarConnector::setValue<int>,
                       AntTweakBarConnector::getValue<int>,
                       (void*)myPProp, defString.c_str());
            
            adjustRange<int>(theTweakBar, theProperty);
            
        }
        else if (theProperty->isOfType<unsigned int>())
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_UINT32, 
                       AntTweakBarConnector::setValue<unsigned int>,
                       AntTweakBarConnector::getValue<unsigned int>,
                       (void*)myPProp, defString.c_str());
            
            adjustRange<unsigned int>(theTweakBar, theProperty);
            
        }
        else if (theProperty->isOfType<float>())
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_FLOAT, 
                       AntTweakBarConnector::setValue<float>,
                       AntTweakBarConnector::getValue<float>,
                       (void*)myPProp,
                       (defString + " step=0.05").c_str());
            
            adjustRange<float>(theTweakBar, theProperty);
            
        }
        else if (theProperty->isOfType<double>())
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_DOUBLE, 
                       AntTweakBarConnector::setValue<double>,
                       AntTweakBarConnector::getValue<double>,
                       (void*)myPProp,
                       (defString + " step=0.05").c_str());
            
            adjustRange<double>(theTweakBar, theProperty);
            
        }
        else if (theProperty->isOfType<unsigned short>())
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_UINT16, 
                       AntTweakBarConnector::setValue<unsigned short>,
                       AntTweakBarConnector::getValue<unsigned short>,
                       (void*)myPProp,
                       defString.c_str());
            
            adjustRange<unsigned short>(theTweakBar, theProperty);
            
        }
        else if (theProperty->isOfType<bool>())
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_BOOLCPP, 
                       AntTweakBarConnector::setValue<bool>,
                       AntTweakBarConnector::getValue<bool>,
                       (void*)myPProp, defString.c_str());
            
        }
        else if (theProperty->isOfType<string>())
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_STDSTRING, 
                       AntTweakBarConnector::setString,
                       AntTweakBarConnector::getString,
                       (void*)myPProp, defString.c_str());
        }
        else if (theProperty->isOfType<vec3>()) 
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_DIR3F, 
                       AntTweakBarConnector::setVec3,
                       AntTweakBarConnector::getVec3,
                       (void*)myPProp, defString.c_str());
        }
        else if (theProperty->isOfType<vec4>()) 
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_COLOR4F, 
                       AntTweakBarConnector::setVec4,
                       AntTweakBarConnector::getVec4,
                       (void*)myPProp, defString.c_str());
        }
        else if (theProperty->isOfType<mat3>()) 
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_QUAT4F, 
                       AntTweakBarConnector::setQuaternion,
                       AntTweakBarConnector::getQuaternion,
                       (void*)myPProp, defString.c_str());
        }
        else if (theProperty->isOfType<mat4>()) 
        {
            const Property_<mat4>::Ptr p = 
            static_pointer_cast<Property_<mat4> >(theProperty);
            
            defString = " group='" + myPropName +"'";
            
            string rotStr = myPropName+" rotation";
            string posStr = myPropName+" position";
            
            TwAddVarCB(theTweakBar, rotStr.c_str(), TW_TYPE_QUAT4F, 
                       AntTweakBarConnector::setMat4_rot,
                       AntTweakBarConnector::getMat4_rot,
                       (void*)myPProp, defString.c_str());
            
            TwAddVarCB(theTweakBar, posStr.c_str(), TW_TYPE_DIR3F, 
                       AntTweakBarConnector::setMat4_pos,
                       AntTweakBarConnector::getMat4_pos,
                       (void*)myPProp, defString.c_str());
            
            // arrange group afterwards
            string barName = TwGetBarName(theTweakBar);
            defString = barName +"/"+ myPropName +" group='"+ theGroup +"'";
            
            TwDefine(defString.c_str());
        }   
        else
        {
            throw PropertyUnsupportedException(theProperty->getName());
        }
    }
    
    template <typename T>
    void TW_CALL 
    AntTweakBarConnector::getValue(void *value, void *clientData) 
    {
        Property * theProperty = (Property*) clientData;
        
        *(T *)value = theProperty->getValue<T>(); 
    }
    
    template <typename T>
    void TW_CALL 
    AntTweakBarConnector::setValue(const void *value, void *clientData) 
    {
        Property * theProperty = (Property*) clientData;
        
        theProperty->setValue( *(const T *)value );
    }
    
    void TW_CALL 
    AntTweakBarConnector::getString(void *value, void *clientData) 
    {
        Property * theProperty = (Property*) clientData;
        
        string *destPtr = static_cast<string *>(value);
        TwCopyStdStringToLibrary(*destPtr, theProperty->getValue<string>());
    }
    
    void TW_CALL 
    AntTweakBarConnector::setString(const void *value, void *clientData) 
    {
        Property * theProperty = (Property*) clientData;
        
        const string *srcPtr = static_cast<const string *>(value);
        theProperty->setValue(*srcPtr);
    }
    
    /**************************************************************************/
    
    
    void TW_CALL 
    AntTweakBarConnector::getVec3(void *value, void *clientData) 
    {
        Property_<vec3> * theProperty = (Property_<vec3>*) clientData;
        
        memcpy(value, &theProperty->val()[0], 3 * sizeof(float));
    }
    
    void TW_CALL 
    AntTweakBarConnector::setVec3(const void *value, void *clientData) 
    {
        Property_<vec3> * theProperty = (Property_<vec3>*) clientData;
        const float *v = (float*) value;
        theProperty->val(vec3(v[0], v[1], v[2]));
        
        //memcpy(&theProperty->val()[0], value, 3 * sizeof(float));
    }
    
    void TW_CALL 
    AntTweakBarConnector::getVec4(void *value, void *clientData) 
    {
        Property_<vec4> * theProperty = (Property_<vec4>*) clientData;
        
        memcpy(value, &theProperty->val()[0], 4 * sizeof(float));
    }
    
    void TW_CALL 
    AntTweakBarConnector::setVec4(const void *value, void *clientData) 
    {
        Property_<vec4> * theProperty = (Property_<vec4>*) clientData;
        const float *v = (float*) value;
        theProperty->val(vec4(v[0], v[1], v[2], v[3]));
    }
    
    void TW_CALL 
    AntTweakBarConnector::getQuaternion(void *value, void *clientData) 
    {
        Property_<mat3> * theProperty = (Property_<mat3>*) clientData;
        quat outQuad (theProperty->val());
        
        memcpy(value, &outQuad[0], 4 * sizeof(float));
        
    }
    
    void TW_CALL 
    AntTweakBarConnector::setQuaternion(const void *value, void *clientData) 
    {
        Property_<mat3> * theProperty = (Property_<mat3>*) clientData;
        const float *v = (float*) value;
        quat outQuad(v[0], v[1], v[2], v[3]);
        
        theProperty->val(mat3_cast(outQuad));
    }
    
    void TW_CALL 
    AntTweakBarConnector::getMat4_rot(void *value, void *clientData) 
    {
        Property_<mat4> * theProperty = (Property_<mat4>*) clientData;
        
        quat outQuad (theProperty->val());
        memcpy(value, &outQuad[0], 4 * sizeof(float));
    }
    
    void TW_CALL 
    AntTweakBarConnector::setMat4_rot(const void *value, void *clientData) 
    {
        Property_<mat4> * theProperty = (Property_<mat4>*) clientData;
        const float *v = (float*) value;
        quat tmpQuad(v[0], v[1], v[2], v[3]);
        mat4 outMat = mat4_cast(tmpQuad);
        outMat[3] = theProperty->val()[3];
        theProperty->val(outMat);
    }
    
    void TW_CALL 
    AntTweakBarConnector::getMat4_pos(void *value, void *clientData) 
    {
        Property_<mat4> * theProperty = (Property_<mat4>*) clientData;
        
        memcpy(value, &theProperty->val()[3][0], 4 * sizeof(float));
    }
    
    void TW_CALL 
    AntTweakBarConnector::setMat4_pos(const void *value, void *clientData) 
    {
        Property_<mat4> * theProperty = (Property_<mat4>*) clientData;
        const float *v = (float*) value;
        mat4 outMat = theProperty->val();
        outMat[3] = vec4(v[0], v[1], v[2], v[3]);
        
        theProperty->val(outMat);
    }
    
    template <typename T>
    void AntTweakBarConnector::adjustRange(TwBar * theTweakBar, const Property::Ptr &theProperty)
    {
        // set range
        if(typename RangedProperty<T>::Ptr castPtr =
           std::dynamic_pointer_cast<RangedProperty<T> >(theProperty))
        {
            T min, max;
            castPtr->getRange(min, max);
            std::stringstream ss;
            ss << TwGetBarName(theTweakBar) << "/'"
            << theProperty->getName()<< "' min=" << min << " max=" << max ;
            
            if(theProperty->isOfType<float>() || theProperty->isOfType<double>())
                ss<<" step="<<(max-min)/1000.f;
            
            TwDefine(ss.str().c_str());
        }
    }
}
#endif
