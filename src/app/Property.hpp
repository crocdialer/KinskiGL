// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "crocore/crocore.hpp"
#include <any>

namespace crocore {

DEFINE_CLASS_PTR(Property)

class Property : public std::enable_shared_from_this<Property>
{
public:

    virtual ~Property() = default;

    inline std::any get_value() const { return m_value; };

    const std::string &name() const;

    void set_name(const std::string &theName);

    void set_tweakable(bool isTweakable);

    bool tweakable() const;

    bool empty() const { return m_value.has_value(); };

    template<typename T>
    inline void set_value(const T &theValue)
    {
        if(!is_of_type<T>()){ throw WrongTypeSetException(name()); }
        if(check_value(theValue))
        {
            m_value = theValue;
            notify_observers();
        }
    }

    template<typename T>
    inline const T &get_value() const
    {
        try { return *std::any_cast<T>(&m_value); }
        catch(const std::bad_any_cast &e) { throw WrongTypeGetException(name()); }
    }

    template<typename T>
    inline T &get_value()
    {
        try { return *std::any_cast<T>(&m_value); }
        catch(const std::bad_any_cast &e) { throw WrongTypeGetException(name()); }
    }

    template<typename T>
    inline T *get_value_ptr()
    {
        try { return std::any_cast<T>(&m_value); }
        catch(const std::bad_any_cast &e) { throw WrongTypeGetException(name()); }
    }

    template<typename C>
    inline bool is_of_type() const
    {
        return m_value.type() == typeid(C);
    }

    virtual bool check_value(const std::any &theVal) { return theVal.type() == m_value.type(); };

    DEFINE_CLASS_PTR(Observer);

    class Observer
    {
    public:
        virtual void update_property(const PropertyConstPtr &theProperty) = 0;
    };

    void add_observer(const ObserverPtr &theObs);

    void remove_observer(const ObserverPtr &theObs);

    void clear_observers();

    void notify_observers();

protected:
    Property();

    Property(const std::string &theName, std::any theValue);

private:
    std::shared_ptr<struct PropertyImpl> m_impl;
    std::any m_value;

public:
    // define std::runtime_errors
    class WrongTypeSetException : public std::runtime_error
    {
    public:
        explicit WrongTypeSetException(const std::string &thePropertyName) :
                std::runtime_error(std::string("Wrong type in setValue for Property: ") + thePropertyName
                                   + std::string(" - Only the original type can be set.")) {}
    };

    class WrongTypeGetException : public std::runtime_error
    {
    public:
        explicit WrongTypeGetException(const std::string &thePropertyName) :
                std::runtime_error(std::string("Wrong type in getValue for Property: ") + thePropertyName) {}
    };

};

template<typename T>
class Property_ : public Property
{
public:
    typedef std::shared_ptr<Property_<T>> Ptr;
    typedef std::weak_ptr<Property_<T>> WeakPtr;

    static Ptr create(const std::string &theName, const T &theValue = T())
    {
        Ptr outPtr(new Property_(theName, theValue));
        return outPtr;
    };

    inline const T &value() const { return get_value<T>(); };

    inline T &value() { return get_value<T>(); };

    inline void set(const T &theVal) { set_value<T>(theVal); };

    inline void value(const T &theVal) { set(theVal); };

    inline operator const T &() const { return value(); }

    inline Property_<T> &operator=(const Property_<T> &theProp)
    {
        set_name(theProp.name());
        set(theProp.value());
        return *this;
    };

    virtual inline Property_<T> &operator=(const T &theVal)
    {
        set(theVal);
        return *this;
    };

    inline Property_<T> &operator+=(const T &theVal)
    {
        set(value() + theVal);
        return *this;
    };

    inline Property_<T> &operator-=(const T &theVal)
    {
        set(value() - theVal);
        return *this;
    };

    inline Property_<T> &operator*=(const T &theVal)
    {
        set(value() * theVal);
        return *this;
    };

    inline Property_<T> &operator/=(const T &theVal)
    {
        set(value() / theVal);
        return *this;
    };

    friend std::ostream &operator<<(std::ostream &os, const Property_<T> &theProp)
    {
        os << theProp.name() << ": " << theProp.get_value<T>();
        return os;
    }

protected:
    Property_() : Property() {};

    Property_(const std::string &theName, const T &theValue) :
            Property(theName, theValue) {};

    Property_(const Property_<T> &other) :
            Property(other.name(), other.get_value<T>()) {};

};

// avoids GCC warning int-in_bool-context
template<>
Property_<bool>& Property_<bool>::operator*=(const bool &theVal);

extern template
class Property_<int32_t>;

extern template
class Property_<uint32_t>;

extern template
class Property_<int64_t>;

extern template
class Property_<uint64_t>;

extern template
class Property_<float>;

extern template
class Property_<double>;

extern template
class Property_<bool>;

template<typename T>
class RangedProperty : public Property_<T>
{
public:

    typedef std::shared_ptr<RangedProperty<T>> Ptr;

    ~RangedProperty<T>() override = default;

    inline RangedProperty<T> &operator=(const T &theVal) override
    {
        this->set(theVal);
        return *this;
    };

    class BadBoundsException : public std::runtime_error
    {
    public:
        explicit BadBoundsException(const std::string &property_name) :
                std::runtime_error(std::string("Bad bounds set for Property: ") + property_name) {}
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
        if(min > max)
            throw BadBoundsException(this->name());
        m_min = min;
        m_max = max;
        check_value(this->get_value());
    };

    std::pair<T, T> range() const
    {
        return std::make_pair(m_min, m_max);
    };

    bool check_value(const std::any &theVal) override
    {
        T v;

        try
        {
            v = std::any_cast<T>(theVal);
            range_check(v);
        }
        catch(const std::bad_any_cast &e)
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

    friend std::ostream &operator<<(std::ostream &os, const RangedProperty<T> &theProp)
    {
        T min = theProp.range().first, max = theProp.range().second;
        os << theProp.name() << ": " << theProp.value() << " (" << min << " - " << max << ")";
        return os;
    }

private:

    RangedProperty() : Property_<T>() {};

    RangedProperty(const std::string &theName, const T &theValue, const T &min, const T &max) :
            Property_<T>(theName, theValue)
    {
        set_range(min, max);
        check_value(theValue);
    };

    inline void range_check(const T &theValue)
    {
        if(m_min > theValue || m_max < theValue)
            throw BadBoundsException(this->name());
    }

    T m_min, m_max;
};

extern template
class RangedProperty<int32_t>;

extern template
class RangedProperty<uint32_t>;

extern template
class RangedProperty<int64_t>;

extern template
class RangedProperty<uint64_t>;

extern template
class RangedProperty<float>;

extern template
class RangedProperty<double>;
}
