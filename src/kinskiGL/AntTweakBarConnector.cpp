#include "kinskiGL/KinskiGL.h"
#include "AntTweakBarConnector.h"

using namespace glm;

namespace kinski
{
    void
    AntTweakBarConnector::connect(TwBar * theTweakBar,
                                  const Property::Ptr theProperty,
                                  const std::string &theGroup) {
            
        Property* myPProp = theProperty.get();
        if (!theProperty->getIsTweakable()) 
        {
            return;
        }

        std::string myPropName = theProperty->getName();
        
        std::string defString = " group='" + theGroup+"'";
        defString += std::string(" label='") + myPropName + "'";
        
        if (theProperty->isOfType<int>()) 
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_INT32, 
                       AntTweakBarConnector::setValue<int>,
                       AntTweakBarConnector::getValue<int>,
                       (void*)myPProp, defString.c_str());
            
        } else if (theProperty->isOfType<unsigned int>()) 
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_UINT32, 
                       AntTweakBarConnector::setValue<unsigned int>,
                       AntTweakBarConnector::getValue<unsigned int>,
                       (void*)myPProp, defString.c_str());
        } else if (theProperty->isOfType<float>()) 
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_FLOAT, 
                       AntTweakBarConnector::setValue<float>,
                       AntTweakBarConnector::getValue<float>,
                       (void*)myPProp,
                       std::string(defString + std::string(" step=0.01")).c_str());
            
        } else if (theProperty->isOfType<double>()) 
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_DOUBLE, 
                       AntTweakBarConnector::setValue<double>,
                       AntTweakBarConnector::getValue<double>,
                       (void*)myPProp,
                       std::string(defString + std::string(" step=0.01")).c_str());
            
        } else if (theProperty->isOfType<unsigned short>()) 
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_UINT16, 
                       AntTweakBarConnector::setValue<unsigned short>,
                       AntTweakBarConnector::getValue<unsigned short>,
                       (void*)myPProp,
                       defString.c_str());
            
        } else if (theProperty->isOfType<bool>()) 
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_BOOLCPP, 
                       AntTweakBarConnector::setValue<bool>,
                       AntTweakBarConnector::getValue<bool>,
                       (void*)myPProp, defString.c_str());
            
        } else if (theProperty->isOfType<std::string>()) 
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
            // TODO: split in mat3 and vec4 and add those to tweakBar
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
        
        std::string *destPtr = static_cast<std::string *>(value);
        TwCopyStdStringToLibrary(*destPtr, theProperty->getValue<std::string>());
    }
    
    void TW_CALL 
    AntTweakBarConnector::setString(const void *value, void *clientData) 
    {
        Property * theProperty = (Property*) clientData;
        
        const std::string *srcPtr = static_cast<const std::string *>(value);
        theProperty->setValue(*srcPtr);
    }
    
    /**************************************************************************/
    
    
    void TW_CALL 
    AntTweakBarConnector::getVec3(void *value, void *clientData) 
    {
        _Property<vec3> * theProperty = (_Property<vec3>*) clientData;
        
        memcpy(value, &theProperty->val()[0], 3 * sizeof(float));
    }
    
    void TW_CALL 
    AntTweakBarConnector::setVec3(const void *value, void *clientData) 
    {
        _Property<vec3> * theProperty = (_Property<vec3>*) clientData;
        const float *v = (float*) value;
        theProperty->val(vec3(v[0], v[1], v[2]));
    }
    
    void TW_CALL 
    AntTweakBarConnector::getVec4(void *value, void *clientData) 
    {
        _Property<vec4> * theProperty = (_Property<vec4>*) clientData;
        
        memcpy(value, &theProperty->val()[0], 4 * sizeof(float));
    }
    
    void TW_CALL 
    AntTweakBarConnector::setVec4(const void *value, void *clientData) 
    {
        _Property<vec4> * theProperty = (_Property<vec4>*) clientData;
        const float *v = (float*) value;
        theProperty->val(vec4(v[0], v[1], v[2], v[3]));
    }
    
    void TW_CALL 
    AntTweakBarConnector::getQuaternion(void *value, void *clientData) 
    {
        _Property<mat3> * theProperty = (_Property<mat3>*) clientData;
        quat outQuad (theProperty->val());
        
        memcpy(value, &outQuad[0], 4 * sizeof(float));
        
    }
    
    void TW_CALL 
    AntTweakBarConnector::setQuaternion(const void *value, void *clientData) 
    {
        _Property<mat3> * theProperty = (_Property<mat3>*) clientData;
        const float *v = (float*) value;
        quat outQuad(v[0], v[1], v[2], v[3]);
        
        theProperty->val(mat3_cast(outQuad));
    }
}
