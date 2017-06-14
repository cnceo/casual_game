#include "tcp_connection_manager.h"

#include "process.h"
#include "tcp_message.h"

#include "componet/other_tools.h"
#include "componet/logger.h"

#include <iostream>
#include <mutex>

namespace water{
namespace process{


void TcpConnectionManager::addPrivateConnection(net::BufferedConnection::Ptr conn, 
                                                   ProcessId processId)
{
    conn->setNonBlocking();

    auto item = std::make_shared<ConnectionHolder>();
    item->type = ConnType::privateType;
    item->id = processId.value();
    item->conn = conn;

    std::lock_guard<componet::Spinlock> lock(m_lock);

    auto insertAllRet = m_allConns.insert({conn->getFD(), item});
    if(insertAllRet.second == false)
    {
        LOG_ERROR("ConnectionManager, insert privateConn to m_allConns failed, processId={}", processId);
        return;
    }

    auto& privateProcessByType = m_privateConns[processId.type()];

    auto insertPrivateRet = privateProcessByType.insert({processId.num(), item});
    if(insertPrivateRet.second == false)
    {
        LOG_ERROR("ConnectionManager, insert privateConn to m_privateConns failed, processId={}", processId);
        m_allConns.erase(insertAllRet.first);
        return;
    }

    try
    {
        m_epoller.regSocket(conn->getFD(), net::Epoller::Event::read);
    }
    catch (const componet::ExceptionBase& ex)
    {
        LOG_ERROR("ConnectionManager, insert privateConn to epoller failed, ex={}, processId={}",
                  ex.what(), processId);
        m_allConns.erase(insertAllRet.first);
        privateProcessByType.erase(insertPrivateRet.first);
    }

    //添加privateConn完成的事件
    e_afterAddPrivateConn(processId);
    LOG_TRACE("ConnectionManager, insert privateConn, fd={}, pid={}, ep={}", 
              conn->getFD(), processId, conn->getRemoteEndpoint());
}

bool TcpConnectionManager::addPublicConnection(net::BufferedConnection::Ptr conn, ClientSessionId clientId)
{
    if (conn == nullptr)
        return false;

    conn->setNonBlocking();
    auto item = std::make_shared<ConnectionHolder>();
    item->type = ConnType::publicType;
    item->id = clientId;
    item->conn = conn;

    std::lock_guard<componet::Spinlock> lock(m_lock);

    auto insertAllRet = m_allConns.insert({conn->getFD(), item});
    if(insertAllRet.second == false)
    {
        LOG_ERROR("ConnectionManager::addPublicConnection failed, clientId={}, ep={}", clientId, conn->getRemoteEndpoint());
        return false;
    }
   
    auto insertPublicRet = m_publicConns.insert({clientId, item});
    if(insertPublicRet.second == false)
    {
        LOG_ERROR("ConnectionManager::addPublicConnection failed, clientId={}, ep={}", clientId, conn->getRemoteEndpoint());
        m_allConns.erase(insertAllRet.first);
        return false;
    }
    
    try
    {
        m_epoller.regSocket(conn->getFD(), net::Epoller::Event::read);
    }
    catch (const componet::ExceptionBase& ex)
    {
        LOG_ERROR("ConnectionManager::addPublicConnection failed, {}, clientId={}, ep={}", 
                  ex, clientId, conn->getRemoteEndpoint());

        m_allConns.erase(insertAllRet.first);
        m_publicConns.erase(insertPublicRet.first);
        return false;
    }
    LOG_TRACE("ConnectionManager, insert publicConn, fd={}, id={}, ep={}", conn->getFD(), clientId, conn->getRemoteEndpoint());
    return true;
}

net::BufferedConnection::Ptr TcpConnectionManager::erasePublicConnection(ClientSessionId clientId)
{
    net::BufferedConnection::Ptr ret;

    {//局部锁, 否则调用eraseConnection会死锁
        std::lock_guard<componet::Spinlock> lock(m_lock);
        auto publicIt = m_publicConns.find(clientId);
        if(publicIt == m_publicConns.end())
            return nullptr;
        ret = publicIt->second->conn;
    }

    eraseConnection(ret);
    return ret;
}

void TcpConnectionManager::eraseConnection(net::BufferedConnection::Ptr conn)
{
    std::lock_guard<componet::Spinlock> lock(m_lock);

    auto it = m_allConns.find(conn->getFD());
    if(it == m_allConns.end())
        return;

    auto publicIt = m_publicConns.find(it->second->id);
    if(publicIt != m_publicConns.end())
    {
        LOG_TRACE("ConnectionManager, erase publicConn, id={}", it->second->id);
        m_publicConns.erase(publicIt);
        e_afterErasePublicConn(it->second->id);
    }
    else
    {
        ProcessId pid(it->second->id);
        auto iter = m_privateConns.find(pid.type());
        if(iter != m_privateConns.end() && iter->second.find(pid.num()) != iter->second.end())
        {
            LOG_TRACE("ConnectionManager, erase privateConn, id={}, fd={}", pid, conn->getFD());
            iter->second.erase(pid.num());
            e_afterErasePrivateConn(pid);
        }
        else
        {
            LOG_ERROR("ConnectionManager, erase conn, notPublic and notPrivate id={}", pid);
        }
    }
    LOG_TRACE("ConnectionManager, erase conn, fd={}, id={}, ep={}", conn->getFD(), it->second->id, conn->getRemoteEndpoint());
    m_epoller.delSocket(conn->getFD());
    m_allConns.erase(it);
}

bool TcpConnectionManager::exec()
{
    try
    {
        if(!checkSwitch())
            return true;

        //绑定epoll消息处理器
        using namespace std::placeholders;
        m_epoller.setEventHandler(std::bind(&TcpConnectionManager::epollerEventHandler, this, _2, _3));

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

void TcpConnectionManager::epollerEventHandler(int32_t socketFD, net::Epoller::Event event)
{
    ConnectionHolder::Ptr connHolder;
    {
        std::lock_guard<componet::Spinlock> lock(m_lock);
        auto connsIter = m_allConns.find(socketFD);
        if(connsIter == m_allConns.end()) 
        {
            if(net::Epoller::Event::error != event)
            {
                LOG_ERROR("epoll报告一个socket事件，但该socket不在manager中, fd={}, event={}", socketFD, event);
            }
            m_epoller.delSocket(socketFD);
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
                while(conn->tryRecv())
                {
                    const auto& rawBuf = conn->recvBuf().readable();
                    auto packet = net::TcpPacket::tryParse(rawBuf.first, rawBuf.second);
                    if (packet == nullptr)
                    {
                        if (conn->recvBuf().full()) //缓冲区空间不够收一个包
                        {
                            if (conn->recvBuf().size() >= 1 * 1024 * 1024)
                            {
                                LOG_TRACE("recv tcp packet, 端消息超长, 踢掉, remote={}", conn->getRemoteEndpoint());
                                eraseConnection(conn);
                                conn->close();
                                return;
                            }
                            
                            conn->recvBuf().resize(conn->recvBuf().size() + 1024);//一次增加1k
                            LOG_TRACE("recv tcp packet, 长消息, remote={}, recved size=", conn->getRemoteEndpoint(), conn->recvBuf().size());
                            continue;
                        }
                        break;
                    }
                    if (!m_recvQueue.push({connHolder, packet}) )
                        break;//队列满，停止处理
                    conn->recvBuf().commitRead(packet->size());
                }
            }
            break;
        case net::Epoller::Event::write:
            {
                std::lock_guard<componet::Spinlock> lock(connHolder->sendLock);
                while(connHolder->conn->trySend()) //缓冲区已经发空
                {
                    if(connHolder->sendQueue.empty()) //累积未发送的消息队列也已经发空
                    {
                        m_epoller.modifySocket(socketFD, net::Epoller::Event::read);
                        break;
                    }

                    while (!connHolder->sendQueue.empty())
                    {
                        net::Packet::Ptr packet = connHolder->sendQueue.front();
                        const auto& rawBuf = conn->recvBuf().writeable(packet->size());
                        if (rawBuf.second < packet->size())
                            break;

                        packet->copy(rawBuf.first, rawBuf.second);
                        conn->recvBuf().commitWrite(packet->size());
                        connHolder->sendQueue.pop_front();
                    }
                }
            }
            break;
        case net::Epoller::Event::error:
            {
                LOG_ERROR("epoll error, {}", conn->getRemoteEndpoint());
                eraseConnection(conn);
                conn->close();
            }
            break;
        default:
            LOG_ERROR("epoll, unexcept event type, {}", conn->getRemoteEndpoint());
            break;
        }
    }
    catch (const net::ReadClosedConnection& ex)
    {
        LOG_TRACE("对方断开连接, {}, fd={}", conn->getRemoteEndpoint(), conn->getFD());
        eraseConnection(conn);
        conn->close();
    }
    catch (const net::NetException& ex)
    {
        LOG_TRACE("连接异常, {}, {}", conn->getRemoteEndpoint(), ex.what());
        eraseConnection(conn);
        conn->close();
    }
}

bool TcpConnectionManager::getPacket(ConnectionHolder::Ptr* conn, net::Packet::Ptr* packet)
{
    std::pair<ConnectionHolder::Ptr, net::Packet::Ptr> ret;
    if(!m_recvQueue.pop(&ret))
        return false;

    *conn = ret.first;
    *packet = ret.second;
    return true;
}

bool TcpConnectionManager::sendPacketToPrivate(ProcessId processId, net::Packet::Ptr packet)
{
    std::lock_guard<componet::Spinlock> lock(m_lock);
    auto typeIter = m_privateConns.find(processId.type());
    if(typeIter == m_privateConns.end())
    {
        LOG_ERROR("send private packet, 目标进程未连接, processId={}", processId);
        return false;
    }

    auto numIter = typeIter->second.find(processId.num());
    if(numIter == typeIter->second.end())
    {
        LOG_ERROR("send private packet, 目标进程未连接, processId={}", processId);
        return false;
    }

    try
    {
        sendPacket(numIter->second, packet);
    }
    catch (const componet::ExceptionBase& ex)
    {
        LOG_ERROR("send private packet, socket异常, {}, remoteProcessId={}", ex.what(), processId);
        return false;
    }

    return true;
}

void TcpConnectionManager::broadcastPacketToPrivate(ProcessType processType, net::Packet::Ptr packet)
{
    std::lock_guard<componet::Spinlock> lock(m_lock);
    auto iter = m_privateConns.find(processType);
    if(iter == m_privateConns.end())
    {
        LOG_ERROR("broadcast private packet, 目标进程类型未连接, processType={}",
                  processType);
        return;
    }

    for(auto& item : iter->second)
    {
        try
        {
            sendPacket(item.second, packet);
        }
        catch (const componet::ExceptionBase& ex)
        {
            LOG_ERROR("broadcast private packet, socket异常, {}, remoteProcessId={}", 
                      ex.what(), ProcessId(item.second->id));
        }
    }
}

bool TcpConnectionManager::sendPacketToPublic(ClientSessionId clientId, net::Packet::Ptr packet)
{
    std::lock_guard<componet::Spinlock> lock(m_lock);
    auto it = m_publicConns.find(clientId);
    if(it == m_publicConns.end())
    {
        LOG_ERROR("send packet to public, socket not found, clientId={}", clientId);
        return false;
    }

    try
    {
        sendPacket(it->second, packet);
    }
    catch (const componet::ExceptionBase& ex)
    {
        LOG_ERROR("send private packet, socket异常, {}, id={}", ex.what(), clientId);
        return false;
    }
    return true;
}

void TcpConnectionManager::broadcastPacketToPublic(net::Packet::Ptr packet)
{
    std::lock_guard<componet::Spinlock> lock(m_lock);
    for(auto& item : m_publicConns)
    {
        try
        {
            sendPacket(item.second, packet);
        }
        catch (const componet::ExceptionBase& ex)
        {
            LOG_ERROR("broadcast private packet, socket异常, {}, id={}", ex, item.first);
        }
    }
}

bool TcpConnectionManager::sendPacket(ConnectionHolder::Ptr connHolder, net::Packet::Ptr packet)
{
    std::lock_guard<componet::Spinlock> lock(connHolder->sendLock);

    if (packet->size() > connHolder->conn->sendBuf().size())
    {
        LOG_TRACE("send tcp packet, 发送超大消息, size={}, remote={}", packet->size(), connHolder->conn->getRemoteEndpoint());
        connHolder->conn->sendBuf().resize(packet->size());
    }

    if(!connHolder->sendQueue.empty()) //待发送队列非空, 直接排队
    {
        if(connHolder->sendQueue.size() > 512)
            return false;

        connHolder->sendQueue.push_back(packet);
        LOG_TRACE("发送过慢，发送队列出现排队, connId:{}", connHolder->id);
        return true;
    }

    const auto& rawBuf = connHolder->conn->sendBuf().writeable(packet->size());
    if (rawBuf.second >= packet->size() )
    {
        packet->copy(rawBuf.first, rawBuf.second);
        connHolder->conn->sendBuf().commitWrite(packet->size());
        if(connHolder->conn->trySend())
            return true; //直接发送成功
    }
    else
    {
        //buf空间不足, 无法放入, 排队, 等epoll_write事件时处理这些未发出的消息
        connHolder->sendQueue.push_back(packet);
    }

    //没有一次性发送成功，注册epoll_write事件
    m_epoller.modifySocket(connHolder->conn->getFD(), net::Epoller::Event::read_write);
    return true;
}

uint32_t TcpConnectionManager::totalPrivateConnNum() const
{
    uint32_t ret = 0;
    for(auto& item : m_privateConns)
        ret += item.second.size();

    return ret;
}

}}
