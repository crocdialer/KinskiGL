#include "UUID.hpp"

namespace kinski{ namespace bluetooth{

namespace
{
    const bluetooth::UUID
    BASE_BLE_UUID = bluetooth::UUID("00000000-0000-1000-8000-00805F9B34FB");
}
    
UUID::UUID()
{
    // start with generic base UUID bytes
    memcpy(m_data, BASE_BLE_UUID.bytes(), 16);
    
//    for(int i = 0; i < 16; i++){ m_data[i] = kinski::random_int<uint8_t>(0, 255); }
}

UUID& UUID::operator=(const UUID &the_other)
{
    memcpy(m_data, the_other.bytes(), 16);
    return *this;
}
    
UUID::UUID(const std::string &the_str)
{
    if(the_str.size() == 36)
    {
        // remove "-"s
        auto str = the_str.substr(0, 8);
        str += the_str.substr(9, 4);
        str += the_str.substr(14, 4);
        str += the_str.substr(19, 4);
        str += the_str.substr(24, 12);
        
        for(int i = 0; i < 16; i++){ m_data[i] = std::stoul(str.substr(i * 2, 2), 0 , 16); }
    }
    else
    {
        LOG_WARNING << "invalid UUID-string: " << the_str;
        
        // start with generic base UUID bytes
        *this = BASE_BLE_UUID;
    }
}

UUID::UUID(uint8_t *the_bytes, Type t)
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

const std::string UUID::string() const
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
