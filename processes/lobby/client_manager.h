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

class Client
{
friend class ClientManager;
private:
    CREATE_FUN_NEW(Client)
    Client() = default;

public:
    TYPEDEF_PTR(Client)

    bool sendToMe(TcpMsgCode code, const ProtoMsg& proto);

private:
    ClientConnectionId ccid = INVALID_CCID;
    ClientUniqueId cuid = INVALID_CUID;
    std::string openid;
    std::string name;
    //TODO 更多的字段
    uint32_t roomId;
};

class ClientManager
{
private:
    NON_COPYABLE(ClientManager)
    ClientManager() = default;
public:
    ~ClientManager() = default;

    void init();

    Client::Ptr getByCuid(ClientUniqueId cuid);
    Client::Ptr getByOpenid(const std::string& openid);

    bool sendToClient(ClientConnectionId ccid, TcpMsgCode code, const ProtoMsg& proto);

    void timerExec();

    void regMsgHandler();

private:
    bool insert(Client::Ptr client);
    void erase(Client::Ptr client);

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
