#include "http_connection_manager.h"

#include "net/packet.h"
#include "net/http_packet.h"

#include "componet/other_tools.h"
#include "componet/logger.h"
#include "net/buffered_connection.h"


#include <iostream>
#include <mutex>

namespace water{
namespace process{


HttpConnectionManager::HttpConnectionManager()
{
}

void HttpConnectionManager::addConnection(net::BufferedConnection::Ptr conn, ConnType type)
{
    conn->setNonBlocking();

    auto connHolder = std::make_shared<ConnectionHolder>();
    connHolder->id = uniqueID++;
    connHolder->conn = conn;

    std::lock_guard<componet::Spinlock> lock(m_lock);

    auto insertAllRet = m_allConns.insert({conn->getFD(), connHolder});
    if(insertAllRet.second == false)
    {
        LOG_ERROR("ConnectionManager, insert httpConn to m_allConns failed, remoteEp={}", 
                  conn->getRemoteEndpoint());
    }

    try
    {
        net::Epoller::Event epollEvent = conn->trySend() ? net::Epoller::Event::read : net::Epoller::Event::read_write;
        m_epoller.regSocket(conn->getFD(), epollEvent);

        auto recvPacketType = connHolder->type == ConnType::client ? net::HttpMsg::Type::request : net::HttpMsg::Type::response;
        connHolder->recvPacket = net::HttpPacket::create(recvPacketType);
        LOG_DEBUG("add http conn to epoll, remote ep={}", conn->getRemoteEndpoint());
    }
    catch (const componet::ExceptionBase& ex)
    {
        LOG_ERROR("ConnectionManager, insert httpConn to epoller failed, ex={}, remoteEp={}",
                  ex.what(), conn->getRemoteEndpoint());
        m_allConns.erase(insertAllRet.first);
    }
}

void HttpConnectionManager::delConnection(net::BufferedConnection::Ptr conn)
{
    std::lock_guard<componet::Spinlock> lock(m_lock);

    auto it = m_allConns.find(conn->getFD());
    if(it == m_allConns.end())
        return;

    m_allConns.erase(it);
    m_epoller.delSocket(conn->getFD());
}

bool HttpConnectionManager::exec()
{
    try
    {
        if(!checkSwitch())
            return true;

        //绑定epoll消息处理器
        using namespace std::placeholders;
        m_epoller.setEventHandler(std::bind(&HttpConnectionManager::epollerEventHandler, this, _2, _3));

        while(checkSwitch())
        {
            m_epoller.wait(std::chrono::milliseconds(5)); //5 milliseconds 一轮
        }
    }
    catch (const net::NetException& ex)
    {
        LOG_ERROR("connManager 异常退出 , {}", ex.what());
        return false;
    }

    return true;
}

void HttpConnectionManager::epollerEventHandler(int32_t socketFD, net::Epoller::Event event)
{
    ConnectionHolder::Ptr connHolder;
    {
        std::lock_guard<componet::Spinlock> lock(m_lock);
        auto connsIter = m_allConns.find(socketFD);
        if(connsIter == m_allConns.end())
        {
            LOG_ERROR("epoll报告一个socket事件，但该socket不在manager中");
            return;
        }
        connHolder = connsIter->second;
    }
    net::BufferedConnection::Ptr conn = connHolder->conn;
    try
    {
        switch (event)
        {
        case net::Epoller::Event::read:
            {
                if (m_recvQueue.full()) //队列满了, 不收了
                    break;
                net::HttpPacket::Ptr& packet = connHolder->recvPacket;
                while(conn->tryRecv())
                {
                    const auto& rawBuf = conn->recvBuf().readable();
                    size_t parsedSize = packet->parse(rawBuf.first, rawBuf.second); //从上次没解析的地方开始解析
                    conn->recvBuf().commitRead(parsedSize); //已解析部分从recvBuf中取出
                    if (!packet->complete()) //还没收到一个完整的包, 停止本次处理
                        break;
                    m_recvQueue.push({connHolder, packet});   //收到的包入队
                    packet = net::HttpPacket::create(packet->msgType()); //重置待收包
                }
            }
            break;
        case net::Epoller::Event::write:
            {
                std::lock_guard<componet::Spinlock> lock(connHolder->sendLock);
                while (conn->trySend())
                {
                    if (connHolder->sendQueue.empty())
                    {
                        m_epoller.modifySocket(socketFD, net::Epoller::Event::read);
                        break;
                    }

                    while (!connHolder->sendQueue.empty()) 
                    {
                        net::Packet::Ptr packet = connHolder->sendQueue.front();
                        const auto& rawBuf = conn->sendBuf().writeable(packet->size());
                        if (rawBuf.second == 0)
                            break;

                        const auto copySize = packet->copy(rawBuf.first, rawBuf.second);
                        conn->sendBuf().commitWrite(copySize);
                        if (copySize < packet->size())
                        {
                            packet->pop(copySize);
                            break;
                        }
                        connHolder->sendQueue.pop_front();
                    }
                }
            }
            break;
        case net::Epoller::Event::error:
            {
                LOG_ERROR("epoll error, {}", conn->getRemoteEndpoint());
                delConnection(conn);
            }
            break;
        default:
            LOG_ERROR("epoll, unexcept event type, {}", conn->getRemoteEndpoint());
            break;
        }
    }
    catch (const net::ReadClosedConnection& ex)
    {
        LOG_TRACE("对方断开连接, {}", conn->getRemoteEndpoint());
        delConnection(conn);
    }
    catch (const net::NetException& ex)
    {
        LOG_TRACE("连接异常, {}", conn->getRemoteEndpoint());
        delConnection(conn);
    }
}

bool HttpConnectionManager::sendPacket(ConnectionHolder::Ptr connHolder, net::Packet::Ptr packet)
{
    std::lock_guard<componet::Spinlock> lock(connHolder->sendLock);

    if (!connHolder->sendQueue.empty())
    {
        if (connHolder->sendQueue.size() > 512)
            return false;

        connHolder->sendQueue.push_back(packet);
        return true;
    }

    net::BufferedConnection::Ptr conn = connHolder->conn;
    const auto& rawBuf = conn->sendBuf().writeable(packet->size());
    if (rawBuf.second >= packet->size())
    {
        packet->copy(rawBuf.first, rawBuf.second);
        conn->sendBuf().commitWrite(packet->size());
        if (conn->trySend())
            return true;
    }
    
    auto copySize = packet->copy(rawBuf.first, rawBuf.second);
    conn->sendBuf().commitWrite(copySize);
    packet->pop(copySize);

    connHolder->sendQueue.push_back(packet);
    LOG_TRACE("发送过慢，发送队列出现排队, connId:{}", connHolder->id);
    m_epoller.modifySocket(connHolder->conn->getFD(), net::Epoller::Event::read_write);
    return true;
}


}}
