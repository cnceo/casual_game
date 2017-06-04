#include "client_manager.h"

#include "gateway.h"

#include "componet/logger.h"
#include "componet/datetime.h"

#include "protocol/protobuf/proto_manager.h"
#include "protocol/protobuf/public/client.codedef.h"
#include "protocol/protobuf/private/login.codedef.h"

namespace gateway{

using namespace protocol;
using namespace protobuf;

using LockGuard = std::lock_guard<componet::Spinlock>;

ClientManager::ClientManager(ProcessId pid)
: m_pid(pid),
  m_clientCounter(1)
{
}

ClientManager::Client::Ptr ClientManager::createNewClient()
{
    auto client = Client::create();
    uint64_t now = componet::toUnixTime(componet::Clock::now());
    uint32_t processNum = m_pid.num();
    uint32_t counter = m_clientCounter.fetch_add(1);
    client->ccid = (now << 32) | (processNum << 24) | counter;
    return client;
}

bool ClientManager::insert(Client::Ptr client)
{
    if (client == nullptr)
        return false;
    LockGuard lock(m_clientsLock);
    return m_clients.insert(client->ccid, client);
}

void ClientManager::erase(Client::Ptr client)
{
    if (client == nullptr)
        return;
    LockGuard lock(m_clientsLock);
    m_clients.erase(client->ccid);
}

ClientManager::Client::Ptr ClientManager::getByCcid(ClientConnectionId ccid)
{
    LockGuard lock(m_clientsLock);
    auto iter = m_clients.find(ccid);
    if (iter == m_clients.end())
        return nullptr;
    return iter->second;
}

void ClientManager::eraseLater(Client::Ptr client)
{
    if (client == nullptr)
        return;
    erase(client);
    m_dyingClients.push_back(client);
}

void ClientManager::timerExec(const componet::TimePoint& now)
{
    { //action 1, 执行延迟删除
        for (auto client : m_dyingClients)
        {
            //client->conn->close(); //断开连接
        }
        m_dyingClients.clear();
    }
}

ClientConnectionId ClientManager::clientOnline()
{
    auto client = createNewClient();
    if (!insert(client))
    {
        LOG_ERROR("ClientManager::clientOnline failed, 生成的ccid出现重复, ccid={}", client->ccid);
        return INVALID_CCID;
    }

    LOG_TRACE("客户端接入，分配ClientConnectionId, ccid={}", client->ccid);
    return client->ccid;
}

void ClientManager::clientOffline(ClientConnectionId ccid)
{
    Client::Ptr client = getByCcid(ccid);
    if (client == nullptr)
    {
        LOG_TRACE("ClientManager, 客户端离线, ccid不存在, ccid={}", ccid);
        return;
    }

    if(client->state == Client::State::logining)
        LOG_TRACE("ClientManager, 客户端离线, 登录过程终止, ccid={}", ccid);
    else
        LOG_TRACE("ClientManager, 客户端离线, ccid={}", ccid);

    //destroy
    erase(client);
}

void ClientManager::kickOutClient(ClientConnectionId ccid, bool delay/* = true*/)
{
    Client::Ptr client = getByCcid(ccid);

    if (client == nullptr)
    {
        LOG_TRACE("ClientManager, 踢下线, ccid不存在, ccid={}, delay={}", ccid);
        return;
    }

    if(client->state == Client::State::logining)
        LOG_TRACE("ClientManager, 踢下线, 登录过程终止, ccid={}, delay={}", ccid);
    else
        LOG_TRACE("ClientManager, 踢下线, ccid={}, delay={}", ccid);
    //TODO 这里要加入到客户端的通知， 告知为何踢出, 次函数需要修改加入一个踢出原因参数
    delay ? eraseLater(client) : erase(client);
    return;
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

void ClientManager::proto_C_Login(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto rcv = PROTO_PTR_CAST_PUBLIC(C_Login, proto);


    auto client = getByCcid(ccid);
    if (client == nullptr)
    {
        LOG_DEBUG("login, step 1, 处理 C_Login, client 已经离线, ccid={}, openid={}", ccid, rcv->openid());
        return;
    }

    //TODO login step1, 依据登陆类型和登陆信息验证登陆有效性, 微信的
    bool isLegal = true;

    if (isLegal)
    {
        PrivateProto::LoginQuest snd;
        snd.set_openid(rcv->openid());
        snd.set_ccid(ccid);
        snd.set_name(rcv->nick_name());
        Gateway::me().sendToPrivate(ProcessId("lobby", 1), PROTO_CODE_PRIVATE(LoginQuest), snd);
        LOG_TRACE("login, step 1, openid&token 验证通过, ccid={}, openid={}, token={}", ccid, rcv->openid(), rcv->token());
    }
    else //暂时不用处理
    {

    }
}

void ClientManager::proto_RetLoginQuest(ProtoMsgPtr proto)
{
    auto rcv = PROTO_PTR_CAST_PRIVATE(RetLoginQuest, proto);
    auto client = getByCcid(rcv->ccid());
    if (client == nullptr)
    {
        LOG_TRACE("login, step 3, lobby 确认通过, client 已离线, ccid={}, openid={}, cuid={}", rcv->ccid(), rcv->openid(), rcv->cuid());
        return;
    }

    client->cuid = rcv->cuid();
    client->state = Client::State::playing;

    PublicProto::S_LoginRet snd;
    int32_t rc = rcv->ret_code();
    if (rc == PrivateProto::RLQ_SUCCES)
    {
        snd.set_ret_code(PublicProto::LOGINR_SUCCES);
        snd.set_cuid(client->cuid);
        snd.set_temp_token("xxxx"); //此版本一律返回xxxx， 此字段暂不启用
    }
    else
    {
        snd.set_ret_code(PublicProto::LOGINR_FAILED);
        //在定时器中延迟删除, 保证最后一条消息能够有机会发出
        eraseLater(client);
    }
    LOG_TRACE("login, step 3, 登陆成功, ccid={}, openid={}, cuid={}", rcv->ccid(), rcv->openid(), rcv->cuid());
    Gateway::me().sendToClient(client->ccid, PROTO_CODE_PUBLIC(S_LoginRet), snd);
}

void ClientManager::proto_ClientBeReplaced(ProtoMsgPtr proto)
{
    auto rcv = PROTO_PTR_CAST_PRIVATE(ClientBeReplaced, proto);
    auto client = getByCcid(rcv->ccid());
    if (client == nullptr)
    {
        LOG_TRACE("login, step 2.5, 客户端被挤下线, 已经不在线了, ccid={}, openid={}, cuid={}", rcv->ccid(), rcv->openid(), rcv->cuid());
        return;
    }
    
    kickOutClient(rcv->ccid());
    LOG_TRACE("login, step 2.5, 客户端被挤下线, ccid={}, openid={}, cuid={}", rcv->ccid(), rcv->openid(), rcv->cuid());
}

void ClientManager::regMsgHandler()
{
    using namespace std::placeholders;
    /**********************msg from client*************/
    REG_PROTO_PUBLIC(C_Login, std::bind(&ClientManager::proto_C_Login, this, _1, _2));

    /*********************msg from cluster*************/
    REG_PROTO_PRIVATE(RetLoginQuest, std::bind(&ClientManager::proto_RetLoginQuest, this, _1));
    REG_PROTO_PRIVATE(ClientBeReplaced, std::bind(&ClientManager::proto_ClientBeReplaced, this, _1));
}

}

