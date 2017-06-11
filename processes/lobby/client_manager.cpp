#include "client_manager.h"
#include "client.h"
#include "lobby.h"
#include "redis_handler.h"
#include "game13.h"

#include "componet/logger.h"

#include "protocol/protobuf/public/client.codedef.h"
#include "protocol/protobuf/private/login.codedef.h"


namespace lobby{

void Client::afterLeaveRoom()
{
    m_roomId = 0;
    PROTO_VAR_PUBLIC(S_G13_PlayerQuited, snd);
    sendToMe(sndCode, snd);
}

bool Client::sendToMe(TcpMsgCode code, const ProtoMsg& proto)
{
    ProcessId gatewayPid("gateway", 1);
    return Lobby::me().relayToPrivate(ccid(), gatewayPid, code, proto);
}

bool Client::noticeMessageBox(const std::string& text)
{
    PROTO_VAR_PUBLIC(S_Notice, snd);
//    PublicProto::S_Notice snd;
    snd.set_type(PublicProto::S_Notice::MSG_BOX);
    snd.set_text(text);
    return sendToMe(sndCode, snd);
//    sendToMe(PROTO_CODE_PUBLIC(S_Notice), snd);
}

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
//    std::vector<std::string> redisCmd = {"info"};
    LOG_DEBUG("recovery clients data from redis");
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

Client::Ptr ClientManager::loadClientFromDB(const std::string& openId)
{
    return nullptr; //TODO 加入真实的数据库访问
}

bool ClientManager::saveClientToDB(Client::Ptr client)
{
    return true;  //TODO 加入真实的数据库访问
}

void ClientManager::proto_LoginQuest(ProtoMsgPtr proto, ProcessId gatewayPid)
{
    //login step2, 获取用户当前状态， 如果是新用户则需要加入数据库
    auto rcv = PROTO_PTR_CAST_PRIVATE(LoginQuest, proto);
    const ClientUniqueId ccid = rcv->ccid();
    LOG_TRACE("C_Login, ccid={}, openid={}", ccid, rcv->openid());


    PrivateProto::RetLoginQuest ret;
    TcpMsgCode retCode = PROTO_CODE_PRIVATE(RetLoginQuest);
    ret.set_ccid(ccid);

    Client::Ptr client = getByOpenid(rcv->openid());
    if (client == nullptr) //不在线
    {
        client = loadClientFromDB(rcv->openid());
        if(client == nullptr) //新用户
        {
            client = Client::create();
            client->m_ccid = ccid;
            client->m_cuid = getClientUniqueId();
            client->m_openid = rcv->openid();
            client->m_name = rcv->name();

            do {
                if (insert(client))
                {
                    if(saveClientToDB(client))
                        break;
                }
                else
                {
                    erase(client); //失败要重新删掉
                }
                //注册失败, 登陆失败
                ret.set_cuid(client->cuid());
                ret.set_ret_code(PrivateProto::RLQ_REG_FAILED);
                Lobby::me().sendToPrivate(gatewayPid, retCode, ret);
                LOG_TRACE("login, step 2, new client reg failed, ccid={}, cuid={}, openid={}", client->ccid(), client->cuid(), client->openid());
                return;
            } while(false);
        }
        else //登陆过但不在线
        {
            client->m_ccid = ccid;
        }
    }
    else //已经在线了, 把当前在线的挤掉
    {
        //TODO modify, 发送消息给gw，把老的挤下线
        PrivateProto::ClientBeReplaced cbr;
        TcpMsgCode cbrCode = PROTO_CODE_PRIVATE(ClientBeReplaced);
        cbr.set_cuid(client->cuid());
        cbr.set_ccid(client->ccid());
        cbr.set_openid(client->openid());
        Lobby::me().sendToPrivate(gatewayPid, cbrCode, cbr);

        //更新ccid
        erase(client);
        client->m_ccid = ccid;
        if (!insert(client))
        {
            //更新ccid失败, 登陆失败
            ret.set_cuid(client->cuid());
            ret.set_ret_code(PrivateProto::RLQ_REG_FAILED);
            Lobby::me().sendToPrivate(gatewayPid, retCode, ret);
            LOG_TRACE("login, step 2, new client reg failed, ccid={}, cuid={}, openid={}", client->ccid(), client->cuid(), client->openid());
        }

    }

    //登陆成功
    ret.set_ret_code(PrivateProto::RLQ_SUCCES);
    ret.set_cuid(client->cuid());
    ret.set_openid(client->openid());
    Lobby::me().sendToPrivate(gatewayPid, retCode, ret);
    LOG_TRACE("login, step 2, 读取或注册client数据成功, ccid={}, cuid={}, openid={}", ccid, client->cuid(), client->openid());

    //更新游戏信息
    Room::clientOnline(client);
    return;
}

void ClientManager::proto_C_SendChat(const ProtoMsgPtr& proto, ClientConnectionId ccid)
{
    auto rcv = PROTO_PTR_CAST_PUBLIC(C_SendChat, proto);
    Client::Ptr client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    auto room = Room::get(client->roomId());
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
        }
        break;
    }
    room->sendToOthers(client->cuid(), sndCode, snd);
}


ClientUniqueId ClientManager::getClientUniqueId()
{
    //TODO uniqueid的组成和生成规则
    return ++m_uniqueIdCounter;
}

void ClientManager::regMsgHandler()
{
    using namespace std::placeholders;
    /************msg from client***********/
    REG_PROTO_PUBLIC(C_SendChat, std::bind(&ClientManager::proto_C_SendChat, this, _1, _2));
    /************msg from cluster**********/
    REG_PROTO_PRIVATE(LoginQuest, std::bind(&ClientManager::proto_LoginQuest, this, _1, _2));
}

}
