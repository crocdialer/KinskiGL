#include "kinskiGL/KinskiGL.h"
#include "AntTweakBarConnector.h"

namespace kinski
{
    void
    AntTweakBarConnector::connect(TwBar * theTweakBar,
                                  Property::Ptr theProperty,
                                  const std::string &theName) {
            
        Property* myPProp = theProperty.get();
        if (!theProperty->getIsTweakable()) 
        {
            return;
        }
        
        std::string myPropName = theProperty->getName();
        
        std::string myGroup = std::string(" group=") + theName;
        
        if (theProperty->isOfType<int>()) 
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_INT32, 
                       AntTweakBarConnector::setValue<int>,
                       AntTweakBarConnector::getValue<int>,
                       (void*)myPProp, myGroup.c_str());
            
        } else if (theProperty->isOfType<unsigned int>()) 
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_UINT32, 
                       AntTweakBarConnector::setValue<unsigned int>,
                       AntTweakBarConnector::getValue<unsigned int>,
                       (void*)myPProp, myGroup.c_str());
        } else if (theProperty->isOfType<float>()) 
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_FLOAT, 
                       AntTweakBarConnector::setValue<float>,
                       AntTweakBarConnector::getValue<float>,
                       (void*)myPProp,
                       std::string(myGroup + std::string(" step=0.01")).c_str());
            
        } else if (theProperty->isOfType<double>()) 
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_DOUBLE, 
                       AntTweakBarConnector::setValue<double>,
                       AntTweakBarConnector::getValue<double>,
                       (void*)myPProp,
                       std::string(myGroup + std::string(" step=0.01")).c_str());
            
        } else if (theProperty->isOfType<unsigned short>()) 
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_UINT16, 
                       AntTweakBarConnector::setValue<unsigned short>,
                       AntTweakBarConnector::getValue<unsigned short>,
                       (void*)myPProp,
                       myGroup.c_str());
            
        } else if (theProperty->isOfType<bool>()) 
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_BOOLCPP, 
                       AntTweakBarConnector::setValue<bool>,
                       AntTweakBarConnector::getValue<bool>,
                       (void*)myPProp, myGroup.c_str());
            
        } else if (theProperty->isOfType<std::string>()) 
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_STDSTRING, 
                       AntTweakBarConnector::setString,
                       AntTweakBarConnector::getString,
                       (void*)myPProp, myGroup.c_str());
        }
        else if (theProperty->isOfType<glm::vec3>()) 
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_DIR3F, 
                       AntTweakBarConnector::setArrayValue<glm::vec3>,
                       AntTweakBarConnector::getArrayValue<glm::vec3>,
                       (void*)myPProp, myGroup.c_str());
        }
        else if (theProperty->isOfType<glm::vec4>()) 
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_COLOR4F, 
                       AntTweakBarConnector::setArrayValue<glm::vec4>,
                       AntTweakBarConnector::getArrayValue<glm::vec4>,
                       (void*)myPProp, myGroup.c_str());
        }
        else if (theProperty->isOfType<glm::vec3>()) 
        {
            TwAddVarCB(theTweakBar, myPropName.c_str(), TW_TYPE_QUAT4F, 
                       AntTweakBarConnector::setArrayValue<glm::quat>,
                       AntTweakBarConnector::getArrayValue<glm::quat>,
                       (void*)myPProp, myGroup.c_str());
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
    
    template <typename T>
    void TW_CALL 
    AntTweakBarConnector::getArrayValue(void *value, void *clientData) 
    {
        Property * theProperty = (Property*) clientData;
        
        *(T *)value = theProperty->getValue<T>(); 
    }
    
    template <typename T>
    void TW_CALL 
    AntTweakBarConnector::setArrayValue(const void *value, void *clientData) 
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
    

}
