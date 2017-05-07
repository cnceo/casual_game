/*
 * Author: LiZhaojia
 *
 * Created: 2015-03-14 16:04 +0800
 *
 * Modified: 2015-03-14 16:04 +0800
 *
 * Description:  客户端登录处理, 处理登录流程
 */

#ifndef PROCESS_GATEWAY_LOGIN_PROCESSER_H
#define PROCESS_GATEWAY_LOGIN_PROCESSER_H

//#include "def.h"

#include "water/componet/spinlock.h"
#include "water/componet/datetime.h"
#include "water/componet/event.h"
#include "base/process_thread.h"
#include "base/connection_checker.h"

#include <list>

namespace gateway{


using namespace water;

class LoginProcessor
{
    struct ClientInfo
    {
        TYPEDEF_PTR(ClientInfo)

        LoginId loginId;
        std::string account;

        std::map<RoleId, std::string> roleList;
    };
    using LockGuard = std::lock_guard<componet::Spinlock>;

public:
    TYPEDEF_PTR(LoginProcessor)
    CREATE_FUN_MAKE(LoginProcessor)

    NON_COPYABLE(LoginProcessor)

    LoginProcessor();
    ~LoginProcessor() = default;

    void newClient(LoginId loginId, const std::string& account);
    void clientConnReady(LoginId loginId);

    void delClient(LoginId loginId);

    void timerExec(const componet::TimePoint& now);
    void regMsgHandler();

public:
    componet::Event<void (water::net::PacketConnection::Ptr, LoginId)> e_clientConnectReady;

private:
    LoginId getLoginId();


private:
    ClientInfo::Ptr getClientByLoginId(LoginId loginId);

    //处理client消息

    //处理db的消息

private:
    LoginId m_loginCounter;

    componet::Spinlock m_disconnectedClientsLock;
    std::list<LoginId> m_disconnectedClients;

    componet::Spinlock m_clientsLock;
    std::map<LoginId, ClientInfo::Ptr> m_clients; //<loginId, clientInfo>, 消息驱动
/*
public:
    static LoginProcessor& me();
    */
};

}


#endif
