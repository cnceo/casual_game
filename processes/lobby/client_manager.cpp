#include "client_manager.h"
#include "client.h"
#include "lobby.h"
#include "redis_handler.h"
#include "game13.h"

#include "componet/logger.h"

#include "protocol/protobuf/public/client.codedef.h"
#include "protocol/protobuf/private/login.codedef.h"


namespace lobby{


static const char* MAX_CLIENT_UNIQUE_ID_NAME = "max_client_unique_id";

/***********************************************/

ClientManager* ClientManager::s_me = nullptr;

ClientManager& ClientManager::me()
{
    if(s_me == nullptr)
        s_me = new ClientManager();
    return *s_me;
}

////////////////////

void ClientManager::init()
{
    recoveryFromRedis();
}

void ClientManager::recoveryFromRedis()
{

    RedisHandler& redis = RedisHandler::me();
    std::string maxUinqueIdStr = redis.get(MAX_CLIENT_UNIQUE_ID_NAME);
    if (!maxUinqueIdStr.empty())
    {
        m_uniqueIdCounter = atoll(maxUinqueIdStr.c_str());
        LOG_TRACE("ClientManager启动, 初始化, 加载m_uniqueIdCounter={}", m_uniqueIdCounter);
    }
    else
    {
        LOG_TRACE("ClientManager启动, 初始化, 默认m_uniqueIdCounter={}", m_uniqueIdCounter);
    }

    return;


    auto exec = [this](const std::string openid, const std::string& bin) -> bool
    {
        auto client = Client::create();
        if (!client->deserialize(bin))
        {
            LOG_ERROR("recovery clients from reids, deserialize client failed, openid={}", openid);
            return true;
        }
        if (!insert(client))
        {
            LOG_ERROR("recovery clients from reids, insert client failed, openid={}, cuid={}, name={}",
                      client->openid(), client->cuid(), client->name());
        }
        return true;
    };
    redis.htraversal(CLIENT_TABLE_NAME, exec);
    LOG_TRACE("recovery clients data from redis");
}

void ClientManager::timerExec()
{
}

bool ClientManager::sendToClient(ClientConnectionId ccid, TcpMsgCode code, const ProtoMsg& proto)
{
    ProcessId gatewayPid("gateway", 1);
    return Lobby::me().relayToPrivate(ccid, gatewayPid, code, proto);
}

bool ClientManager::insert(Client::Ptr client)
{
    if(client == nullptr)
        return false;

    if (!m_openid2Clients.insert(client->openid(), client))
        return false;
    if (!m_cuid2Clients.insert(client->cuid(), client))
    {
        m_openid2Clients.erase(client->openid());
        return false;
    }
    if (!m_ccid2Clients.insert(client->ccid(), client))
    {
        m_openid2Clients.erase(client->openid());
        m_cuid2Clients.erase(client->cuid());
        return false;
    }
   return true;
}

void ClientManager::erase(Client::Ptr client)
{
    m_openid2Clients.erase(client->openid());
    m_cuid2Clients.erase(client->cuid());
    m_ccid2Clients.erase(client->ccid());
}

Client::Ptr ClientManager::getByCcid(ClientConnectionId ccid)
{
    auto iter = m_ccid2Clients.find(ccid);
    if (iter == m_ccid2Clients.end())
        return nullptr;
    return iter->second;
}

Client::Ptr ClientManager::getByCuid(ClientUniqueId cuid)
{
    auto iter = m_cuid2Clients.find(cuid);
    if (iter == m_cuid2Clients.end())
        return nullptr;
    return iter->second;
}

Client::Ptr ClientManager::getByOpenid(const std::string& openid)
{
    auto iter = m_openid2Clients.find(openid);
    if (iter == m_openid2Clients.end())
        return nullptr;
    return iter->second;
}

std::pair<Client::Ptr, bool> ClientManager::loadClient(const std::string& openid)
{
    RedisHandler& redis = RedisHandler::me();
    const std::string& bin = redis.hget(CLIENT_TABLE_NAME, openid);
    if (bin == "")
        return {nullptr, true};

    auto client = Client::create();
    if( !client->deserialize(bin) )
    {
        LOG_ERROR("load client from redis, deserialize failed, openid={}", openid);
        return {nullptr, false};
    }
    LOG_TRACE("load client, successed, openid={}, cuid={}, roomid={}", client->openid(), client->cuid(), client->roomid());
    return {client, true};
}

bool ClientManager::saveClient(Client::Ptr client)
{
    if (client == nullptr)
        return false;

    return client->saveToDB();
}

void ClientManager::proto_LoginQuest(ProtoMsgPtr proto, ProcessId gatewayPid)
{
    //login step2, 获取用户当前状态， 如果是新用户则需要加入数据库
    auto rcv = PROTO_PTR_CAST_PRIVATE(LoginQuest, proto);
    const ClientUniqueId ccid = rcv->ccid();
    const std::string openid = rcv->openid();
    LOG_TRACE("C_Login, ccid={}, openid={}", ccid, openid);


    PrivateProto::RetLoginQuest retMsg;
    TcpMsgCode retCode = PROTO_CODE_PRIVATE(RetLoginQuest);
    retMsg.set_ccid(ccid);
    retMsg.set_openid(openid);

    Client::Ptr client = getByOpenid(openid);
    if (client == nullptr) //不在线
    {
        const auto& loadRet = loadClient(openid);
        if (!loadRet.second)
        {
            retMsg.set_ret_code(PrivateProto::RLQ_REG_FAILED);
            Lobby::me().sendToPrivate(gatewayPid, retCode, retMsg);
            LOG_ERROR("login, step 2, new client load failed, openid={}", openid);
            return;
        }
        client = loadRet.first;
        if(client == nullptr) //新用户
        {
            client = Client::create();
            client->m_ccid = ccid;
            client->m_cuid = getClientUniqueId();
            client->m_openid = openid;
            client->m_name = rcv->name();

            bool insertRet = insert(client);
            bool saveRet = false;
            if (insertRet)
            {
                saveRet = saveClient(client);
                if (!saveRet)
                {
                    erase(client);
                    LOG_ERROR("login, step 2, new client, saveClient failed, openid={}", openid);
                }
            }
            if (!(insertRet && saveRet))
            {
                retMsg.set_ret_code(PrivateProto::RLQ_REG_FAILED);
                Lobby::me().sendToPrivate(gatewayPid, retCode, retMsg);
                LOG_TRACE("login, step 2, new client reg failed, ccid={}, cuid={}, openid={}", client->ccid(), client->cuid(), openid);
                return;
            }
            LOG_TRACE("login, step 2, new client reg successed, ccid={}, cuid={}, openid={}", client->ccid(), client->cuid(), openid);
        }
        else //登陆过但不在线
        {
            client->m_ccid = ccid;
            if (!insert(client))
            {
                retMsg.set_ret_code(PrivateProto::RLQ_REG_FAILED);
                Lobby::me().sendToPrivate(gatewayPid, retCode, retMsg);
                LOG_ERROR("login, step 2, old client online failed, ccid={}, cuid={}, openid={}", client->ccid(), client->cuid(), client->openid());
                return;
            }
            LOG_TRACE("login, step 2, old client online successed, ccid={}, cuid={}, openid={}", client->ccid(), client->cuid(), openid);
        }
    }
    else //已经在线了, 把当前在线的挤掉
    {
        client->noticeMessageBox("同一账号在另一台设备商登录, 你已下线");

        //TODO modify, 发送消息给gw，把老的挤下线
        PrivateProto::ClientBeReplaced cbr;
        TcpMsgCode cbrCode = PROTO_CODE_PRIVATE(ClientBeReplaced);
        cbr.set_cuid(client->cuid());
        cbr.set_ccid(client->ccid());
        cbr.set_openid(client->openid());
        Lobby::me().sendToPrivate(gatewayPid, cbrCode, cbr);
        LOG_TRACE("login, step 2.5, client was replaced, ccid={}, cuid={}, openid={}, newccid={}", client->ccid(), client->cuid(), client->openid(), ccid);

        //更新ccid 和 name
        erase(client);
        client->m_ccid = ccid;
        client->m_name = rcv->name();
        if (!insert(client))
        {
            //更新ccid失败, 登陆失败
            retMsg.set_ret_code(PrivateProto::RLQ_REG_FAILED);
            Lobby::me().sendToPrivate(gatewayPid, retCode, retMsg);
            LOG_TRACE("login, step 2, new client reg failed, ccid={}, cuid={}, openid={}", client->ccid(), client->cuid(), client->openid());
        }

    }

    //登陆成功
    retMsg.set_ret_code(PrivateProto::RLQ_SUCCES);
    retMsg.set_cuid(client->cuid());
    retMsg.set_openid(client->openid());
    Lobby::me().sendToPrivate(gatewayPid, retCode, retMsg);
    LOG_TRACE("login, step 2, 读取或注册client数据成功, ccid={}, cuid={}, openid={}", ccid, client->cuid(), client->openid());

    //更新可能的房间游戏信息
    client->syncBasicDataToClient();
    Room::clientOnline(client);
    return;
}

void ClientManager::proto_C_SendChat(const ProtoMsgPtr& proto, ClientConnectionId ccid)
{
    auto rcv = PROTO_PTR_CAST_PUBLIC(C_SendChat, proto);
    Client::Ptr client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    auto room = Room::get(client->roomid());
    if (room == nullptr)
        return;

    PROTO_VAR_PUBLIC(S_Chat, snd);
    snd.set_cuid(client->cuid());
    auto sndCtn = snd.mutable_content();
    sndCtn->set_type(rcv->type());
    switch (rcv->type())
    {
    case PublicProto::CHAT_TEXT:
        {
            sndCtn->set_data_text(rcv->data_text());
        }
        break;
    case PublicProto::CHAT_FACE:
    case PublicProto::CHAT_VOICE:
        {
            sndCtn->set_data_int(rcv->data_int());
            sndCtn->set_data_text(rcv->data_text());
        }
        break;
    }
    room->sendToOthers(client->cuid(), sndCode, snd);
}

void ClientManager::proto_C_G13_ReqGameHistoryCount(ClientConnectionId ccid)
{
    Client::Ptr client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    PROTO_VAR_PUBLIC(S_G13_GameHistoryCount, snd);
    snd.set_total(client->m_g13his.details.size());
    snd.set_win(client->m_g13his.win);
    snd.set_lose(client->m_g13his.lose);
    sendToClient(ccid, sndCode, snd);
}

void ClientManager::proto_C_G13_ReqGameHistoryDetial(const ProtoMsgPtr& proto, ClientConnectionId ccid)
{
    auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_ReqGameHistoryDetial, proto);
    Client::Ptr client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    const uint32_t page     = rcv->page();
    const uint32_t perPage  = 5;
    const uint32_t total    = client->m_g13his.details.size();

