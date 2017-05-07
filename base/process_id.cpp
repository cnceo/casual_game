#include "process_id.h"

#include "process_config.h"
#include "componet/format.h"



namespace water{
namespace process{

std::vector<std::string> ProcessIdentity::s_type2Name = {"none"};
std::map<std::string, ProcessType> ProcessIdentity::s_name2Type;

std::string ProcessIdentity::typeToString(ProcessType type)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= s_type2Name.size())
        return "none";

    return s_type2Name[index];
}

ProcessType ProcessIdentity::stringToType(const std::string& str)
{
    auto it = s_name2Type.find(str);
    if(it == s_name2Type.end())
        return INVALID_PROCESS_TYPE;

    return it->second;
}

ProcessIdentity::ProcessIdentity(const std::string& typeStr, int8_t num)
{
    ProcessType type = ProcessIdentity::stringToType(typeStr);
    if(type == INVALID_PROCESS_TYPE)
    {
        m_type = 0;
        m_num = 0;
        return;
    }

    m_type   = type;
    m_num    = num;
}

ProcessIdentity::ProcessIdentity(ProcessIdentityValue value)
{
    setValue(value);
}

std::string ProcessIdentity::toString() const
{
    std::string ret = typeToString(m_type);
    componet::formatAndAppend(&ret, "-{}", m_num);
    return ret;
}

void ProcessIdentity::clear()
{
    m_type = 0;
    m_num = 0;
}

bool ProcessIdentity::isValid() const
{
    return typeToString(m_type) != "none" && m_num != 0;
}

ProcessIdentityValue ProcessIdentity::value() const
{
    return (ProcessIdentityValue(0) << 16u) | (m_type << 8u) | m_num;
}

void ProcessIdentity::setValue(ProcessIdentityValue value)
{
    m_type   = (value & 0xff00) >> 8u;
    m_num    = value & 0xff;
}

void ProcessIdentity::type(ProcessType type) 
{
    m_type = type;
}

ProcessType ProcessIdentity::type() const
{
    return m_type;
}

void ProcessIdentity::num(ProcessNum num)
{
    m_num = num;
}

ProcessNum ProcessIdentity::num() const
{
    return m_num;
}

/////////////////////////////////////////////////////////////////////
bool operator==(const ProcessIdentity& pid1, const ProcessIdentity& pid2)
{
    return pid1.value() == pid2.value();
}

bool operator!=(const ProcessIdentity& pid1, const ProcessIdentity& pid2)
{
    return pid1.value() != pid2.value();
}

bool operator<(const ProcessIdentity& pid1, const ProcessIdentity& pid2)
{
    return pid1.value() < pid2.value();
}


}}

