#include "UUID.hpp"

namespace kinski{ namespace bluetooth{

namespace
{
    const bluetooth::UUID
    BASE_BLE_UUID = bluetooth::UUID("00000000-0000-1000-8000-00805F9B34FB");
}

UUID::UUID()
{
    // start with generic base UUID
    *this = BASE_BLE_UUID;
}

UUID& UUID::operator=(const UUID &the_other)
{
    memcpy(m_data, the_other.bytes(), 16);
    return *this;
}

UUID::UUID(const std::string &the_str)
{
    // does the string contain hex - 0x? -> remove it
    auto input_str = the_str;
    if(input_str.size() > 1 && input_str[0] == '0' && input_str[1] == 'x')
    {
        input_str = input_str.substr(2);
    }

    if(the_str.size() == 36)
    {
        // remove "-"s
        auto str = the_str.substr(0, 8);
        str += the_str.substr(9, 4);
        str += the_str.substr(14, 4);
        str += the_str.substr(19, 4);
        str += the_str.substr(24, 12);

        for(uint8_t i = 0; i < 16; i++){ m_data[i] = std::stoul(str.substr(i * 2, 2), 0 , 16); }
    }
    else if(input_str.size() == 4 || input_str.size() == 8)
    {
        UUID::Type t = input_str.size() == 4 ? UUID::UUID_16 : UUID::UUID_32;
        const size_t num_bytes = input_str.size() / 2;
        uint8_t bytes[num_bytes];

        for(uint8_t i = 0; i < num_bytes; i++)
        {
            bytes[i] = std::stoul(input_str.substr(i * 2, 2), 0 , 16);
        }
        *this = UUID(bytes, t);
    }
    else
    {
        LOG_WARNING << "UUID-string has invalid size: " << the_str;

        // start with generic base UUID bytes
        *this = BASE_BLE_UUID;
    }
}

UUID::UUID(const uint8_t *the_bytes, Type t)
{
    *this = BASE_BLE_UUID;

    size_t bytes_to_copy = 0, offset = 0;

    switch (t)
    {
        case UUID_16:
            bytes_to_copy = 2;
            offset = 2;
            break;
        case UUID_32:
            bytes_to_copy = 4;
            break;
        case UUID_128:
            bytes_to_copy = 16;
            break;
    }
    memcpy(m_data + offset, the_bytes, bytes_to_copy);
}

const uint8_t* UUID::bytes() const { return m_data; }

const std::string UUID::to_string() const
{
    std::stringstream ss;

    for(int i = 0; i < 16; i++)
    {
        ss << std::setfill('0') << std::setw(2) << std::hex << (int)m_data[i];
    }
    auto ret = ss.str();
    ret.insert(8, "-");
    ret.insert(13, "-");
    ret.insert(18, "-");
    ret.insert(23, "-");
    return ret;
}

bool UUID::operator==(const UUID &the_other) const
{
    for(int i = 0; i < 16; i++){ if(m_data[i] != the_other.m_data[i]){ return false; } }
    return true;
}

bool UUID::operator!=(const UUID &the_other) const { return !(*this == the_other); }

bool UUID::operator<(const UUID &the_other) const
{
    for(int i = 0; i < 16; i++)
    {
        if(m_data[i] < the_other.m_data[i]){ return true; }
        else if(m_data[i] > the_other.m_data[i]){ return false; }
    }
    return false;
}

}}// namespaces
