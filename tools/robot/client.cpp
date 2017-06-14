#include "client.h"

#include "componet/scope_guard.h"
#include "coroutine/coroutine.h"

#include "protocol/protobuf/public/client.codedef.h"

Client& Client::me()
{
    return s_me;
}

Client Client::s_me;


Client::Client()
    : m_serverEp(serverAddr)
{
}

void Client::run()
{
    try 
    {
//        m_timer.regEventHandler(std::chrono::milliseconds(1), std::bind(&Client::dealMsg, this, std::placeholders::_1));
        m_timer.regEventHandler(std::chrono::milliseconds(1), std::bind(corot::doSchedule));
        ProtoManager::me().loadConfig(cfgDir);

        auto tcpConn = net::TcpConnection::connect(m_serverEp);
        m_conn = net::BufferedConnection::create(std::move(*tcpConn));
//        m_conn->setRecvPacket(net::TcpPacket::create());
        m_conn->setNonBlocking();
        m_epoller.setEventHandler(std::bind(&Client::epollEventHandler, this, std::placeholders::_3));
        m_epoller.regSocket(m_conn->getFD(), net::Epoller::Event::read);
        while(true)
        {
            m_epoller.wait(std::chrono::milliseconds(1));
            m_timer();
            corot::doSchedule();
        }
    }
    catch(const net::NetException& ex)
    {
        LOG_ERROR("exception occured，ex={}", ex);
        return;
    }
}

void Client::epollEventHandler(net::Epoller::Event event)
{
    try
    {
        switch (event)
        {
        case net::Epoller::Event::read:
            {
                while(m_conn->tryRecv())
                {
                    const auto& rawBuf = m_conn->recvBuf().readable();
                    auto packet = net::TcpPacket::tryParse(rawBuf.first, rawBuf.second);
                    if (packet == nullptr || !m_recvQue.push(packet))
                        break;
                    m_conn->recvBuf().commitRead(packet->size());
                }
            }
            break;
        case net::Epoller::Event::write:
            {
                m_conn->trySend();
//                while(m_conn->trySend())
//                {
//                    if(m_sendQue.empty())
//                    {
//                        m_epoller.modifySocket(m_conn->getFD(), net::Epoller::Event::read);
//                        break;
//                    }
//                    m_conn->setSendPacket(m_sendQue.top());
//                    m_sendQue.pop();
//                }
            }
            break;
        case net::Epoller::Event::error:
            {
            }
            break;
        default:
            break;
        }
    }
    catch (const net::ReadClosedConnection& ex)
    {
        LOG_TRACE("server disconnected the connection");
        m_epoller.delSocket(m_conn->getFD());
    }
    return;
}

void Client::dealMsg(const componet::TimePoint& now)
{
    while (!m_recvQue.empty())
    {
        auto packet = std::static_pointer_cast<net::TcpPacket>(m_recvQue.top());
        m_recvQue.pop();
        auto msg = reinterpret_cast<process::TcpMsg*>(packet->content());
        ProtoManager::me().dealTcpMsg(msg, packet->contentSize(), 0, now);
    }
}

bool Client::sendMsg(TcpMsgCode msgCode, const ProtoMsg& proto)
{
    return trySendMsg(msgCode, proto);
}

ProtoMsgPtr Client::recvMsg(TcpMsgCode msgCode)
{
    while(true)
    {
        auto msg = tryRecvMsg(msgCode);
        if (msg != nullptr)
            return msg;
        corot::this_corot::yield();
    }
    return nullptr; //never reach here
}

bool Client::trySendMsg(TcpMsgCode msgCode, const ProtoMsg& proto)
{
	//create packet
    const uint32_t protoBinSize = proto.ByteSize();
    const uint32_t bufSize = sizeof(process::TcpMsg) + protoBinSize;
    uint8_t* buf = new uint8_t[bufSize];
    ON_EXIT_SCOPE_DO(delete[] buf);

    process::TcpMsg* msg = new(buf) process::TcpMsg(msgCode);
    if(!proto.SerializeToArray(msg->data, protoBinSize))
    {   
        LOG_ERROR("proto serialize failed, msgCode = {}", msgCode);
        return false;
    }   

    auto packet = net::TcpPacket::create();
    packet->setContent(buf, bufSize);
        

	//send packet
#if 0
    if(m_conn->setSendPacket(packet))
        return true;
    if(!m_sendQue.push(packet))
        return false;
    m_epoller.modifySocket(m_conn->getFD(), net::Epoller::Event::read_write);
#else

    auto rawBuf = m_conn->sendBuf().writeable();
    while (rawBuf.second < packet->size())
    {
        corot::this_corot::yield();
        rawBuf = m_conn->sendBuf().writeable();
    }
    packet->copy(rawBuf.first, rawBuf.second);
    m_conn->sendBuf().commitWrite(packet->size());
    while(!m_conn->trySend())
        corot::this_corot::yield();
#endif
    return true;
}

ProtoMsgPtr Client::tryRecvMsg(TcpMsgCode msgCode)
{
    if (m_recvQue.empty())
        return nullptr;
    auto packet = std::static_pointer_cast<net::TcpPacket>(m_recvQue.top());
    auto raw = reinterpret_cast<process::TcpMsg*>(packet->content());
    if (raw->code != msgCode)
        return nullptr;
    m_recvQue.pop();
    const uint8_t* data = raw->data;
    ProtoMsgPtr ret = ProtoManager::me().create(msgCode);
    ret->ParseFromArray(data, packet->contentSize());
    return ret;
}

