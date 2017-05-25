#include "client_manager.h"

#include "componet/logger.h"
#include "componet/datetime.h"

#include "protocol/protobuf/proto_manager.h"
#include "protocol/protobuf/public/login.codedef.h"

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
        LOG_ERROR("ClientManager, 客户端离线, ccid不存在, clientSessionId={}", ccid);
        return;
    }

    if(client->state == Client::State::logining)
        LOG_TRACE("ClientManager, 客户端离线, 登录过程终止, clientSessionId={}", ccid);
    else
        LOG_TRACE("ClientManager, 客户端离线, clientSessionId={}", ccid);
}

void ClientManager::pub_C_Login(const ProtoMsgPtr& proto, ClientConnectionId connId)
{
}

void ClientManager::regMsgHandler()
{
    using namespace std::placeholders;
    REG_PROTO_PUBLIC(C_Login, std::bind(&ClientManager::pub_C_Login, this, _1, _2));
}

}

