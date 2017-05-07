#include "router.h"

#include "water/componet/logger.h"
#include "water/componet/string_kit.h"
#include "base/tcp_message.h"

namespace router{

Router* Router::m_me = nullptr;

Router& Router::me()
{
    return *m_me;
}

void Router::init(int32_t num, const std::string& configDir, const std::string& logDir)
{
    m_me = new Router(num, configDir, logDir);
}

Router::Router(int32_t num, const std::string& configDir, const std::string& logDir)
    : Process("router", num, configDir, logDir)
{
}

void Router::init()
{
    Process::init();

    //using namespace std::placeholders;
}

void Router::tcpPacketHandle(TcpPacket::Ptr packet,
                     TcpConnectionManager::ConnectionHolder::Ptr conn,
                     const componet::TimePoint& now)
{
    ProcessIdentity senderId(conn->id);

    auto envelope = reinterpret_cast<water::process::Envelope*>(packet->content());
    if(envelope == nullptr)
    {
        LOG_DEBUG("relay packet failed, TcpPacket::content() == nullptr, from={}, packetSize={}",
                  senderId, packet->size());
        return;
    }


    //目标进程Id
    ProcessIdentity receiverId(envelope->targetPid);
    if(receiverId.num() == 0) //广播
    {
        m_conns.broadcastPacketToPrivate(receiverId.type(), packet);
        LOG_DEBUG("relay broadcast packet, {}->{}, code={}, length={},", 
                  senderId, ProcessIdentity::typeToString(receiverId.type()), envelope->msg.code, packet->size() );
    }
    else
    {
        auto ret = m_conns.sendPacketToPrivate(receiverId, packet) ? "successed" : "failed";
        LOG_DEBUG("relay packet {}, {}->{}, code={}, length={},", 
                  ret, senderId, receiverId, envelope->msg.code, packet->size());
    }
}

}

