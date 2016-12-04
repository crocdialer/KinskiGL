// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
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
    
    DEFINE_CLASS_PTR(Observer);
    
    class Observer
    {
    public:
        virtual void update_property(const Property::ConstPtr &theProperty) = 0;
    };

    inline boost::any get_value() const {return m_value;};
    inline const std::string& name() const {return m_name;};
    inline void set_name(const std::string& theName) {m_name = theName;};
	inline void set_tweakable(bool isTweakable) {m_tweakable = isTweakable;};
	inline bool tweakable() const {return m_tweakable;};
    inline bool empty() const {return m_value.empty();};

    template <typename T> 
    inline void set_value(const T& theValue)
    {
        if (!is_of_type<T>()) {throw WrongTypeSetException(m_name);}
        if(check_value(theValue))
        {
            m_value = theValue;
            notify_observers();
        }
    }
   
    template <typename T>
    inline const T& get_value() const
    {
        try{return *boost::any_cast<T>(&m_value);}
        catch (const boost::bad_any_cast &theException){throw WrongTypeGetException(m_name);}
    }
    
    template <typename T>
    inline T& get_value()
    {
        try{return *boost::any_cast<T>(&m_value);}
        catch (const boost::bad_any_cast &theException){throw WrongTypeGetException(m_name);}
    }
    
    template <typename T>
    inline T* get_value_ptr()
    {
        try{return boost::any_cast<T>(&m_value);}
        catch (const boost::bad_any_cast &theException){throw WrongTypeGetException(m_name);}
    }
    
    template <typename C>
    inline bool is_of_type() const
    {
        return m_value.type() == typeid(C);
    }
    
    virtual bool check_value(const boost::any &theVal)
    {return theVal.type() == m_value.type();};
    
    inline void add_observer(const ObserverPtr &theObs)
    {
        m_signal.connect(signal_t::slot_type(&Observer::update_property, theObs, _1).track_foreign(theObs));
    }
    
    inline void remove_observer(const ObserverPtr &theObs)
    {
        m_signal.disconnect(boost::bind(&Observer::update_property, theObs, _1));
    }
    
    inline void clear_observers(){m_signal.disconnect_all_slots();}
    inline void notify_observers(){m_signal(shared_from_this());}

protected:
    Property(): m_tweakable(true){}; // default constructor
    Property(const std::string &theName, const boost::any &theValue):
    m_name(theName), m_value(theValue), m_tweakable(true){};
    
private:
    std::string m_name;
    boost::any m_value;
	bool m_tweakable;
    
    typedef boost::signals2::signal<void(const Property::ConstPtr&)> signal_t;
    signal_t m_signal;

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
    
    inline const T& value() const {return get_value<T>();};
    inline T& value() {return get_value<T>();};
    inline void set(const T &theVal){set_value<T>(theVal);};
    inline void value(const T &theVal){set(theVal);};
    inline operator const T&() const { return value(); }
    
    inline Property_<T>& operator=(const Property_<T> &theProp)
    {
        set_name(theProp.name());
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
        set(value() + theVal);
        return *this;
    };
    
    inline Property_<T>& operator-=(const T &theVal)
    {
        set(value() - theVal);
        return *this;
    };
    
    inline Property_<T>& operator*=(const T &theVal)
    {
        set(value() * theVal);
        return *this;
    };
    
    inline Property_<T>& operator/=(const T &theVal)
    {
        set(value() / theVal);
        return *this;
    };
    
    friend std::ostream& operator<<(std::ostream &os,const Property_<T>& theProp)
    {
        os<< theProp.name()<<": "<<theProp.get_value<T>();
        return os;
    }
        
protected:
    Property_():Property(){};
    
    Property_(const std::string &theName, const T &theValue):
    Property(theName, theValue){};
    
    explicit Property_(const Property_<T> &other):
    Property(other.name(), other.get_value<T>()){};

};

extern template class Property_<int32_t>;
extern template class Property_<uint32_t>;
extern template class Property_<int64_t>;
extern template class Property_<uint64_t>;
extern template class Property_<float>;
extern template class Property_<double>;
extern template class Property_<bool>;
//extern template class Property_<std::string>;
//extern template class Property_<std::vector<std::string>>;
    
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
    
    void set_range(const T &min, const T &max)
    {
        if( min > max )
            throw BadBoundsException(this->name());
        m_min = min;
        m_max = max;
        check_value(this->get_value());
    };
    
    std::pair<T, T> range() const
    {
        return std::make_pair(m_min, m_max);
    };

    bool check_value(const boost::any &theVal)
    {
        T v;
        
        try
        {
            v = boost::any_cast<T>(theVal);
            range_check(v);
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
        T min = theProp.range().first, max = theProp.range().second;
        os<< theProp.name() << ": " << theProp.value() << " (" << min << " - " << max << ")";
        return os;
    }

private:

    RangedProperty():Property_<T>(){};
    RangedProperty(const std::string &theName, const T &theValue, const T &min, const T &max):
    Property_<T>(theName, theValue)
    {
        set_range(min, max);
        check_value(theValue);
    };
    
    inline void range_check(const T &theValue)
    {
        if( m_min > theValue || m_max < theValue )
            throw BadBoundsException(this->name());
    }
    
    T m_min, m_max;
};

}
