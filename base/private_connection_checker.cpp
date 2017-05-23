#include "private_connection_checker.h"

#include "componet/logger.h"

namespace water{
namespace process{


PrivateConnectionChecker::PrivateConnectionChecker(ProcessId processId)
: m_processId(processId)
{
}

void PrivateConnectionChecker::addUncheckedConnection(net::PacketConnection::Ptr conn, ConnType type)
{
    try
    {
        conn->setNonBlocking();

        ConnInfo connInfo;
        if(type == ConnType::in)
        {
            connInfo.state = ConnState::recvId;
            conn->setRecvPacket(net::Packet::create(sizeof(ProcessId)));
        }
        else
        {
            connInfo.state = ConnState::sendId;
            conn->setSendPacket(net::Packet::create(&m_processId, sizeof(m_processId)));
        }
        connInfo.conn = conn;

        LockGuard lock(m_mutex);
        m_conns.push_back(connInfo);
    }
    catch(const componet::ExceptionBase& ex)
    {
        const char* typeStr = (type == ConnType::in) ? "incoming" : "outgoing";
        LOG_ERROR("check {} private connection failed, remoteEp={}, {}", 
                  typeStr, conn->getRemoteEndpoint(), ex);
        conn->close();
    }
}

void PrivateConnectionChecker::checkConn()
{
    while(checkSwitch())
    {
        m_mutex.lock();
        for(auto it = m_conns.begin(); it != m_conns.end(); )
        {
            try
            {
                switch (it->state)
                {
                case ConnState::recvId:
                    {
                        if( !it->conn->tryRecv() )
                            break;

                        //记下接入者id
                        auto packet = it->conn->getRecvPacket();
                        it->remoteId = *reinterpret_cast<const ProcessId*>(packet->data());
                        //检查接入者id是否合法(当前只检查格式有效性)
                        ConnCheckRetMsg checkRet;
                        checkRet.pid = m_processId;
                        if(it->remoteId.isValid())
                        {

                            //检查通过
                            checkRet.result = true;
                            it->checkResult = true;
                            it->conn->setSendPacket(net::Packet::create(&checkRet, sizeof(checkRet)));
                        }
                        else
                        {
                            //检查未通过
                            checkRet.result = false;
                            it->checkResult = false;
                            it->conn->setSendPacket(net::Packet::create(&checkRet, sizeof(checkRet)));
                        }
                        it->state = ConnState::sendRet;
                    }
                    //no break
                case ConnState::sendRet:
                    {
                        if( !it->conn->trySend() )
                            break;

                        //发完验证结果，处理已经做过验证的连接
                        if(it->checkResult)
                        {
                            e_connConfirmed(it->conn, it->remoteId);
                            LOG_TRACE("incoming private conn from {} accepted", it->remoteId);
                        }
                        else
                        {
                            LOG_TRACE("incoming private conn from {}  denied, remoteEp={}", 
                                      it->remoteId, it->conn->getRemoteEndpoint());
                        }

                        it = m_conns.erase(it);
                        continue;
                    }
                    break;
                case ConnState::sendId:
                    {
                        //发送自己的id给监听进程
                        if( !it->conn->trySend() )
                            break;

                        //发完自己的id，进入等待对方回复的状体
                        it->conn->setRecvPacket(net::Packet::create(sizeof(ConnCheckRetMsg)));
                        it->state = ConnState::recvRet;
                    }
                    //no break
                case ConnState::recvRet:
                    {
                        if( !it->conn->tryRecv() )
                            break;

                        //记下对方回复的id
                        auto packet = it->conn->getRecvPacket();
                        const ConnCheckRetMsg* checkRet = reinterpret_cast<const ConnCheckRetMsg*>(packet->data());
                        it->remoteId = checkRet->pid;
                        it->checkResult = checkRet->result;

                        //检查回复内容
                        if(it->checkResult)
                        {
                            e_connConfirmed(it->conn, it->remoteId);
                            LOG_TRACE("outgoing private conn to {} accepted, remoteEp={}", 
                                      it->remoteId, it->conn->getRemoteEndpoint());
                        }
                        else
                        {
                            LOG_TRACE("outgonig private conn to {} denied, remoteEp={}", 
                                      it->remoteId, it->conn->getRemoteEndpoint());
                        }

                        //done
                        it = m_conns.erase(it);
                        continue;
                    }
                    break;
                default:
                    break;
                }
            }
            catch (const net::NetException& ex)
            {
                const char* typeStr = (it->state == ConnState::recvId ||
                                       it->state == ConnState::sendRet) ? "in" : "out";
                LOG_ERROR("检查 {} 连接时出错 remoteEp={}, {}", 
                          typeStr, it->conn->getRemoteEndpoint(), ex.what());
                it = m_conns.erase(it);
                continue;
            }
            ++it;
        }
        m_mutex.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

bool PrivateConnectionChecker::exec()
{
    checkConn();
    return true; //正常退出
}

}}
