/*
 * Author: LiZhaojia 
 *
 * Last modified: 2014-12-03 10:40 +0800
 *
 * Description: 
 */

#ifndef LOBBY_LOBBY_H
#define LOBBY_LOBBY_H

#include "base/process.h"

namespace lobby{

using namespace water;
using namespace process;

class Lobby : public Process
{
private:
    Lobby(int32_t num, const std::string& configDir, const std::string& logDir);

    void init() override;

    void tcpPacketHandle(TcpPacket::Ptr packet,
                         TcpConnectionManager::ConnectionHolder::Ptr conn,
                         const componet::TimePoint& now) override;

public:
    static void init(int32_t num, const std::string& configDir, const std::string& logDir);
    static Lobby& me();
private:
    static Lobby* m_me;
};

}

#endif

