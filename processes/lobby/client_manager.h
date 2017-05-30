#ifndef LOBBY_CLIENT_MANAGER_HPP
#define LOBBY_CLIENT_MANAGER_HPP


#include "componet/class_helper.h"
#include "componet/spinlock.h"
#include "componet/fast_travel_unordered_map.h"

#include "base/process_id.h"

#include "protocol/protobuf/proto_manager.h"

namespace lobby{
using namespace water;
using namespace process;

struct Client
{
    TYPEDEF_PTR(Client)
    CREATE_FUN_NEW(Client)

    ClientConnectionId ccid = INVALID_CCID;
    ClientUniqueId cuid = INVALID_CUID;
    std::string openid;
    //TODO 更多的字段
};

class ClientManager
{
private:
    NON_COPYABLE(ClientManager)
    ClientManager() = default;
public:
    ~ClientManager() = default;

    void init();

    bool sendToClient(ClientConnectionId ccid, TcpMsgCode code, const ProtoMsg& proto);

    void timerExec();

    void regMsgHandler();

private:
    bool insert(Client::Ptr client);
    void erase(Client::Ptr client);
    Client::Ptr getByCuid(ClientUniqueId cuid);
    Client::Ptr getByOpenid(const std::string& openid);

private:
    Client::Ptr loadClientFromDB(const std::string& openid);
    bool saveClientToDB(Client::Ptr client);
    //分配uniqueId
    ClientUniqueId getClientUniqueId();
    void recoveryFromRedis();
//    void dealLogin();
private:
    void proto_LoginQuest(ProtoMsgPtr proto, ProcessId gatewayPid);

private:
    uint32_t m_uniqueIdCounter = 0;
    const std::string cuid2ClientsName = "openid2Clients"; //redis hashtable 用的名字
    componet::FastTravelUnorderedMap<std::string, Client::Ptr> m_openid2Clients; //此map进redis
    componet::FastTravelUnorderedMap<ClientUniqueId, Client::Ptr> m_cuid2Clients; //此map不进redis
//    componet::FastTravelUnorderedMap<ClientConnectionId, Client::Ptr> ccid2Clients;

private:
    static ClientManager* s_me;
public:
    static ClientManager& me();
};


}
#endif
