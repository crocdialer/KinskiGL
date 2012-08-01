//
//  Property.h
//  KINSKIGL
//
//  
//

#ifndef _KINSKI_PROPERTY_INCLUDED__
#define _KINSKI_PROPERTY_INCLUDED__

#include <string>
#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>
#include "Exception.h"

namespace kinski
{

class Property 
{
public:
    typedef boost::shared_ptr<Property> Ptr;
    
    Property(); // default constructor
    Property(const std::string &theName, const boost::any &theValue);
   
    boost::any getValue() const;
    const std::string& getName() const;
    void setName(const std::string& theName);

	void setIsTweakable(bool isTweakable);
	bool getIsTweakable() const;

    bool empty() const {return m_value.empty();};
    
    template <typename T> 
    void setValue(const T& theValue) 
    {
        if (!isOfType<T>()) {throw WrongTypeSetException(m_name);}
        if(checkValue(theValue))
            m_value = theValue;
    }
   
    template <typename T>
    T getValue() const
    {
        if (!isOfType<T>()) {throw WrongTypeGetException(m_name);}
        
        try 
        {
            return boost::any_cast<T>(m_value);
        }
        catch (const boost::bad_any_cast &theException)
        {
            throw theException;
        }
    }
    
    template <typename C>
    inline bool isOfType() const
    {
        return m_value.type() == typeid(C);
    }
    
    virtual bool checkValue(const boost::any &theVal){return true;};

private:
    std::string m_name;
    boost::any m_value;
    
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
    
    class WrongTypeGetException : public Exception
    {
    public:
        WrongTypeGetException(std::string thePropertyName) : 
            Exception(std::string("Wrong type in getValue for Property: ") + thePropertyName)
        {}
    };

};
    
template<typename T>
class _Property : public Property
{
public:
    typedef boost::shared_ptr< _Property<T> > Ptr;
    
    _Property():Property(){};
    
    _Property(const std::string &theName, const T &theValue):
    Property(theName, theValue){};
    
    explicit _Property(const _Property<T> &other):
    Property(other.getName(), other.getValue<T>()){};
    
    static Ptr create(const std::string &theName, const T &theValue)
    {
        Ptr outPtr (new _Property(theName, theValue));
        return outPtr;
    };
    
    inline const T val() const {return getValue<T>();};
    
    inline void set(const T &theVal){setValue<T>(theVal);};
    
    inline void val(const T &theVal){set(theVal);};
    inline void operator()(const T &theVal){setValue<T>(theVal);};
    
    inline _Property<T>& operator=(T const& theVal)
    {
        set(theVal);
        return *this;
    };
    
    inline _Property<T>& operator+=(T const& theVal)
    {
        *this = getValue<T>() + theVal; 
        return *this;
    };
    
    inline _Property<T>& operator+=(_Property<T> const& otherProp)
    {
        *this = getValue<T>() + otherProp.getValue<T>(); 
        return *this;
    };
    
    inline const T operator+(_Property<T> const& otherProp)
    {
        return getValue<T>() + otherProp.getValue<T>(); 
    };
    
    inline _Property<T>& operator-=(T const& theVal)
    {
        *this = getValue<T>() - theVal; 
        return *this;
    };
    
    inline _Property<T>& operator-=(_Property<T> const& otherProp)
    {
        *this = getValue<T>() - otherProp.getValue<T>(); 
        return *this;
    };
    
    inline const T operator-(_Property<T> const& otherProp)
    {
        return getValue<T>() - otherProp.getValue<T>(); 
    };
    
    inline _Property<T>& operator*=(T const& theVal)
    {
        *this = getValue<T>() * theVal; 
        return *this;
    };
    
    inline _Property<T>& operator*=(_Property<T> const& otherProp)
    {
        *this = getValue<T>() + otherProp.getValue<T>(); 
        return *this;
    };
    
    inline const T operator*(_Property<T> const& otherProp)
    {
        return getValue<T>() * otherProp.getValue<T>(); 
    };
    
    inline _Property<T>& operator/=(T const& theVal)
    {
        *this = getValue<T>() / theVal; 
        return *this;
    };
    
    inline _Property<T>& operator/=(_Property<T> const& otherProp)
    {
        *this = getValue<T>() / otherProp.getValue<T>(); 
        return *this;
    };
    
    inline const T operator/(_Property<T> const& otherProp)
    {
        return getValue<T>() / otherProp.getValue<T>(); 
    };
    
    inline friend const T operator+(T theVal, const _Property<T>& theProp)
    {
        return theVal + theProp.getValue<T>(); 
    };
    
    inline friend const T operator+(const _Property<T>& theProp, T theVal)
    {
        return theVal + theProp.getValue<T>(); 
    };

    inline friend const T operator-(T theVal, const _Property<T>& theProp)
    {
        return theVal - theProp.getValue<T>(); 
    };
    
    inline friend const T operator-(const _Property<T>& theProp, T theVal)
    {
        return theProp.getValue<T>() - theVal; 
    };
    
    inline friend const T operator*(T theVal, const _Property<T>& theProp)
    {
        return theVal * theProp.getValue<T>(); 
    };
    
    inline friend const T operator*(const _Property<T>& theProp, T theVal)
    {
        return theVal * theProp.getValue<T>(); 
    };
    
    inline friend const T operator/(T theVal, const _Property<T>& theProp)
    {
        return theVal / theProp.getValue<T>(); 
    };
    
    inline friend const T operator/(const _Property<T>& theProp, T theVal)
    {
        return theProp.getValue<T>() / theVal; 
    };
    
    friend std::ostream& operator<<(std::ostream &os,const _Property<T>& theProp)
    {
        os<< theProp.getName()<<": "<<theProp.getValue<T>()<<"\n";
        return os;
    }
};

        
template<typename T>
class _RangedProperty : public _Property<T>
{
public:
    
    typedef boost::shared_ptr< _RangedProperty<T> > Ptr;
    
    _RangedProperty():_Property<T>(){};
    _RangedProperty(const std::string &theName, const T &theValue,
                    const T &min, const T &max):
    _Property<T>(theName, theValue)
    {
        setRange(min, max);
        checkValue(theValue);
    };
    
    inline _RangedProperty<T>& operator=(T const& theVal)
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
        Ptr outPtr(new _RangedProperty(theName, theValue, min, max));
        return outPtr;
    };
    
    void setRange(const T &min, const T &max)
    {
        if( min > max )
            throw BadBoundsException(this->getName());
        
        m_min = min;
        m_max = max;
    };
    
    void getRange(T &min, T &max) const
    {
        min = m_min;
        max = m_max;
    };

    bool checkValue(const boost::any &theVal)
    {
        T v = boost::any_cast<T>(theVal);
        try
        {
            rangeCheck(v);
            return true;
        }
        catch(const BadBoundsException &e)
        {
            v = std::min(v, m_max);
            v = std::max(v, m_min);
            this->set(v);
        }

        return false;
    };
    
    friend std::ostream& operator<<(std::ostream &os,const _RangedProperty<T>& theProp)
    {
        T min, max;
        theProp.getRange(min, max);
        
        os<< theProp.getName()<<": "<<theProp.val()<<" ( "<<min<<" - "<<max<<" )\n";
        return os;
    }

private:

    inline void rangeCheck(const T &theValue)
    {
        // check range
        if( m_min > theValue || m_max < theValue )
            throw BadBoundsException(this->getName());
    }
    
    T m_min, m_max;
};

}

#endif // _KINSKI_PROPERTY_INCLUDED__
