#include "login_processor.h"

#include "water/componet/logger.h"

#include "gateway.h"
#include "role_manager.h"
#include "protocol/rawmsg/public/login.codedef.public.h"
#include "protocol/rawmsg/public/login.h"

#include "protocol/rawmsg/private/login.codedef.private.h"
#include "protocol/rawmsg/private/login.h"

#include "protocol/rawmsg/rawmsg_manager.h"

namespace gateway{

using namespace water;
using namespace process;

/*
LoginProcessor& LoginProcessor::me()
{
    static LoginProcessor me;
    return me;
}
*/

LoginProcessor::LoginProcessor()
: m_loginCounter(0)
{
}

LoginId LoginProcessor::getLoginId()
{
    LoginId ret = ++m_loginCounter;
    return (ret << 16u) + Gateway::me().getId().value();
}

void LoginProcessor::newClient(LoginId loginId, const std::string& account)
{
    auto client = ClientInfo::Ptr(new ClientInfo);
    client->loginId = loginId;
    client->account = account;

    LockGuard lock(m_clientsLock);
    m_clients.insert({loginId, client});
}

void LoginProcessor::clientConnReady(LoginId loginId)
{
    ClientInfo::Ptr client = getClientByLoginId(loginId);
    if(client == nullptr)
        return;

    PrivateRaw::QuestRoleList send;
    send.loginId = client->loginId;
    client->account.copy(send.account, sizeof(send.account));
    send.account[sizeof(send.account) - 1] = 0;

    ProcessIdentity receiver("dbcached", 1); 
    Gateway::me().sendToPrivate(receiver, RAWMSG_CODE_PRIVATE(QuestRoleList), &send, sizeof(send));
    LOG_DEBUG("登录, 向db请求角色列表, account={}", client->account);
}

void LoginProcessor::delClient(LoginId loginId)
{
    LockGuard lock(m_disconnectedClientsLock);
    //在定时器中延迟删除
    m_disconnectedClients.push_back(loginId);
    LOG_DEBUG("登录, 下线, loginId={}", loginId);
}

//此函数订阅主定时器，在主定时线程中执行
void LoginProcessor::timerExec(const componet::TimePoint& now)
{
    //删掉已经断开的clients
    m_disconnectedClientsLock.lock();
    std::list<LoginId> temp = m_disconnectedClients;
    m_disconnectedClients.clear();
    m_disconnectedClientsLock.unlock();

    LockGuard lock(m_clientsLock);
    for(auto loginId : temp)
    {
        auto it = m_clients.find(loginId);
        if(it == m_clients.end()) //已经不在了, 正常的话, 这个角色已经登陆完成, 在rolemanager中了
        {
            RoleManager::me().onClientDisconnect(loginId);
            continue;
        }
        m_clients.erase(loginId);
    }
}

LoginProcessor::ClientInfo::Ptr LoginProcessor::getClientByLoginId(LoginId loginId)
{
    LockGuard lock(m_clientsLock);
    auto it = m_clients.find(loginId);
    if(it == m_clients.end())
        return nullptr;
    return it->second;
}

void LoginProcessor::regMsgHandler()
{
    using namespace std::placeholders;
    /*
    REG_RAWMSG_PUBLIC(SelectRole, std::bind(&LoginProcessor::clientmsg_SelectRole, this, _1, _2, _3));
    REG_RAWMSG_PUBLIC(CreateRole, std::bind(&LoginProcessor::clientmsg_CreateRole, this, _1, _2, _3));
    REG_RAWMSG_PUBLIC(GetRandName, std::bind(&LoginProcessor::clientmsg_GetRandName, this, _1, _2, _3));

    REG_RAWMSG_PRIVATE(RetRoleList, std::bind(&LoginProcessor::servermsg_RetRoleList, this, _1, _2));
    REG_RAWMSG_PRIVATE(RetCreateRole, std::bind(&LoginProcessor::servermsg_RetCreateRole, this, _1, _2));//, _3, _4));
    REG_RAWMSG_PRIVATE(RetRandName, std::bind(&LoginProcessor::servermsg_RetRandName, this, _1, _2));
    */
}

}
