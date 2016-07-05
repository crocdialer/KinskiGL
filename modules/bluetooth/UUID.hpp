#pragma once
#include "core/core.hpp"

namespace kinski{ namespace bluetooth{

class UUID
{
public:
    enum Type{UUID_16 = 2, UUID_32 = 4, UUID_128 = 16};
    
    UUID();
    UUID(const std::string &the_str);
    explicit UUID(const uint8_t *the_bytes, Type t = UUID_128);
    
    UUID& operator=(const UUID &the_other);
    
    const uint8_t* bytes() const;
    const std::string string() const;

    bool operator==(const UUID &the_other) const;
    bool operator!=(const UUID &the_other) const;
    bool operator<(const UUID &the_other) const;

private:
    uint8_t m_data[16];
};
    
}}// namespaces
