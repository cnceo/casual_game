#include "client_manager.h"

#include "lobby.h"
#include "redis_handler.h"

#include "componet/logger.h"

#include "protocol/protobuf/public/client.codedef.h"


namespace lobby{


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
    std::vector<std::string> redisCmd = {"info"};
}

void ClientManager::timerExec()
{
}

bool ClientManager::sendToClient(ClientConnectionId ccid, TcpMsgCode code, const ProtoMsg& proto)
{
    ProcessId gatewayId("gateway", 1);
    return Lobby::me().relayToPrivate(ccid, gatewayId, code, proto);
}

void ClientManager::proto_C_Login(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto msg = PROTO_PTR_CAST_PUBLIC(C_Login, proto);
    LOG_TRACE("login msg, {}, {}", msg->login_type(), msg->wechat_openid());

    ClientUniqueId cuid = getClientUniqueId();

    PublicProto::S_LoginRet ret;
    ret.set_ret_code(1);
    ret.set_unique_id(cuid);
    ret.set_temp_token("7041");
    sendToClient(ccid, PROTO_CODE_PUBLIC(S_LoginRet), ret);
    return;
}

ClientUniqueId ClientManager::getClientUniqueId()
{
    return ++m_uniqueIdCounter;
}

void ClientManager::regMsgHandler()
{
    using namespace std::placeholders;
    REG_PROTO_PUBLIC(C_Login, std::bind(&ClientManager::proto_C_Login, this, _1, _2));
}

}
