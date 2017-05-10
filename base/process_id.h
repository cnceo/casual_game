/*
 * Author: LiZhaojia 
 *
 * Last modified: 2014-11-27 15:45 +0800
 *
 * Description: 进程标识
 */


#ifndef WATER_PROCESS_ID_H
#define WATER_PROCESS_ID_H


#include <string>
#include <vector>
#include <map>


namespace water{
namespace process{

//客户端ID
typedef uint64_t ClientIdentity;
const ClientIdentity INVALID_CLIENT_IDENDITY_VALUE = 0;

// ProcessType, 实际仅使用低8bits
typedef uint32_t ProcessType;
const ProcessType INVALID_PROCESS_TYPE = 0;

// ProcessNum，实际仅使用低8bits
typedef uint32_t ProcessNum;
// ProcessIdentityValue =  |- 16bits reserve -|- 8bits ProcessType -|- 8bits ProcessNum -|
typedef uint32_t ProcessIdentityValue;
const ProcessIdentityValue INVALID_PROCESS_IDENDITY_VALUE = 0;

//服务器ID
class ProcessIdentity
{
public:


    ProcessIdentity(const std::string& typeStr, int8_t num);
    ProcessIdentity(ProcessIdentityValue value_ = INVALID_PROCESS_IDENDITY_VALUE);

    //copy & assign
    ProcessIdentity(const ProcessIdentity& other) = default;
    ProcessIdentity& operator=(const ProcessIdentity& other) = default;

    void clear();
    bool isValid() const;

    std::string toString() const;

    void setValue(ProcessIdentityValue value);
    ProcessIdentityValue value() const;

    void type(ProcessType type);
    ProcessType type() const;

    void num(ProcessNum num);
    ProcessNum num() const;

private:
    ProcessType m_type = 0;
    ProcessNum m_num = 0;

public:
    static std::string typeToString(ProcessType type);
    static ProcessType stringToType(const std::string& str);

private:
    friend class ProcessConfig;
    static std::vector<std::string> s_type2Name;
    static std::map<std::string, ProcessType> s_name2Type;

};

bool operator==(const ProcessIdentity& pid1, const ProcessIdentity& pid2);
bool operator!=(const ProcessIdentity& pid1, const ProcessIdentity& pid2);
bool operator<(const ProcessIdentity& pid1, const ProcessIdentity& pid2);

}}

#endif
