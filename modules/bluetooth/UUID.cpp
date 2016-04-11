#include "UUID.hpp"

namespace kinski{ namespace bluetooth{

UUID::UUID()
{
    for(int i = 0; i < 16; i++){ m_data[i] = kinski::random_int<uint8_t>(0, 255); }
}

UUID::UUID(const std::string &the_str)
{
    // remove "-"s
    auto str = the_str.substr(0, 8);
    str += the_str.substr(9, 4);
    str += the_str.substr(14, 4);
    str += the_str.substr(19, 4);
    str += the_str.substr(24, 12);

    for(int i = 0; i < 16; i++){ m_data[i] = std::stoul(str.substr(i * 2, 2), 0 , 16); }
}

UUID::UUID(uint8_t the_bytes[16])
{
    memcpy(m_data, the_bytes, 16);
}

const uint8_t* UUID::bytes() const{ return m_data; }

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
        if(m_data[i] == the_other.m_data[i]){ continue; }
        if(m_data[i] < the_other.m_data[i]){ return true; }
    }
    return false;
}

}}// namespaces
