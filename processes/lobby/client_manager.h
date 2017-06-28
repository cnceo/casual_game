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

class Client;

class ClientManager
{
private:
    using ClientPtr = std::shared_ptr<Client>;
    NON_COPYABLE(ClientManager)
    ClientManager() = default;
public:
    ~ClientManager() = default;

    void init();

    ClientPtr getByCuid(ClientUniqueId cuid);
    ClientPtr getByCcid(ClientConnectionId ccid);
    ClientPtr getByOpenid(const std::string& openid);

    bool sendToClient(ClientConnectionId ccid, TcpMsgCode code, const ProtoMsg& proto);

    void timerExec();

    void regMsgHandler();

    bool saveClient(ClientPtr client);
    bool saveClient(const Client& client);
private:
    bool insert(ClientPtr client);
    void erase(ClientPtr client);

private:
    std::pair<ClientPtr, bool> loadClient(const std::string& openid);
    //分配uniqueId
    ClientUniqueId getClientUniqueId();
    void recoveryFromRedis();
//    void dealLogin();
private:
    void proto_C_SendChat(const ProtoMsgPtr& proto, ClientConnectionId ccid);
    void proto_C_G13_ReqGameHistoryCount(ClientConnectionId ccid);
    void proto_C_G13_ReqGameHistoryDetial(const ProtoMsgPtr& proto, ClientConnectionId ccid);
private:
    void proto_LoginQuest(ProtoMsgPtr proto, ProcessId gatewayPid);

private:
    uint32_t m_uniqueIdCounter = 100000;
    const std::string cuid2ClientsName = "openid2Clients"; //redis hashtable 用的名字
    componet::FastTravelUnorderedMap<std::string, ClientPtr> m_openid2Clients; //此map进redis
    componet::FastTravelUnorderedMap<ClientUniqueId, ClientPtr> m_cuid2Clients; //此map不进redis
    componet::FastTravelUnorderedMap<ClientConnectionId, ClientPtr> m_ccid2Clients;

private:
    static ClientManager* s_me;
public:
    static ClientManager& me();
};


}
#endif
