//
//  Property.h
//  KINSKIGL
//
//  
//

#ifndef _KINSKIProperty__INCLUDED__
#define _KINSKIProperty__INCLUDED__

#include <boost/any.hpp>

#include "Definitions.h"
#include "Exception.h"

namespace kinski
{

class Property : public std::enable_shared_from_this<Property>
{
public:
    typedef std::shared_ptr<Property> Ptr;
    
    class Observer
    {
    public:
        typedef std::shared_ptr<Observer> Ptr;
        virtual void updateProperty(const Property::Ptr &theProperty) = 0;
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
            m_value = theValue;
        
        notifyObservers();
    }
   
    template <typename T>
    inline const T& getValue() const
    {
        if (!isOfType<T>()) {throw WrongTypeGetException(m_name);}
        
        try 
        {
            return boost::any_cast<const T&>(m_value);
        }
        catch (const boost::bad_any_cast &theException)
        {
            throw WrongTypeGetException(m_name);
        }
    }
    
    template <typename C>
    inline bool isOfType() const
    {
        return m_value.type() == typeid(C);
    }
    
    virtual bool checkValue(const boost::any &theVal)
    {return theVal.type() == m_value.type();};
    
    inline void addObserver(const Observer::Ptr &theObs)
    {m_observers.push_back(theObs);};
    
    inline void removeObserver(const Observer::Ptr &theObs)
    {m_observers.remove(theObs);};
    
    inline void clearObservers(){m_observers.clear();};
    
    inline void notifyObservers()
    {
        std::list<Observer::Ptr>::iterator it = m_observers.begin();
        Ptr self = shared_from_this();
        for (; it != m_observers.end(); it++)
        {
            (*it)->updateProperty(self);
        }
    };

protected:
    Property(): m_tweakable(true){}; // default constructor
    Property(const std::string &theName, const boost::any &theValue):
    m_name(theName), m_value(theValue), m_tweakable(true){};
    
private:
    std::string m_name;
    boost::any m_value;
    
	bool m_tweakable;
    
    std::list<Observer::Ptr> m_observers;

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
class Property_ : public Property
{
public:
    typedef std::shared_ptr< Property_<T> > Ptr;
    
    static Ptr create(const std::string &theName, const T &theValue)
    {
        Ptr outPtr (new Property_(theName, theValue));
        return outPtr;
    };
    
    inline const T& val() const {return getValue<T>();};
    
    inline void set(const T &theVal){setValue<T>(theVal);};
    
    inline void val(const T &theVal){set(theVal);};
    inline void operator()(const T &theVal){setValue<T>(theVal);};
    
    inline Property_<T>& operator=(T const& theVal)
    {
        set(theVal);
        return *this;
    };
    
    inline Property_<T>& operator+=(T const& theVal)
    {
        *this = getValue<T>() + theVal; 
        return *this;
    };
    
    inline Property_<T>& operator+=(Property_<T> const& otherProp)
    {
        *this = getValue<T>() + otherProp.getValue<T>(); 
        return *this;
    };
    
    inline const T operator+(Property_<T> const& otherProp)
    {
        return getValue<T>() + otherProp.getValue<T>(); 
    };
    
    inline Property_<T>& operator-=(T const& theVal)
    {
        *this = getValue<T>() - theVal; 
        return *this;
    };
    
    inline Property_<T>& operator-=(Property_<T> const& otherProp)
    {
        *this = getValue<T>() - otherProp.getValue<T>(); 
        return *this;
    };
    
    inline const T operator-(Property_<T> const& otherProp)
    {
        return getValue<T>() - otherProp.getValue<T>(); 
    };
    
    inline Property_<T>& operator*=(T const& theVal)
    {
        *this = getValue<T>() * theVal; 
        return *this;
    };
    
    inline Property_<T>& operator*=(Property_<T> const& otherProp)
    {
        *this = getValue<T>() + otherProp.getValue<T>(); 
        return *this;
    };
    
    inline const T operator*(Property_<T> const& otherProp)
    {
        return getValue<T>() * otherProp.getValue<T>(); 
    };
    
    inline Property_<T>& operator/=(T const& theVal)
    {
        *this = getValue<T>() / theVal; 
        return *this;
    };
    
    inline Property_<T>& operator/=(Property_<T> const& otherProp)
    {
        *this = getValue<T>() / otherProp.getValue<T>(); 
        return *this;
    };
    
    inline const T operator/(Property_<T> const& otherProp)
    {
        return getValue<T>() / otherProp.getValue<T>(); 
    };
    
    inline friend const T operator+(T theVal, const Property_<T>& theProp)
    {
        return theVal + theProp.getValue<T>(); 
    };
    
    inline friend const T operator+(const Property_<T>& theProp, T theVal)
    {
        return theVal + theProp.getValue<T>(); 
    };

    inline friend const T operator-(T theVal, const Property_<T>& theProp)
    {
        return theVal - theProp.getValue<T>(); 
    };
    
    inline friend const T operator-(const Property_<T>& theProp, T theVal)
    {
        return theProp.getValue<T>() - theVal; 
    };
    
    inline friend const T operator*(T theVal, const Property_<T>& theProp)
    {
        return theVal * theProp.getValue<T>(); 
    };
    
    inline friend const T operator*(const Property_<T>& theProp, T theVal)
    {
        return theVal * theProp.getValue<T>(); 
    };
    
    inline friend const T operator/(T theVal, const Property_<T>& theProp)
    {
        return theVal / theProp.getValue<T>(); 
    };
    
    inline friend const T operator/(const Property_<T>& theProp, T theVal)
    {
        return theProp.getValue<T>() / theVal; 
    };
    
    friend std::ostream& operator<<(std::ostream &os,const Property_<T>& theProp)
    {
        os<< theProp.getName()<<": "<<theProp.getValue<T>()<<std::endl;
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
class RangedProperty : public Property_<T>
{
public:
    
    typedef std::shared_ptr< RangedProperty<T> > Ptr;
    
    inline RangedProperty<T>& operator=(T const& theVal)
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
            v = std::min(v, m_max);
            v = std::max(v, m_min);
            this->set(v);
            return false;
        }
        
        return true;
    };
    
    friend std::ostream& operator<<(std::ostream &os,const RangedProperty<T>& theProp)
    {
        T min, max;
        theProp.getRange(min, max);
        
        os<< theProp.getName()<<": "<<theProp.val()<<" ( "<<min<<" - "<<max<<" )\n";
        return os;
    }

private:

    RangedProperty():Property_<T>(){};
    RangedProperty(const std::string &theName, const T &theValue,
                    const T &min, const T &max):
    Property_<T>(theName, theValue)
    {
        setRange(min, max);
        checkValue(theValue);
    };
    
    inline void rangeCheck(const T &theValue)
    {
        // check range
        if( m_min > theValue || m_max < theValue )
            throw BadBoundsException(this->getName());
    }
    
    T m_min, m_max;
};

}

#endif // _KINSKIProperty__INCLUDED__
