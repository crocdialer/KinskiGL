//
//  Property.h
//  ATS
//
//  Created by Sebastian Heymann on 8/11/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef _KINSKI_PROPERTY_INCLUDED__
#define _KINSKI_PROPERTY_INCLUDED__

#include <string>
#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>
#include "Exception.h"

namespace kinski {

class Property 
{
public:
    typedef boost::shared_ptr<Property> Ptr;
    
    //Property(); // default constructor
    Property(std::string theName, boost::any &theProperty);
   
    boost::any getValue();
    std::string getName();

	void setIsTweakable(bool isTweakable);
	bool getIsTweakable();

    template <typename T> 
    void setValue(T theValue) 
    {
        if (!isOfType<T>()) {throw WrongTypeSetException(m_name);}
        
        m_value = theValue;
    }
   
    template <typename T>
    T getValue() {
        
        if (!isOfType<T>()) {throw WrongTypeGetException(m_name);}
        
        try 
        {
            return boost::any_cast<T>(m_value);
        
        } catch (const boost::bad_any_cast &theException) {
//            LOG(ERROR) << "cannot cast value of '"  << m_name << "' to the requested type.";
            throw theException;
        }
    }
    
    template <typename T>
    bool isOfType() {
        return m_value.type() == typeid(T);
    }

private:
    std::string m_name;
    boost::any &m_value;
	bool m_tweakable;

public:
    // define exceptions
    class WrongTypeSetException : public Exception
    {
    public:
        WrongTypeSetException(std::string thePropertyName) : 
            Exception(std::string("Wrong type in setValue for Property: ") + thePropertyName 
                + std::string(" - Only the original type can be set."))
        {}
    }; 
    
    // define exceptions
    class WrongTypeGetException : public Exception
    {
    public:
        WrongTypeGetException(std::string thePropertyName) : 
            Exception(std::string("Wrong type in getValue for Property: ") + thePropertyName)
        {}
    }; 
};

}

#endif // _KINSKI_PROPERTY_INCLUDED__
