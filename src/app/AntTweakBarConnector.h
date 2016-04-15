// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include <AntTweakBar.h>
#include "core/Property.hpp"

namespace kinski {

class AntTweakBarConnector {
public:
    static void connect(TwBar * theTweakBar,
                        const Property::Ptr &theProperty,
                        const std::string &theGroup="");
    
    // define exceptions
    class PropertyUnsupportedException : public Exception
    {
    public:
        PropertyUnsupportedException(const std::string &thePropertyName) :
        Exception("Could not add property '"+thePropertyName+"' "\
                  "to tweakBar : Type not supported")
        {};
    }; 
    
private:
    
    template <typename T>
    static void TW_CALL getValue(void *value, void *clientData);
    
    template <typename T>
    static void TW_CALL setValue(const void *value, void *clientData);
    
    static void TW_CALL getString(void *value, void *clientData);
    static void TW_CALL setString(const void *value, void *clientData);
    
    static void TW_CALL getVec2_X(void *value, void *clientData);
    static void TW_CALL setVec2_X(const void *value, void *clientData);
    
    static void TW_CALL getVec2_Y(void *value, void *clientData);
    static void TW_CALL setVec2_Y(const void *value, void *clientData);
    
    static void TW_CALL getVec3(void *value, void *clientData);
    static void TW_CALL setVec3(const void *value, void *clientData);
    
    static void TW_CALL getVec4(void *value, void *clientData);
    static void TW_CALL setVec4(const void *value, void *clientData);
    
    static void TW_CALL getQuaternion(void *value, void *clientData);
    static void TW_CALL setQuaternion(const void *value, void *clientData);
    
    static void TW_CALL 
    getMat4_rot(void *value, void *clientData); 
    static void TW_CALL 
    setMat4_rot(const void *value, void *clientData); 
    static void TW_CALL 
    getMat4_pos(void *value, void *clientData); 
    static void TW_CALL 
    setMat4_pos(const void *value, void *clientData);
    
    // helper to eventually set the range within the tweakbar
    // for instances of RangedProperty
    template <typename T>
    static void adjustRange(TwBar * theTweakBar, const Property::Ptr &theProperty);

};
}//namespace