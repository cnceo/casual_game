#ifndef LOBBY_CLIENT_HPP
#define LOBBY_CLIENT_HPP


#include "client_manager.h"

namespace lobby{
using namespace water;
using namespace process;

class Client
{
friend class ClientManager;
private:
    CREATE_FUN_NEW(Client)
    Client() = default;

public:
    TYPEDEF_PTR(Client)

    ClientConnectionId ccid() const;
    ClientUniqueId cuid() const;
    const std::string& openid() const;

    uint32_t roomId() const;
    void setRoomId(uint32_t roomId);
    void afterLeaveRoom();

    int32_t money() const;
    bool enoughMoney(int32_t money);
    int32_t addMoney(int32_t money);

    const std::string& name() const;

    bool sendToMe(TcpMsgCode code, const ProtoMsg& proto);
    bool noticeMessageBox(const std::string& text);
    template<typename ... Params>
    bool noticeMessageBox(const std::string& format, Params&&...);

private:
    ClientConnectionId m_ccid = INVALID_CCID;
    ClientUniqueId m_cuid = INVALID_CUID;
    std::string m_openid;
    std::string m_name;
    //TODO 更多的字段
    uint32_t m_roomId;

    int32_t m_money = 10000;
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

inline uint32_t Client::roomId() const
{
    return m_roomId;
}

inline void Client::setRoomId(uint32_t roomId)
{
    m_roomId = roomId;
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
    return m_money += money;
}

inline const std::string& Client::name() const
{
    return m_name;
}

template<typename... Params>
inline bool Client::noticeMessageBox(const std::string& format, Params&&... params)
{
    return noticeMessageBox(componet::format(format, std::forward<Params>(params)...));
}

}
#endif

