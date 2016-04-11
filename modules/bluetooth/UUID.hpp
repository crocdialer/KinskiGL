#pragma once
#include "core/core.hpp"

namespace kinski{ namespace bluetooth{

class UUID
{
public:
    UUID();
    UUID(const std::string &the_str);
    explicit UUID(uint8_t the_bytes[16]);

    const uint8_t* bytes() const;
    const std::string string() const;

    bool operator==(const UUID &the_other) const;
    bool operator!=(const UUID &the_other) const;
    bool operator<(const UUID &the_other) const;

private:
    uint8_t m_data[16];
};

}}// namespaces
