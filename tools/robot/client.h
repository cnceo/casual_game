#include "net/connection.h"
#include "net/packet_connection.h"
#include "net/epoller.h"
#include "base/tcp_packet.h"
#include "base/tcp_message.h"
#include "componet/logger.h"
#include "componet/timer.h"
#include "componet/circular_queue.h"
#include "protocol/protobuf/proto_manager.h"

using namespace water;

using TcpMsgCode = process::TcpMsgCode;

const char cfgDir[] = "/home/water/github/casual_game/config";
const net::Endpoint serverAddr("127.0.0.1:7000");
//const net::Endpoint serverAddr("119.23.71.237:7000");

class Client
{
public:
    Client();
    void run();

    bool sendMsg(TcpMsgCode msgCode, const ProtoMsg& proto);
    ProtoMsgPtr recvMsg(TcpMsgCode msgCode);

    bool trySendMsg(TcpMsgCode msgCode, const ProtoMsg& proto);

private:
    void epollEventHandler(net::Epoller::Event event);
    void dealMsg(const componet::TimePoint& now);
    void regMsgHandler();
    ProtoMsgPtr tryRecvMsg(TcpMsgCode msgCode);

private:
    net::PacketConnection::Ptr m_conn;
    net::Epoller m_epoller;
    componet::Timer m_timer;
    componet::CircularQueue<net::Packet::Ptr> m_recvQue;
    componet::CircularQueue<net::Packet::Ptr> m_sendQue;

public:
    static Client& me();
private:
    static Client s_me;
    net::Endpoint m_serverEp;
};

#define SEND_MSG(msgName, proto) Client::me().sendMsg(PROTO_CODE_PUBLIC(msgName), proto);
#define RECV_MSG(msgName) PROTO_PTR_CAST_PUBLIC(msgName, Client::me().recvMsg(PROTO_CODE_PUBLIC(msgName)));
