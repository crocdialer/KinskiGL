#include <AntTweakBar.h>
#include "Property.h"

namespace kinski 
{

class AntTweakBarConnector {
public:
    static void connect(TwBar * theTweakBar,
                        const Property::Ptr theProperty,
                        const std::string &theGroup="");
private:
    
    template <typename T>
    static void TW_CALL getValue(void *value, void *clientData);
    
    template <typename T>
    static void TW_CALL setValue(const void *value, void *clientData);
    
    static void TW_CALL getString(void *value, void *clientData);
    static void TW_CALL setString(const void *value, void *clientData);
    
    static void TW_CALL getVec3(void *value, void *clientData);
    static void TW_CALL setVec3(const void *value, void *clientData);
    
    static void TW_CALL getVec4(void *value, void *clientData);
    static void TW_CALL setVec4(const void *value, void *clientData);
    
    static void TW_CALL getQuaternion(void *value, void *clientData);
    static void TW_CALL setQuaternion(const void *value, void *clientData);

};
};
