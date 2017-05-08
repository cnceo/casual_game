#include "gateway.h"

#include "login_processor.h"

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
    m_clientChecker = ClientConnectionChecker::create();
    //新的链接加入checker
    m_publicNetServer->e_newConn.reg(std::bind(&ClientConnectionChecker::addnewClientConnection,
                                               m_clientChecker, std::placeholders::_1));
    //checker初步处理过的连接交给loginProcessor做进一步处理
    m_clientChecker->e_clientConfirmed.reg(std::bind(&TcpConnectionManager::addPublicConnection, &m_conns, _1, _2));
    m_timer.regEventHandler(std::chrono::milliseconds(100), std::bind(&ClientConnectionChecker::timerExec, m_clientChecker, std::placeholders::_1));


    //cretae loginProcessor  //TODO
//    m_loginProcessor = LoginProcessor::create();
//    m_loginProcessor->e_clientConnectReady.reg(std::bind(&TcpConnectionManager::addPublicConnection, &m_conns, _1, _2));

    //客户端连接断开时的处理
    m_conns.e_afterErasePublicConn.reg(std::bind(&LoginProcessor::delClient, &LoginProcessor::me(), _1));
    m_timer.regEventHandler(std::chrono::milliseconds(100), std::bind(&LoginProcessor::timerExec, &LoginProcessor::me(), std::placeholders::_1));

    //注册消息处理事件和主定时器事件
    registerTcpMsgHandler();
    registerTimerHandler();
}

void Gateway::lanchThreads()
{
    Process::lanchThreads();
}

bool Gateway::sendToPrivate(ProcessIdentity pid, TcpMsgCode code)
{
    return sendToPrivate(pid, code, nullptr, 0);
}

bool Gateway::sendToPrivate(ProcessIdentity pid, TcpMsgCode code, const ProtoMsg& proto)
{
    return relayToPrivate(getId().value(), pid, code, proto);
}

bool Gateway::sendToPrivate(ProcessIdentity pid, TcpMsgCode code, const void* raw, uint32_t size)
{
    return relayToPrivate(getId().value(), pid, code, raw, size);
}

bool Gateway::relayToPrivate(uint64_t sourceId, ProcessIdentity pid, TcpMsgCode code, const ProtoMsg& proto)
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

    const ProcessIdentity routerId("router", 1);
    return m_conns.sendPacketToPrivate(routerId, packet);
}

bool Gateway::relayToPrivate(uint64_t sourceId, ProcessIdentity pid, TcpMsgCode code, const void* raw, uint32_t size)
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

    const ProcessIdentity routerId("router", 1);
    return m_conns.sendPacketToPrivate(routerId, packet);
}

bool Gateway::sendToClient(LoginId loginId, TcpMsgCode code, const void* raw, uint32_t size)
{
    const uint32_t bufSize = sizeof(TcpMsg) + size;
    uint8_t* buf = new uint8_t[bufSize];
    ON_EXIT_SCOPE_DO(delete[] buf);

    TcpMsg* msg = new(buf) TcpMsg(code);
    std::memcpy(msg->data, raw, size);

    TcpPacket::Ptr packet = TcpPacket::create();
    packet->setContent(buf, bufSize);

    return m_conns.sendPacketToPublic(loginId, packet);
}

bool Gateway::sendToClient(LoginId loginId, TcpMsgCode code, const ProtoMsg& proto)
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

    return m_conns.sendPacketToPublic(loginId, packet);
}

net::PacketConnection::Ptr Gateway::eraseClientConn(LoginId loginId)
{
    return m_conns.erasePublicConnection(loginId);
}

void Gateway::loadConfig()
{
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

