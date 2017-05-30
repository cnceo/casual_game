#include "tcp_packet.h"
#include <cstring>

namespace water{
namespace process{

TcpPacket::TcpPacket()
: Packet(sizeof(SizeType))
{
}

TcpPacket::TcpPacket(SizeType contentSize)
    : net::Packet(sizeof(SizeType) + contentSize)
{
    std::memcpy(data(), &contentSize, sizeof(contentSize));
}

void TcpPacket::setContent(const void* content, SizeType contentSize)
{
    const SizeType packetSize = sizeof(SizeType) + contentSize;
    resize(packetSize);
    std::memcpy(data(), &contentSize, sizeof(contentSize));
    std::memcpy(data() + sizeof(contentSize), content, contentSize);
}

void* TcpPacket::content()
{
    /*
    if(m_type == Packet::BuffType::send && m_cursor != size())
        return nullptr;
*/
    if(m_type == Packet::BuffType::recv && m_cursor != size())
        return nullptr;
    if(size() < sizeof(SizeType))
        return nullptr;

    return data() + sizeof(SizeType);
}
/*
void* TcpPacket::resizeContent(SizeType contentSize)
{
    const SizeType packetSize = sizeof(SizeType) + contentSize;
    resize(packetSize);
    return data() + sizeof(SizeType);
}
*/
TcpPacket::SizeType TcpPacket::contentSize() const
{
    /*
    if(m_type == Packet::BuffType::send && m_cursor != size())
        return 0;
*/
    if(m_type == Packet::BuffType::recv && m_cursor != size())
        return 0;
    if(size() < sizeof(SizeType))
        return 0;

    return size() - sizeof(SizeType);
}


void TcpPacket::addCursor(SizeType add)
{
    m_cursor += add;
    //当收到一个SizeType长度的数据时, 即得知了这个包的实际总长度，
    //把需要收取的长度改写为包的实际长度，以便继续接收
    if(m_type == Packet::BuffType::recv && m_cursor == sizeof(SizeType))
    {
        SizeType packetSize = *reinterpret_cast<const SizeType*>(data()) + sizeof(SizeType);
        resize(packetSize);
    }
}

}}
