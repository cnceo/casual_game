#include "anysdk_login_manager.h"

#include "gateway.h"

#include "componet/logger.h"

#include "net/http_packet.h"

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

void AnySdkLoginManager::dealHttpPackets(componet::TimePoint now)
{
    auto& conns = Gateway::me().httpConnectionManager();

    HttpConnectionManager::ConnectionHolder::Ptr conn;
    net::HttpPacket::Ptr packet;
    while(conns.getPacket(&conn, &packet))
    {
        if (conn->type == HttpConnectionManager::ConnType::client)
            clientVerifyRequest(conn->hcid, packet);
        else
            assVerifyResponse(packet);
    }
}

void AnySdkLoginManager::clientVerifyRequest(HttpConnectionId hcid, std::shared_ptr<net::HttpPacket> packet)
{
    LOG_DEBUG("http request");
    if (packet->msgType() != net::HttpMsg::Type::request)
        return;

}

void AnySdkLoginManager::assVerifyResponse(std::shared_ptr<net::HttpPacket> packet)
{
    LOG_DEBUG("http response");
    if (packet->msgType() != net::HttpMsg::Type::response)
        return;
}

void AnySdkLoginManager::onNewHttpConnection(net::BufferedConnection::Ptr conn)
{
    HttpConnectionId hcid = genHttpConnectionId();
    auto& conns = Gateway::me().httpConnectionManager();
    LOG_TRACE("anysdk http, new client http connection, remote={}", conn->getRemoteEndpoint());
    conns.addConnection(hcid, conn, HttpConnectionManager::ConnType::client);
}

HttpConnectionId AnySdkLoginManager::genHttpConnectionId() const
{
    return ++m_lastHcid;
}


void AnySdkLoginManager::startNameResolve()
{
    auto resolve = [this]()
    {
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
            this->m_assIp.lock.lock();
            this->m_assIp.ipStr = resolveRet.first;
            this->m_assIp.lock.unlock();
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
}


}
