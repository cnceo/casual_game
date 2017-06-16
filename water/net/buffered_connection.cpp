#include "buffered_connection.h"


namespace water{ 
namespace net{

ConnectionBuffer::ConnectionBuffer(uint32_t sizeLimit /*= 4096*/)
    : m_buf(sizeLimit)//先按定长处理, 带有最大长度限制的变长buf支持, 留待以后再做优化
{
}

std::pair<uint8_t*, uint32_t> ConnectionBuffer::readable()
{
    std::pair<uint8_t*, uint32_t> ret;
    ret.first  = m_buf.data() + m_dataBegin;
    ret.second = m_dataEnd - m_dataBegin;
    return ret;
}

bool ConnectionBuffer::commitRead(uint32_t size)
{
    if (m_dataEnd - m_dataBegin < size)
        return false;

    m_dataBegin += size;
    if (m_dataBegin == m_dataEnd) //读空了
    {
        m_dataBegin = 0;
        m_dataEnd = 0;
    }

    return true;
}

std::pair<uint8_t*, uint32_t> ConnectionBuffer::writeable(uint32_t size)
{
    std::pair<uint8_t*, uint32_t> ret(m_buf.data() + m_dataEnd, 0);
    if (m_buf.size() - m_dataEnd < size || m_buf.size() == m_dataEnd) //尾部空闲空间不足
    {
       if (m_dataBegin == 0) //前面也没空间了
           return ret;

       memmove(m_buf.data(), m_buf.data() + m_dataBegin, m_dataEnd - m_dataBegin);
    }
    ret.first  = m_buf.data() + m_dataEnd;
    ret.second = m_buf.size() - m_dataEnd;
    return ret;
}

bool ConnectionBuffer::commitWrite(uint32_t size)
{
    if (m_buf.size() - m_dataEnd < size) //
        return false;

    m_dataEnd += size;
    return true;
}

///////////////////////////////////////////////////////////////////////////

BufferedConnection::BufferedConnection(TcpConnection&& tcpConn)
:TcpConnection(std::forward<TcpConnection&&>(tcpConn))
{
}

bool BufferedConnection::trySend()
{
    const auto& rawBuf = m_sendBuf.readable();
    if (rawBuf.second == 0)
        return true;

    const auto sendSize = send(rawBuf.first, rawBuf.second);
    if (sendSize == -1) //socket忙, 数据无法写入
        return false;
    
    m_sendBuf.commitRead(sendSize); //不可能失败
    return m_sendBuf.empty();
}

bool BufferedConnection::tryRecv()
{
    const auto& rawBuf = m_recvBuf.writeable();
    if (rawBuf.second == 0)
        return !m_recvBuf.empty();

    const auto recvSize = recv(rawBuf.first, rawBuf.second);
    if (recvSize == 0)
        SYS_EXCEPTION(ReadClosedConnection, "BufferedConnection::tryRecv");

    if(recvSize == -1) //暂时无数据
        return !m_recvBuf.empty();

    m_recvBuf.commitWrite(recvSize); //不可能失败
    return true;
}

}}