    uint32_t first = perPage * page;
    if (first != 0 && first >= total)
        first = perPage * (page - 1);

    PROTO_VAR_PUBLIC(S_G13_GameHistoryDetial, snd);
    for (const auto&detail : client->m_g13his.details)
    {
        auto item = snd.add_items();
        item->set_roomid(detail->roomid);
        item->set_rank(detail->rank);
        item->set_time(detail->time);

        for (const auto& opp : detail->opps)
        {
            auto sndOpp = item->add_opps();
            sndOpp->set_name(opp.name);
            sndOpp->set_rank(opp.rank);
        }
    }
    sendToClient(ccid, sndCode, snd);
}

ClientUniqueId ClientManager::getClientUniqueId()
{
    const ClientUniqueId nextCuid = m_uniqueIdCounter + 1;

    RedisHandler& redis = RedisHandler::me();
    if (!redis.set(MAX_CLIENT_UNIQUE_ID_NAME, componet::format("{}", nextCuid)))
        return INVALID_CUID;
    return ++m_uniqueIdCounter;
}

void ClientManager::regMsgHandler()
{
    using namespace std::placeholders;
    /************msg from client***********/
    REG_PROTO_PUBLIC(C_SendChat, std::bind(&ClientManager::proto_C_SendChat, this, _1, _2));
    REG_PROTO_PUBLIC(C_G13_ReqGameHistoryCount, std::bind(&ClientManager::proto_C_G13_ReqGameHistoryCount, this, _2));
    REG_PROTO_PUBLIC(C_G13_ReqGameHistoryDetial, std::bind(&ClientManager::proto_C_G13_ReqGameHistoryDetial, this, _1, _2));
    /************msg from cluster**********/
    REG_PROTO_PRIVATE(LoginQuest, std::bind(&ClientManager::proto_LoginQuest, this, _1, _2));
}

}

