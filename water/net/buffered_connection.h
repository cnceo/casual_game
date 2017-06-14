/*
 * Author: LiZhaojia - waterlzj@gmail.com
 *
 * Last modified: 2017-06-14 11:03 +0800
 *
 * Description: 
 */


#ifndef WATER_NET_BUFFERED_CONNECTION
#define WATER_NET_BUFFERED_CONNECTION

#include "connection.h"
#include "net_exception.h"


namespace water{ 
namespace net{

class ConnectionBuffer
{
public:
    ConnectionBuffer(uint32_t sizeLimit = 4096);

    std::pair<uint8_t*, uint32_t> readable();
    bool commitRead(uint32_t size);

    //参数指期望的写入量, 0为
    std::pair<uint8_t*, uint32_t> writeable(uint32_t size = 0);
    bool commitWrite(uint32_t size);

    bool empty() const;

private:
    std::vector<uint8_t> m_buf;
    uint32_t m_dataBegin = 0;
    uint32_t m_dataEnd = 0;
//    uint32_t m_sizeLimit;
};

//异常定义, 读取已关闭的socket
DEFINE_EXCEPTION(ReadClosedConnection, net::NetException);

class BufferedConnection : public TcpConnection
{
public:
    TYPEDEF_PTR(BufferedConnection)
    CREATE_FUN_MAKE(BufferedConnection)
    explicit BufferedConnection(TcpConnection&& tcpConn);

public:
    //直接访问缓冲区
    ConnectionBuffer& sendBuf();
    ConnectionBuffer& recvBuf();

    //将发送缓冲写入socket，返回true表示缓冲区已经发空
    bool trySend();
    //读取socket数据放入接收缓冲，返回true表示接收缓冲非空
    bool tryRecv();
private:
    ConnectionBuffer m_sendBuf;
    ConnectionBuffer m_recvBuf;
};

inline ConnectionBuffer& BufferedConnection::sendBuf()
{
    return m_sendBuf;
}

inline ConnectionBuffer& BufferedConnection::recvBuf()
{
    return m_recvBuf;
}

inline bool ConnectionBuffer::empty() const
{
    return m_dataBegin == m_dataEnd;
}


}}


#endif
