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

#include "protocol/protobuf/proto_manager.h"

namespace gateway{

using namespace water;
using namespace process;


class ClientManager
{

    class Client
    {
    public:
        enum class State
        {
            logining,
            playing,
        };

        TYPEDEF_PTR(Client)
        CREATE_FUN_MAKE(Client)
    public:
        ClientConnectionId ccid = INVALID_CCID_VALUE;
        ClientUniqueId cuid = INVALID_CUID_VALUE;
        State state = State::logining;
    };

public:
    TYPEDEF_PTR(ClientManager)
    CREATE_FUN_MAKE(ClientManager)

    NON_COPYABLE(ClientManager)

    ClientManager(ProcessId processId);
    ~ClientManager() = default;

    //添加一个client connection, 返回分配给这个conn的clientId，失败返回INVALID_CLIENT_IDENDITY_VALUE
    ClientConnectionId clientOnline();
    void clientOffline(ClientConnectionId ccid);

    void regMsgHandler();
    void regClientMsgRelay();

private:
    Client::Ptr createNewClient();

private://消息处理
    void pub_C_Login(const ProtoMsgPtr& proto, ClientConnectionId connId);

private:
    const ProcessId m_processId;
    const uint32_t MAX_CLIENT_COUNTER = 0xffffff;
    uint32_t m_clientCounter = 0;

    componet::Spinlock m_clientsLock;
    componet::FastTravelUnorderedMap<ClientConnectionId, Client::Ptr> m_clients;
};

} //end namespace gateway


#endif
