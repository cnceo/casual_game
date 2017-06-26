#include "anysdk_login_manager.h"

#include "gateway.h"

#include "componet/logger.h"
#include "componet/scope_guard.h"

#include "net/http_packet.h"

#include "coroutine/coroutine.h"

#include <sys/socket.h>  
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 


//dns解析代码 //很丑陋, 先将就吧
std::pair<std::string, bool> resolveHost(const std::string& hostname )   
{  
    struct addrinfo *answer, hint, *curr;  
    char ipstr[16];                                                                                                          
    bzero(&hint, sizeof(hint));  
    //hint.ai_family = AF_UNSPEC;
    hint.ai_family = AF_INET;  
    hint.ai_socktype = SOCK_STREAM;  

    int retc = getaddrinfo(hostname.c_str(), NULL, &hint, &answer);  
    if (retc != 0)  
        return {gai_strerror(retc), false};  

    std::string ret;
    for (curr = answer; curr != NULL; curr = curr->ai_next) 
    {   
        inet_ntop(AF_INET, &(((struct sockaddr_in *)(curr->ai_addr))->sin_addr), ipstr, 16);  
        ret = ipstr;
        break; //拿到一个立刻返回
    }   
    freeaddrinfo(answer);  
    return {ret, true};
}


namespace gateway{

AnySdkLoginManager AnySdkLoginManager::s_me;

AnySdkLoginManager& AnySdkLoginManager::me()
{
    return s_me;
}

//////////////////////////////////////////////////////////////////////////
AnySdkLoginManager::AnySdkLoginManager()
{
}

void AnySdkLoginManager::onNewHttpConnection(net::BufferedConnection::Ptr conn)
{
    HttpConnectionId hcid = genCliHttpConnectionId();
    LOG_TRACE("ASS, new client http connection, hcid={}, remote={}", hcid, conn->getRemoteEndpoint());
    auto& conns = Gateway::me().httpConnectionManager();
    if (!conns.addConnection(hcid, conn, HttpConnectionManager::ConnType::client))
    {
        LOG_ERROR("ASS, insert cli conn to tcpConnManager failed, hcid={}", hcid);
        return;
    }

    //create a new client
    auto client = AllClients::AnySdkClient::create();
    client->assip = &m_assip;
    client->clihcid = hcid;
    {
        std::lock_guard<componet::Spinlock> lock(m_allClients.newClentsLock);
        if (m_allClients.newClients.insert({hcid, client}))
        {
            LOG_ERROR("ASS, insert client to corotCliets failed, hcid={}", client->clihcid);
            conns.eraseConnection(hcid);
            return;
        }
    }
}

void AnySdkLoginManager::dealHttpPackets(componet::TimePoint now)
{
    auto& conns = Gateway::me().httpConnectionManager();

    HttpConnectionManager::ConnectionHolder::Ptr conn;
    net::HttpPacket::Ptr packet;
    while(conns.getPacket(&conn, &packet))
    {
        const HttpConnectionId clihcid = isCliHcid(conn->hcid) ? conn->hcid : conn->hcid - 1;
        const HttpConnectionId asshcid = clihcid + 1;
        auto iter = m_allClients.corotClients.find(clihcid);
        if (iter == m_allClients.corotClients.end())
        {
            LOG_DEBUG("ASS, deal packet, client is gone, conn->hcid={}", conn->hcid);
            conns.eraseConnection(clihcid);
            conns.eraseConnection(clihcid + 1);
            continue;
        }

        auto client = iter->second;
        if (conn->type == HttpConnectionManager::ConnType::client)
        {
            if (client->status != AllClients::AnySdkClient::Status::recvCliReq)
            {
                LOG_ERROR("ASS, deal packet, recvd cli packet under status={}, conn->hcid={}", client->status, conn->hcid);
                conns.eraseConnection(clihcid);
                conns.eraseConnection(clihcid + 1);
                continue;
            }

            client->cliReq = packet;
            client->status = AllClients::AnySdkClient::Status::connToAss;
            LOG_DEBUG("ASS, deal packet, recved cli packet, status={}, conn->hcid={}", client->status, conn->hcid);
        }
        else
        {
            if (client->status != AllClients::AnySdkClient::Status::recvAssRsp)
            {
                LOG_ERROR("ASS, deal packet, recvd ass packet under status={}, conn->hcid={}", client->status, conn->hcid);
                conns.eraseConnection(clihcid);
                conns.eraseConnection(clihcid + 1);
                continue;
            }
            client->assRsp = packet;
            client->status = AllClients::AnySdkClient::Status::rspToCli;
            conns.eraseConnection(asshcid);
            LOG_DEBUG("ASS, deal packet, recved ass packet, status={}, conn->hcid={}", client->status, conn->hcid);
        }
    }
}

void AnySdkLoginManager::afterClientDisconnect(HttpConnectionId hcid)
{
    std::lock_guard<componet::Spinlock> lock(m_closedHcidsLock);
    m_closedHcids.push_back(hcid);
}

bool AnySdkLoginManager::isAssHcid(HttpConnectionId hcid)
{
    return hcid % 2 == 0;
}

bool AnySdkLoginManager::isCliHcid(HttpConnectionId hcid)
{
    return hcid % 2 != 0;
}

HttpConnectionId AnySdkLoginManager::genCliHttpConnectionId() const
{
    return m_lastHcid += 2;
}


void AnySdkLoginManager::startNameResolve()
{
    auto resolve = [this]()
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        while (true)
        {
            const auto& resolveRet = resolveHost("oauth.anysdk.com");
            if (!resolveRet.second)
            {
                LOG_TRACE("resolve host oauth.anysdk.com failed, will retry in 5 seconds ...");
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }

//            LOG_TRACE("resolve host oauth.anysdk.com ok, ip={}, will expire in 60 seconds ...", resolveRet.first);
            this->m_assip.lock.lock();
            this->m_assip.ipstr = resolveRet.first;
            this->m_assip.lock.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }
    };

