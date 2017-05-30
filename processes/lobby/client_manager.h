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

    ClientConnectionId ccid = INVALID_CCID;
    ClientUniqueId cuid = INVALID_CUID;
    ProcessId roomId;
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
    void proto_C_Login(ProtoMsgPtr proto, ClientConnectionId ccid);

private:
    //分配uniqueId
    ClientUniqueId getClientUniqueId();
    void recoveryFromRedis();

private:
    uint32_t m_uniqueIdCounter = 0;
    const std::string cuid2ClientsName = "cuid2Clients";
    componet::FastTravelUnorderedMap<ClientUniqueId, Client::Ptr> cuid2Clients;

private:
    static ClientManager* s_me;
public:
    static ClientManager& me();
};


}
#endif
