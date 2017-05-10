/*
 * Author: LiZhaojia - waterlzj@gmail.com
 *
 * Last modified: 2017-05-10 16:31 +0800
 *
 * Description: 客户端管理器
 */

#ifndef PROCESS_GATEWAY_CLIENT_MANAGER_H
#define PROCESS_GATEWAY_CLIENT_MANAGER_H

#include "componet/class_helper.h"
#include "componet/spinlock.h"
#include "componet/fast_travel_unordered_map.h"

#include "base/process_id.h"

namespace gateway{

using namespace water;


class ClientManager
{
    struct Client
    {
        ClientId id;


    };

public:
    TYPEDEF_PTR(ClientManager)
    CREATE_FUN_MAKE(ClientManager)

    NON_COPYABLE(ClientManager)

    ClientManager(ProcessIdentity processId);
    ~ClientManager() = default;

    //添加一个client connection, 返回分配给这个conn的clientId，失败返回INVALID_CLIENT_IDENDITY_VALUE
    ClientIdentity clientOnline(net::PacketConnection::Ptr conn);


private:
    ClientIdentity getClientIdendity();
    void clientOffline(net::PacketConnection::Ptr conn);


private:
    const ProcessIdentity m_processId;
    uint32_t m_clientCounter = 0;
    componet::FastTravelUnorderedMap<ClientIdentity, Client> m_clients;
};

} //end namespace gateway


#endif