    std::thread resolver(resolve);
    resolver.detach(); //自由运行
}


bool AnySdkLoginManager::checkAccessToken(std::string openid, std::string token) const
{
    auto iter = m_tokens.find(openid);
    if (iter == m_tokens.end())
        return false;
    return iter->second->token == token;
}

void AnySdkLoginManager::timerExec(componet::TimePoint now)
{
    //删除过期tokens
    for (auto iter = m_tokens.begin(); iter != m_tokens.end(); )
    {
        if (iter->second->expiryTime <= now)
        {
            iter = m_tokens.erase(iter);
            continue;
        }
        ++iter;
    }

    {//启动新进连接的处理协程
        std::lock_guard<componet::Spinlock> lock(m_allClients.newClentsLock);
        for (auto iter = m_allClients.newClients.begin(); iter != m_allClients.newClients.end(); )
        {
            auto client = iter->second;
            iter = m_allClients.newClients.erase(iter);
            
            if (!m_allClients.corotClients.insert({client->clihcid, client}).second)
            {
                LOG_ERROR("ASS, move client to corotClients failed, hcid={}", client->clihcid);
                Gateway::me().httpConnectionManager().eraseConnection(client->clihcid);
                continue;
            }

            //为这个连接启一个协程
            corot::create(std::bind(&AllClients::AnySdkClient::corotExec, client));
            LOG_TRACE("ASS, CREATE CROTO successed, hcid={}", client->clihcid);
        }
    }

    {//处理连接断开事件
        std::list<HttpConnectionId> closedHcids;
        {
            std::lock_guard<componet::Spinlock> lock(m_closedHcidsLock);
            closedHcids.swap(m_closedHcids);
        }
        std::lock_guard<componet::Spinlock> lock(m_allClients.newClentsLock);
        for (auto hcid : closedHcids)
        {
            HttpConnectionId clihcid = isCliHcid(hcid) ? hcid : hcid - 1;
            auto iter = m_allClients.corotClients.find(clihcid);
            if (iter == m_allClients.corotClients.end())
                continue;

            auto client = iter->second;

            //所有处理均已结束, 无所谓了
            if ( client->status >= AllClients::AnySdkClient::Status::done)
                continue;

            //client 断开, 且相关逻辑并未处理完成
            if ( (isCliHcid(hcid) && (client->status <= AllClients::AnySdkClient::Status::rspToCli)) )
            {
                client->status = AllClients::AnySdkClient::Status::abort;
                continue;
            }

            //ass 断开, 且相关逻辑并未处理完成
            if(isAssHcid(hcid) && (client->status <= AllClients::AnySdkClient::Status::recvAssRsp))
            {
                client->status = AllClients::AnySdkClient::Status::assAbort;
                continue;
            }
        }
    }

    {//销毁失效的client
        for (auto iter = m_allClients.corotClients.begin(); iter != m_allClients.corotClients.end(); )
        {
            if (iter->second->status == AllClients::AnySdkClient::Status::destroy)
            {
                iter = m_allClients.corotClients.erase(iter);
                continue;
            }
            ++iter;
        }
    }
}

