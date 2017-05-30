#include "client_manager.h"

#include "gateway.h"

#include "componet/logger.h"
#include "componet/datetime.h"

#include "protocol/protobuf/proto_manager.h"
#include "protocol/protobuf/public/client.codedef.h"

namespace gateway{

using LockGuard = std::lock_guard<componet::Spinlock>;

ClientManager::ClientManager(ProcessId processId)
: m_processId(processId)
{
}

ClientManager::Client::Ptr ClientManager::createNewClient()
{
    auto client = Client::create();
    uint64_t now = componet::toUnixTime(componet::Clock::now());
    uint32_t processNum = m_processId.num();
    client->ccid = (now << 32) | (processNum << 24) | (++m_clientCounter);
    return client;
}

ClientConnectionId ClientManager::clientOnline()
{
    auto client = createNewClient();
    if (!m_clients.insert({client->ccid, client}))
    {
        LOG_ERROR("ClientManager::clientOnline failed, 生成的ccid出现重复, ccid={}", client->ccid);
        return INVALID_CCID;
    }

    LOG_TRACE("客户端接入，分配clientId为 {}", client->ccid);
    return client->ccid;
}

void ClientManager::clientOffline(ClientConnectionId ccid)
{
    Client::Ptr client(nullptr);

    {
        LockGuard lock(m_clientsLock);

        auto it = m_clients.find(ccid);
        if (it != m_clients.end())
        {
            client = it->second;
            m_clients.erase(it);
        }
    }

    if (client == nullptr)
    {
        LOG_TRACE("ClientManager, 客户端离线, ccid不存在, ccid={}", ccid);
        return;
    }

    if(client->state == Client::State::logining)
        LOG_TRACE("ClientManager, 客户端离线, 登录过程终止, ccid={}", ccid);
    else
        LOG_TRACE("ClientManager, 客户端离线, ccid={}", ccid);
}

void ClientManager::KickOutClient(ClientConnectionId ccid)
{
    Client::Ptr client(nullptr);

    {
        LockGuard lock(m_clientsLock);

        auto it = m_clients.find(ccid);
        if (it != m_clients.end())
        {
            client = it->second;
            m_clients.erase(it);
        }
    }
    if (client == nullptr)
    {
        LOG_TRACE("ClientManager, 踢下线, ccid不存在, ccid={}", ccid);
        return;
    }

    if(client->state == Client::State::logining)
        LOG_TRACE("ClientManager, 踢下线, 登录过程终止, ccid={}", ccid);
    else
        LOG_TRACE("ClientManager, 踢下线, ccid={}", ccid);

}

void ClientManager::relayClientMsgToServer(const ProcessId& pid, TcpMsgCode code, const ProtoMsgPtr& protoPtr, ClientConnectionId ccid)
{
    LOG_DEBUG("relay client msg to process {}, code={}", pid, code);
    Gateway::me().relayToPrivate(ccid, pid, code, *protoPtr);
}

void ClientManager::relayClientMsgToClient(TcpMsgCode code, const ProtoMsgPtr& protoPtr, ClientConnectionId ccid)
{
    LOG_DEBUG("relay client msg to client {}, code={}", ccid, code);
    Gateway::me().sendToClient(ccid, code, *protoPtr);
}

void ClientManager::regMsgHandler()
{
    using namespace std::placeholders;
    /**********************msg from client*************/
//    REG_PROTO_PUBLIC(C_Login, std::bind(&ClientManager::pub_C_Login, this, _1, _2, _3));

    /*********************msg from cluster*************/
//    ProtoManager::me().regHandler(PROTO_CODE_PUBLIC(C_Login), std::bind(&ClientManager::relayClientMsg, this, ProcessId("lobby", 1),PROTO_CODE_PUBLIC(C_Login),  _1, _2));
}

}

