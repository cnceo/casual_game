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

namespace gateway{


class AnySdkLoginManager
{
public:
    AnySdkLoginManager();
    ~AnySdkLoginManager();
    HttpConnectionId getHttpConnectionId() const;

    //启一个线程, 定时的做anysdk server的域名解析, 
    //做法很山寨, getAddrInfo_a这个函数基本不能用, 对着RFC标准自己写异步太折腾, 就先这样子吧
    void startNameResolve();
    
    //登陆令牌验证
    bool checkAccessToken(std::string openid, std::string token) const;

    //处理客户端的验证请求
    clientVerifyRequestHandler()

    void timerExec(componet::TimePoint now);

private:
    //随机一个访问Token, 时间戳+随机32bits整数, 放在一起搞成hex格式
    std::string genAccessToken();

private:
    struct ClientInfo
    {
        TYPEDEF_PTR(ClientInfo)

        std::string openid;
        std::string accessToken;
        componet::TimePoint expriyTime;
    };
    componet::FastTravelUnorderedMap<std::string, ClientInfo::Ptr> m_accesTokens;

    mutable HttpConnectionId m_lastHcid = 0;
    net::BufferedConnection m_connToAss;
    std::atomic<std::string> m_assIp; //AnySdkServer的ip

private:
    static AnySdkLoginManager s_me;
public:
    static AnySdkLoginManager& me();
};


}//end namespace gateway

#endif
