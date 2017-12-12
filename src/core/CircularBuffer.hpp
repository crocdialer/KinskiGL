//
//  CircularBuffer.hpp
//  kinskiGL
//
//  Created by Fabian on 13/09/16.
//
//

#pragma once

#include "core/core.hpp"

namespace kinski
{

template<typename T, typename Allocator = std::allocator<T>>
class CircularBuffer
{
public:
    
    using value_type = T;
    
    CircularBuffer(uint32_t the_cap = 10):
    m_array_size(the_cap + 1),
    m_first(0),
    m_last(0),
    m_data(new T[m_array_size])
    {
        memset(m_data, 0, m_array_size * sizeof(T));
    }
    
    // copy constructor
    CircularBuffer(const CircularBuffer& other):
    m_array_size(other.m_array_size),
    m_first(other.m_first),
    m_last(other.m_last),
    m_data(new T[other.m_array_size])
    {
        memcpy(m_data, other.m_data, m_array_size * sizeof(T));
    }
    
    // move constructor
    CircularBuffer(CircularBuffer&& other):
    m_data(other.m_data),
    m_array_size(other.m_array_size),
    m_first(other.m_first),
    m_last(other.m_last)
    {
        other.m_data = nullptr;
    }
    
    ~CircularBuffer()
    {
        delete[](m_data);
    }
    
    CircularBuffer& operator=(CircularBuffer other)
    {
        std::swap(m_array_size, other.m_array_size);
        std::swap(m_first, other.m_first);
        std::swap(m_last, other.m_last);
        std::swap(m_data, other.m_data);
        return *this;
    }
    
    inline void clear()
    {
        m_first = m_last = 0;
    }
    
    template<class InputIt>
    void assign(InputIt first, InputIt last)
    {
        clear();
        std::copy(first, last, std::back_inserter(*this));
    }
    
    inline void push_back(const T &the_val)
    {
        m_data[m_last] = the_val;
        m_last = (m_last + 1) % m_array_size;
        
        // buffer is full, drop oldest value
        if(m_first == m_last){ m_first = (m_first + 1) % m_array_size; }
    }
    
    inline void pop_front()
    {
        if(!empty()){ m_first = (m_first + 1) % m_array_size; }
    }
    
    inline T& front(){ return m_data[m_first]; }
    inline T& back(){ return m_data[(m_last - 1) % m_array_size]; }
    inline const T& front() const { return m_data[m_first]; }
    inline const T& back() const { return m_data[(m_last - 1) % m_array_size]; }
    
    inline uint32_t capacity() const { return m_array_size - 1; };
    inline void set_capacity(uint32_t the_cap){ *this = CircularBuffer(the_cap); }
    
    inline size_t size() const
    {
        int ret = m_last - m_first;
        if(ret < 0){ ret += m_array_size; }
        return ret;
    };
    
    inline bool empty() const { return m_first == m_last; }
    
    inline T& operator[](uint32_t the_index)
    {
        if(the_index >= size()){ throw Exception("Out of bounds"); }
        return m_data[(m_first + the_index) % m_array_size];
    };
    
    inline const T& operator[](uint32_t the_index) const
    {
        if(the_index >= size()){ throw Exception("Out of bounds"); }
        return m_data[(m_first + the_index) % m_array_size];
    };
    
    class iterator: public std::iterator<std::random_access_iterator_tag, T>
    {
        friend CircularBuffer<T>;
        
        const CircularBuffer<T>* m_buf;
        T *m_array, *m_ptr;
        uint32_t m_size;
        
    public:
        
        iterator():
        m_buf(nullptr),
        m_array(nullptr),
        m_ptr(nullptr),
        m_size(0){}
        
        iterator(const iterator &the_other):
        m_buf(the_other.m_buf),
        m_array(the_other.m_array),
        m_ptr(the_other.m_ptr),
        m_size(the_other.m_size){}
        
        iterator(const CircularBuffer<T>* the_buf, uint32_t the_pos):
        m_buf(the_buf),
        m_array(the_buf->m_data),
        m_ptr(the_buf->m_data + the_pos),
        m_size(the_buf->m_array_size){}
        
        inline iterator& operator=(iterator the_other)
        {
            std::swap(m_buf, the_other.m_buf);
            std::swap(m_array, the_other.m_array);
            std::swap(m_ptr, the_other.m_ptr);
            std::swap(m_size, the_other.m_size);
            return *this;
        }
        
        inline iterator& operator++()
        {
            m_ptr++;
            if(m_ptr >= (m_array + m_size)){ m_ptr -= m_size;}
            return *this;
        }
        inline iterator& operator--()
        {
            m_ptr--;
            if(m_ptr < m_array){ m_ptr += m_size;}
            return *this;
        }
        inline bool operator==(const iterator &the_other) const
        {
            return m_array == the_other.m_array && m_ptr == the_other.m_ptr;
        }
        inline bool operator!=(const iterator &the_other) const { return !(*this == the_other); }
        inline T& operator *() { return *m_ptr; }
        inline const T& operator *() const { return *m_ptr; }
        inline T* operator->(){ return m_ptr; }
        inline const T* operator->() const { return m_ptr; }
        
        inline iterator& operator+(int i)
        {
            m_ptr += i;
            if(m_ptr >= (m_array + m_size)){ m_ptr -= m_size;}
            if(m_ptr < m_array){ m_ptr += m_size;}
            return *this;
        }
        
        inline iterator& operator-(int i)
        {
            return *this + (-i);
        }
        
        inline int operator-(const iterator &the_other) const
        {
            int index = m_ptr - m_array - m_buf->m_first;
            index += index < 0 ? m_size : 0;
            int other_index = the_other.m_ptr - m_array - m_buf->m_first;
            other_index += index < 0 ? m_size : 0;
            return index - other_index;
        }
    };
    typedef const iterator const_iterator;
    
    iterator begin(){ return iterator(this, m_first); }
    const_iterator begin() const { return iterator(this, m_first); }
    iterator end(){ return iterator(this, m_last); }
    const_iterator end() const { return iterator(this, m_last); }
private:
    
    int32_t m_array_size, m_first, m_last;
    T* m_data;
};

}// namespace
