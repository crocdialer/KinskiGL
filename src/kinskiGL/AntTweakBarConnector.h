#include <AntTweakBar.h>
#include "Property.h"

namespace kinski 
{

class AntTweakBarConnector {
public:
    static void connect(TwBar * theTweakBar, Property::Ptr theProperty,
                        const std::string &theName);
    
    template <typename T>
    static void TW_CALL getValue(void *value, void *clientData);
    
    template <typename T>
    static void TW_CALL setValue(const void *value, void *clientData);

    template <typename T>
    static void TW_CALL getArrayValue(void *value, void *clientData);
    
    template <typename T>
    static void TW_CALL setArrayValue(const void *value, void *clientData);
    
    static void TW_CALL getString(void *value, void *clientData);
    static void TW_CALL setString(const void *value, void *clientData);

};
};
