// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include <boost/any.hpp>
#include <boost/signals2.hpp>
#include "core/core.hpp"

namespace kinski
{

class KINSKI_API Property : public std::enable_shared_from_this<Property>
{
public:
    typedef std::shared_ptr<Property> Ptr;
    typedef std::shared_ptr<const Property> ConstPtr;
    virtual ~Property(){};
    
    class Observer
    {
    public:
        typedef std::shared_ptr<Observer> Ptr;
        virtual void update_property(const Property::ConstPtr &theProperty) = 0;
    };

    inline boost::any getValue() const {return m_value;};
    inline const std::string& getName() const {return m_name;};
    inline void setName(const std::string& theName) {m_name = theName;};
	inline void setTweakable(bool isTweakable) {m_tweakable = isTweakable;};
	inline bool isTweakable() const {return m_tweakable;};
    inline bool empty() const {return m_value.empty();};

    template <typename T> 
    inline void setValue(const T& theValue)
    {
        if (!isOfType<T>()) {throw WrongTypeSetException(m_name);}
        if(checkValue(theValue))
        {
            m_value = theValue;
            notifyObservers();
        }
    }
   
    template <typename T>
    inline const T& getValue() const
    {
        try{return *boost::any_cast<T>(&m_value);}
        catch (const boost::bad_any_cast &theException){throw WrongTypeGetException(m_name);}
    }
    
    template <typename T>
    inline T& getValue()
    {
        try{return *boost::any_cast<T>(&m_value);}
        catch (const boost::bad_any_cast &theException){throw WrongTypeGetException(m_name);}
    }
    
    template <typename T>
    inline T* getValuePtr()
    {
        try{return boost::any_cast<T>(&m_value);}
        catch (const boost::bad_any_cast &theException){throw WrongTypeGetException(m_name);}
    }
    
    template <typename C>
    inline bool isOfType() const
    {
        return m_value.type() == typeid(C);
    }
    
    virtual bool checkValue(const boost::any &theVal)
    {return theVal.type() == m_value.type();};
    
    inline void addObserver(const Observer::Ptr &theObs)
    {
        m_signal.connect(signal_type::slot_type(&Observer::update_property, theObs, _1).track_foreign(theObs));
    }
    
    inline void removeObserver(const Observer::Ptr &theObs)
    {
        m_signal.disconnect(boost::bind(&Observer::update_property, theObs, _1));
    }
    
    inline void clearObservers(){m_signal.disconnect_all_slots();}
    inline void notifyObservers(){m_signal(shared_from_this());}

protected:
    Property(): m_tweakable(true){}; // default constructor
    Property(const std::string &theName, const boost::any &theValue):
    m_name(theName), m_value(theValue), m_tweakable(true){};
    
private:
    std::string m_name;
    boost::any m_value;
	bool m_tweakable;
    
    typedef boost::signals2::signal<void(const Property::ConstPtr&)> signal_type;
    signal_type m_signal;

public:
    // define exceptions
    class WrongTypeSetException : public Exception
    {
    public:
        WrongTypeSetException(const std::string &thePropertyName) :
            Exception(std::string("Wrong type in setValue for Property: ") + thePropertyName 
                + std::string(" - Only the original type can be set."))
        {}
    }; 
    
    class WrongTypeGetException : public Exception
    {
    public:
        WrongTypeGetException(const std::string & thePropertyName) : 
            Exception(std::string("Wrong type in getValue for Property: ") + thePropertyName)
        {}
    };

};
    
template<typename T>
class KINSKI_API Property_ : public Property
{
public:
    typedef std::shared_ptr< Property_<T> > Ptr;
    typedef std::weak_ptr< Property_<T> > WeakPtr;
    
    static Ptr create(const std::string &theName, const T &theValue = T())
    {
        Ptr outPtr (new Property_(theName, theValue));
        return outPtr;
    };
    
    inline const T& value() const {return getValue<T>();};
    inline T& value() {return getValue<T>();};
    inline void set(const T &theVal){setValue<T>(theVal);};
    inline void value(const T &theVal){set(theVal);};
    inline operator const T&() const { return value(); }
    
    inline Property_<T>& operator=(const Property_<T> &theProp)
    {
        setName(theProp.getName());
        set(theProp.value());
        return *this;
    };
    
    inline Property_<T>& operator=(const T &theVal)
    {
        set(theVal);
        return *this;
    };
    
    inline Property_<T>& operator+=(const T &theVal)
    {
        set(*this + theVal);
        return *this;
    };
    
    inline Property_<T>& operator-=(const T &theVal)
    {
        set(*this - theVal);
        return *this;
    };
    
    inline Property_<T>& operator*=(const T &theVal)
    {
        set(*this * theVal);
        return *this;
    };
    
    inline Property_<T>& operator/=(const T &theVal)
    {
        set(*this / theVal);
        return *this;
    };
    
    friend std::ostream& operator<<(std::ostream &os,const Property_<T>& theProp)
    {
        os<< theProp.getName()<<": "<<theProp.getValue<T>();
        return os;
    }
        
protected:
    Property_():Property(){};
    
    Property_(const std::string &theName, const T &theValue):
    Property(theName, theValue){};
    
    explicit Property_(const Property_<T> &other):
    Property(other.getName(), other.getValue<T>()){};

};

        
template<typename T>
class KINSKI_API RangedProperty : public Property_<T>
{
public:
    
    typedef std::shared_ptr< RangedProperty<T> > Ptr;
    virtual ~RangedProperty<T>(){};
    
    inline RangedProperty<T>& operator=(const T &theVal)
    {
        this->set(theVal);
        return *this;
    };
    
    class BadBoundsException : public Exception
    {
    public:
        BadBoundsException(std::string thePropertyName) :
        Exception(std::string("Bad bounds set for Property: ") + thePropertyName)
        {}
    };
    
    static Ptr create(const std::string &theName,
                      const T &theValue,
                      const T &min,
                      const T &max)
    {
        Ptr outPtr(new RangedProperty(theName, theValue, min, max));
        return outPtr;
    };
    
    void setRange(const T &min, const T &max)
    {
        if( min > max )
            throw BadBoundsException(this->getName());
        m_min = min;
        m_max = max;
        checkValue(this->getValue());
    };
    
    void getRange(T &min, T &max) const
    {
        min = m_min;
        max = m_max;
    };

    bool checkValue(const boost::any &theVal)
    {
        T v;
        
        try
        {
            v = boost::any_cast<T>(theVal);
            rangeCheck(v);
        }
        catch (const boost::bad_any_cast &e)
        {
            return false;
        }
        catch(const BadBoundsException &e)
        {
            v = clamp(v, m_min, m_max);
            this->set(v);
            return false;
        }
        return true;
    };
    
    friend std::ostream& operator<<(std::ostream &os,const RangedProperty<T>& theProp)
    {
        T min, max;
        theProp.getRange(min, max);
        os<< theProp.getName()<<": "<<theProp.value()<<" ("<<min<<" - "<<max<<")";
        return os;
    }

private:

    RangedProperty():Property_<T>(){};
    RangedProperty(const std::string &theName, const T &theValue, const T &min, const T &max):
    Property_<T>(theName, theValue)
    {
        setRange(min, max);
        checkValue(theValue);
    };
    
    inline void rangeCheck(const T &theValue)
    {
        if( m_min > theValue || m_max < theValue )
            throw BadBoundsException(this->getName());
    }
    
    T m_min, m_max;
};

}