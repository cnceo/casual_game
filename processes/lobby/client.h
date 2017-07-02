#ifndef LOBBY_CLIENT_HPP
#define LOBBY_CLIENT_HPP


#include "client_manager.h"

#include <list>

namespace lobby{
using namespace water;
using namespace process;

extern const char* CLIENT_TABLE_NAME;

struct G13His
{
    struct Detail
    {
        TYPEDEF_PTR(Detail)
        CREATE_FUN_MAKE(Detail)

        struct Opponent
        {
            std::string name;
            int32_t rank = 0;
        };
        uint32_t roomid = 0;
        int32_t  rank = 0;
        int32_t  time = 0;
        std::vector<Opponent> opps;
    };
    std::list<Detail::Ptr> details;
    int32_t win  = 0;
    int32_t lose = 0;
};

class Client
{
friend class ClientManager;
friend class MysqlHandler;
private:
    CREATE_FUN_NEW(Client)
    Client() = default;

public:
    TYPEDEF_PTR(Client)

    ClientConnectionId ccid() const;
    ClientUniqueId cuid() const;
    const std::string& openid() const;

    uint32_t roomid() const;
    void setRoomId(uint32_t roomid);
    void afterLeaveRoom(G13His::Detail::Ptr detail = nullptr);

    int32_t money() const;
    bool enoughMoney(int32_t money);
    int32_t addMoney(int32_t money);

    int32_t money1() const;
    bool enoughMoney1(int32_t money1);
    int32_t addMoney1(int32_t money1);

    const std::string& name() const;

    const std::string& imgurl() const;

    const std::string& ipstr() const;

    bool sendToMe(TcpMsgCode code, const ProtoMsg& proto) const;
    bool noticeMessageBox(const std::string& text);
    template<typename ... Params>
    bool noticeMessageBox(const std::string& format, Params&&...);

    std::string serialize() const;
    bool deserialize(const std::string& bin);

    bool saveToDB() const;

    void syncBasicDataToClient() const;

private:
    ClientConnectionId m_ccid = INVALID_CCID;
    ClientUniqueId m_cuid = INVALID_CUID;
    std::string m_openid;
    std::string m_name;
    //TODO 更多的字段
    uint32_t m_roomid;

    int32_t m_money  = 100000;
    int32_t m_money1 = 1000;

    G13His m_g13his;

    std::string m_token;
    std::string m_imgurl;
    std::string m_ipstr;
};


inline ClientConnectionId Client::ccid() const
{
    return m_ccid;
}

inline ClientUniqueId Client::cuid() const
{
    return m_cuid;
}
inline const std::string& Client::openid() const
{
    return m_openid;
}

inline uint32_t Client::roomid() const
{
    return m_roomid;
}

inline void Client::setRoomId(uint32_t roomid)
{
    if (roomid == m_roomid)
        return;
    m_roomid = roomid;
    saveToDB();
}

inline int32_t Client::money() const
{
    return m_money;
}

inline bool Client::enoughMoney(int32_t money)
{
    return m_money > money;
}

inline int32_t Client::addMoney(int32_t money)
{
    if (money == 0)
        return money;

    m_money += money;
    saveToDB();
    syncBasicDataToClient();
    return m_money;
}

inline const std::string& Client::name() const
{
    return m_name;
}

inline int32_t Client::money1() const
{
    return m_money1;
}

inline bool Client::enoughMoney1(int32_t money1)
{
    return m_money1 > money1;
}

inline int32_t Client::addMoney1(int32_t money1)
{
    if (money1 == 0)
        return money1;

    m_money1 += money1;
    saveToDB();
    return m_money1;
}

inline const std::string& Client::imgurl() const
{
    return m_imgurl;
}

inline const std::string& Client::ipstr() const
{
    return m_ipstr;
}

template<typename... Params>
inline bool Client::noticeMessageBox(const std::string& format, Params&&... params)
{
    return noticeMessageBox(componet::format(format, std::forward<Params>(params)...));
}

}
#endif

