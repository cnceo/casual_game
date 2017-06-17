/*
 * Author: LiZhaojia 
 *
 * Last modified: 2014-10-11 23:02 +0800
 *
 * Description: 
 */

#ifndef WATER_HTTP_CONNECTION_MANAGER_H
#define WATER_HTTP_CONNECTION_MANAGER_H

#include "net/buffered_connection.h"
#include "net/epoller.h"

#include "componet/event.h"
#include "componet/class_helper.h"
#include "componet/spinlock.h"
#include "componet/lock_free_circular_queue_ss.h"

#include "process_id.h"
#include "process_thread.h"

#include <unordered_map>
#include <atomic>
#include <list>

namespace water{
namespace net {class BufferedConnection; class HttpPacket; class Packet;}
namespace process{

class HttpConnectionManager : public ProcessThread
{
    using HttpPacketPtr = std::shared_ptr<net::HttpPacket>;
    using HttpPacketCPtr = std::shared_ptr<const net::HttpPacket>;
    using PacketPtr = std::shared_ptr<net::Packet>;
public:

    enum class ConnType
    {
        client,
        server
    };
    struct ConnectionHolder
    {
        TYPEDEF_PTR(ConnectionHolder);

        int64_t id;
        std::shared_ptr<net::BufferedConnection> conn;
        ConnType type = ConnType::client;
        HttpPacketPtr recvPacket = nullptr;
        //由于socke太忙而暂时无法发出的包，缓存在这里
        componet::Spinlock sendLock;
        std::list<PacketPtr> sendQueue; 
        bool keeyAlive = false;
    };
public:
    HttpConnectionManager();
    ~HttpConnectionManager() = default;

    bool exec() override;

    void addConnection(std::shared_ptr<net::BufferedConnection> conn, ConnType type);
    void delConnection(std::shared_ptr<net::BufferedConnection> conn);

    void sendPacket(HttpConnectionId hcid, PacketPtr packet);

private:
    void epollerEventHandler(int32_t socketFD, net::Epoller::Event event);

    //从接收队列中取出一个packet, 并得到与其相关的conn
    bool sendPacket(ConnectionHolder::Ptr connHolder, PacketPtr packet);

private:

	int32_t uniqueID = 0;
    componet::Spinlock m_lock;

    net::Epoller m_epoller;
    //所有的连接, {fd, conn}
    std::unordered_map<int32_t, ConnectionHolder::Ptr> m_allConns;

    componet::LockFreeCircularQueueSPSC<std::pair<ConnectionHolder::Ptr, HttpPacketCPtr>> m_recvQueue;
};

}}

#endif