void AnySdkLoginManager::AllClients::AnySdkClient::corotExec()
{
    HttpConnectionId asshcid = clihcid + 1;
    auto& conns = Gateway::me().httpConnectionManager();
    ON_EXIT_SCOPE_DO(LOG_TRACE("ASS, CROTO EXIT successed, hcid={}", clihcid));
    while (true)
    {
        switch (status)
        {
        case Status::recvCliReq:
            {
                corot::this_corot::yield();
                break;
            }
        case Status::connToAss:
            {
                //conn to ass
                std::string ipstr;
                net::Endpoint ep;
                {
                    std::lock_guard<componet::Spinlock> lock(assip->lock);
                    ipstr = assip->ipstr;
                }
                ep.ip.fromString(ipstr);
                ep.port = 80;
                auto conn = net::TcpConnection::create(ep);
                try
                {
                    while(!conn->tryConnect())
                        corot::this_corot::yield();
                }
                catch (const net::NetException& ex)
                {
                    LOG_ERROR("ASS, conn to ass failed, hcid={}, ipstr={}", asshcid, ipstr);
                    status = Status::assAbort;
                    break;
                }

                //reg conn
                auto assconn = net::BufferedConnection::create(std::move(*conn));
                if (!conns.addConnection(asshcid, assconn, HttpConnectionManager::ConnType::server))
                {
                    LOG_ERROR("ASS, insert ass conn to tcpConnManager failed, hcid={}", clihcid);
                    return;
                }
                status = Status::reqToAss;
                LOG_TRACE("ASS, conn to ass successed, hcid={}, ipstr={}", asshcid, ipstr);
            }
        case Status::reqToAss:
            {
                if (!conns.sendPacket(asshcid, cliReq))
                {
                    corot::this_corot::yield();
                    break;
                }
                LOG_TRACE("ASS, send request to ass, hcid={}", clihcid);
                status = Status::recvAssRsp;
            }
        case Status::recvAssRsp:
            {
                corot::this_corot::yield();
                break;
            }
        case Status::rspToCli:
            {
                if (!conns.sendPacket(clihcid, assRsp))
                {
                    corot::this_corot::yield();
                    break;
                }
                LOG_TRACE("ASS, send response to cli, hcid={}", clihcid);
                status = Status::done;
            }
        case Status::done:
            {
                LOG_TRACE("ASS, done, destroy later,  hcid={}", clihcid);
                conns.eraseConnection(clihcid);
            }
            return;
        case Status::assAbort:
            {
                //TODO 发送一个http403给cli, 然后关闭
                conns.eraseConnection(clihcid);
                LOG_TRACE("ASS, assAbort, destroy later, hcid={}", clihcid);
            }
            return;
        case Status::abort:
            {
                conns.eraseConnection(asshcid);
                LOG_TRACE("ASS, abort, destroy later, hcid={}", clihcid);
            }
            return;
        default:
            return;
        }
    }
}

}
