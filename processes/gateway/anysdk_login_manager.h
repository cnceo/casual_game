/*
 * Author: LiZhaojia - waterlzj@gmail.com
 *
 * Last modified: 2017-06-18 04:16 +0800
 *
 * Description: 
 */


/*
 * 1, 换掉http_conn_manager中的conn索引的数据类型, 把socketFD换成hcid
 * 2, 普通http_server 用于listen
 * 3, 直接用buffered_connecton进行连接请求, 可以考虑封装一个http_client来做这件事儿
 */


#ifndef GATEWAY_ANYSDK_LOGIN_MANAGER_H
#define GATEWAY_ANYSDK_LOGIN_MANAGER_H

#include "net/buffered_connection.h"

#include "componet/fast_travel_unordered_map.h"
#include "componet/spinlock.h"
#include "componet/datetime.h"

#include "base/process_id.h"

#include <atomic>


namespace water{ namespace net{ class HttpPacket; } }

namespace gateway{

using namespace water;
using namespace process;

class AnySdkLoginManager
{
public:
    AnySdkLoginManager() = default;
    ~AnySdkLoginManager() = default;


    //新的http连接呼入
    void onNewHttpConnection(net::BufferedConnection::Ptr conn);

    //启一个线程, 定时的做anysdk server的域名解析, 
    //做法很山寨, getAddrInfo_a这个函数基本不能用, 对着RFC标准自己写异步太折腾, 就先这样子吧
    void startNameResolve();
    
    //登陆令牌验证
    bool checkAccessToken(std::string openid, std::string token) const;

    //删除过期的token
    void timerExec(componet::TimePoint now);

    //包处理循环 
    void dealHttpPackets(componet::TimePoint now);

private:
    //得到一个新的hcid
    HttpConnectionId genHttpConnectionId() const;

    //随机一个访问Token, 时间戳+随机32bits整数, 放在一起搞成hex格式
    std::string genAccessToken();

    //处理client的包
    void clientVerifyRequest(HttpConnectionId hcid, std::shared_ptr<net::HttpPacket> packet);
    //处理ass server的包
    void assVerifyResponse(std::shared_ptr<net::HttpPacket> packet);

private:
    struct TokenInfo
    {
        TYPEDEF_PTR(TokenInfo)

        std::string openid;
        std::string token;
        componet::TimePoint expiryTime;
    };
    componet::FastTravelUnorderedMap<std::string, TokenInfo::Ptr> m_tokens;

    std::unordered_map<HttpConnectionId, std::string> m_openids;

    mutable HttpConnectionId m_lastHcid = 0;
    net::BufferedConnection::Ptr m_connToAss;


    struct
    {
        componet::Spinlock lock;
        std::string ipStr; //AnySdkServer的ip
    } m_assIp;

private:
    static AnySdkLoginManager s_me;
public:
    static AnySdkLoginManager& me();
};


}//end namespace gateway

#endif
