#include "lobby.h"

#include "water/componet/logger.h"
#include "water/componet/string_kit.h"
#include "base/tcp_message.h"

namespace lobby{

Lobby* Lobby::m_me = nullptr;

Lobby& Lobby::me()
{
    return *m_me;
}

void Lobby::init(int32_t num, const std::string& configDir, const std::string& logDir)
{
    m_me = new Lobby(num, configDir, logDir);
}

Lobby::Lobby(int32_t num, const std::string& configDir, const std::string& logDir)
    : Process("lobby", num, configDir, logDir)
{
}

void Lobby::init()
{
    Process::init();

    //using namespace std::placeholders;
}

void Lobby::tcpPacketHandle(TcpPacket::Ptr packet,
                     TcpConnectionManager::ConnectionHolder::Ptr conn,
                     const componet::TimePoint& now)
{
    ProcessId senderId(conn->id);

    auto envelope = reinterpret_cast<water::process::Envelope*>(packet->content());
    if(envelope == nullptr)
    {
        LOG_DEBUG("relay packet failed, TcpPacket::content() == nullptr, from={}, packetSize={}",
                  senderId, packet->size());
        return;
    }


    //目标进程Id
    ProcessId receiverId(envelope->targetPid);
    if(receiverId.num() == 0) //广播
    {
        m_conns.broadcastPacketToPrivate(receiverId.type(), packet);
        LOG_DEBUG("relay broadcast packet, {}->{}, code={}, length={},", 
                  senderId, ProcessId::typeToString(receiverId.type()), envelope->msg.code, packet->size() );
    }
    else
    {
        auto ret = m_conns.sendPacketToPrivate(receiverId, packet) ? "successed" : "failed";
        LOG_DEBUG("relay packet {}, {}->{}, code={}, length={},", 
                  ret, senderId, receiverId, envelope->msg.code, packet->size());
    }
}

}

