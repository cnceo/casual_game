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

    void recoveryFromRedis();

    void timerExec();

private:
    //分配uniqueId
    ClientUniqueId getClientUniqueId();

private:
    uint32_t m_uniqueIdCounter;
    const std::string cuid2ClientsName = "cuid2Clients";
    componet::FastTravelUnorderedMap<ClientUniqueId, Client::Ptr> cuid2Clients;

private:
    static ClientManager* s_me;
public:
    static ClientManager& getMe();
};


}
#endif
