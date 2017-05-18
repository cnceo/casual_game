#ifndef LOBBY_CLIENT_MANAGER_HPP
#define LOBBY_CLIENT_MANAGER_HPP


#include "componet/class_helper.h"
#include "componet/spinlock.h"
#include "componet/fast_travel_unordered_map.h"

#include "base/process_id.h"

#include "protocol/protobuf/proto_manager.h"

namespace lobby{
using namespace water;

class Client
{
};

class ClientManager : 
{
public:


    ClientManager() = default();
    ~ClientManager() = default();

private:
    componet::FastTravelUnorderedMap<ClientConnectionId, Client::Ptr>
    componet::FastTravelUnorderedMap<ClientUniqueId, Client::Ptr>
};


}
#endif
