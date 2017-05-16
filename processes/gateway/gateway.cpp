#include "gateway.h"

#include "water/componet/logger.h"
#include "water/componet/scope_guard.h"
#include "water/net/endpoint.h"
#include "base/tcp_message.h"
#include "protocol/rawmsg/rawmsg_manager.h"

namespace gateway{

using protocol::rawmsg::RawmsgManager;


Gateway* Gateway::m_me = nullptr;

Gateway& Gateway::me()
{
    return *m_me;
}

void Gateway::init(int32_t num, const std::string& configDir, const std::string& logDir)
{
    m_me = new Gateway(num, configDir, logDir);
}

Gateway::Gateway(int32_t num, const std::string& configDir, const std::string& logDir)
: Process("gateway", num, configDir, logDir)
{
}

void Gateway::init()
{
    //先执行基类初始化
    process::Process::init();

    loadConfig();

    using namespace std::placeholders;

    if(m_publicNetServer == nullptr)
        EXCEPTION(componet::ExceptionBase, "无公网监听，请检查配置")

    //create checker
    m_clientManager = ClientManager::create(getId());

    //处理新建立的客户连接
    m_publicNetServer->e_newConn.reg(std::bind(&Gateway::newClientConnection, this, _1));

    //已加入connectionManager的连接被断开时的处理
    m_conns.e_afterErasePublicConn.reg(std::bind(&ClientManager::clientOffline, m_clientManager, _1));

    //m_timer.regEventHandler(std::chrono::milliseconds(100), std::bind(&LoginProcessor::timerExec, &LoginProcessor::me(), std::placeholders::_1));

    //注册消息处理事件和主定时器事件
    registerTcpMsgHandler();
    registerTimerHandler();
}

void Gateway::lanchThreads()
{
    Process::lanchThreads();
}

bool Gateway::sendToPrivate(ProcessId pid, TcpMsgCode code)
{
    return sendToPrivate(pid, code, nullptr, 0);
}

bool Gateway::sendToPrivate(ProcessId pid, TcpMsgCode code, const ProtoMsg& proto)
{
    return relayToPrivate(getId().value(), pid, code, proto);
}

bool Gateway::sendToPrivate(ProcessId pid, TcpMsgCode code, const void* raw, uint32_t size)
{
    return relayToPrivate(getId().value(), pid, code, raw, size);
}

bool Gateway::relayToPrivate(uint64_t sourceId, ProcessId pid, TcpMsgCode code, const ProtoMsg& proto)
{
    const uint32_t protoBinSize = proto.ByteSize();
    const uint32_t bufSize = sizeof(Envelope) + protoBinSize;
    uint8_t* buf = new uint8_t[bufSize];
    ON_EXIT_SCOPE_DO(delete[] buf);

    Envelope* envelope = new(buf) Envelope(code);
    envelope->targetPid  = pid.value();
    envelope->sourceId = sourceId;

    if(!proto.SerializeToArray(envelope->msg.data, protoBinSize))
    {
        LOG_ERROR("proto serialize failed, msgCode = {}", code);
        return false;
    }

    TcpPacket::Ptr packet = TcpPacket::create();
    packet->setContent(buf, bufSize);

    const ProcessId routerId("router", 1);
    return m_conns.sendPacketToPrivate(routerId, packet);
}

bool Gateway::relayToPrivate(uint64_t sourceId, ProcessId pid, TcpMsgCode code, const void* raw, uint32_t size)
{
    const uint32_t bufSize = sizeof(Envelope) + size;
    uint8_t* buf = new uint8_t[bufSize];
    ON_EXIT_SCOPE_DO(delete[] buf);

    Envelope* envelope  = new(buf) Envelope(code);
    envelope->targetPid = pid.value();
    envelope->sourceId  = sourceId;
    std::memcpy(envelope->msg.data, raw, size);

    TcpPacket::Ptr packet = TcpPacket::create();
    packet->setContent(buf, bufSize);

    LOG_DEBUG("sendToPrivate, code={}, rawSize={}, tcpMsgSize={}, packetSize={}, contentSize={}", 
              code, size, bufSize, packet->size(), *(uint32_t*)(packet->data()));

    const ProcessId routerId("router", 1);
    return m_conns.sendPacketToPrivate(routerId, packet);
}

bool Gateway::sendToClient(ClientConnectionId ccid, TcpMsgCode code, const void* raw, uint32_t size)
{
    const uint32_t bufSize = sizeof(TcpMsg) + size;
    uint8_t* buf = new uint8_t[bufSize];
    ON_EXIT_SCOPE_DO(delete[] buf);

    TcpMsg* msg = new(buf) TcpMsg(code);
    std::memcpy(msg->data, raw, size);

    TcpPacket::Ptr packet = TcpPacket::create();
    packet->setContent(buf, bufSize);

    return m_conns.sendPacketToPublic(ccid, packet);
}

bool Gateway::sendToClient(ClientConnectionId ccid, TcpMsgCode code, const ProtoMsg& proto)
{
    const uint32_t protoBinSize = proto.ByteSize();
    const uint32_t bufSize = sizeof(TcpMsg) + protoBinSize;
    uint8_t* buf = new uint8_t[bufSize];
    ON_EXIT_SCOPE_DO(delete[] buf);

    TcpMsg* msg = new(buf) TcpMsg(code);
    if(!proto.SerializeToArray(msg->data, protoBinSize))
    {
        LOG_ERROR("proto serialize failed, msgCode = {}", code);
        return false;
    }

    TcpPacket::Ptr packet = TcpPacket::create();
    packet->setContent(buf, bufSize);

    return m_conns.sendPacketToPublic(ccid, packet);
}

net::PacketConnection::Ptr Gateway::eraseClientConn(ClientConnectionId ccid)
{
    return m_conns.erasePublicConnection(ccid);
}

void Gateway::loadConfig()
{
}

void Gateway::newClientConnection(net::PacketConnection::Ptr conn)
{
    if(conn == nullptr)
        return;

    try
    {
        conn->setNonBlocking();
        conn->setRecvPacket(process::TcpPacket::create());

        ClientConnectionId ccid = m_clientManager->clientOnline();
        if (ccid == INVALID_CCID_VALUE)
        {
            LOG_ERROR("Gateway::newClientConnection failed, 加入ClientManager失败, {}", conn->getRemoteEndpoint());
            return;
        }
        if (!m_conns.addPublicConnection(conn, ccid))
        {
            m_clientManager->clientOffline(ccid);
        }
    }
    catch (const net::NetException& ex)
    {
        LOG_ERROR("Gateway::newClientConnection failed, {}, {}", conn->getRemoteEndpoint(), ex);
    }
}

void Gateway::tcpPacketHandle(TcpPacket::Ptr packet, 
                               TcpConnectionManager::ConnectionHolder::Ptr conn,
                               const componet::TimePoint& now)
{
    if(packet == nullptr || packet->contentSize() < sizeof(water::process::TcpMsg))
        return;

//    LOG_DEBUG("tcpPacketHandle, packetsize()={}, contentSize={}", 
//              packet->size(), packet->contentSize());

    auto tcpMsg = reinterpret_cast<water::process::TcpMsg*>(packet->content());
    if(water::process::isRawMsgCode(tcpMsg->code))
        RawmsgManager::me().dealTcpMsg(tcpMsg, packet->contentSize(), conn->id, now);
    else if(water::process::isProtobufMsgCode(tcpMsg->code))
        ProtoManager::me().dealTcpMsg(tcpMsg, packet->contentSize(), conn->id, now);
}

}

