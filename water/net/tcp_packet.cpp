#include "tcp_packet.h"
#include <cstring>

namespace water{
namespace net{

TcpPacket::Ptr TcpPacket::tryParse(uint8_t* data, SizeType size)
{
    if (size < sizeof(SizeType))
        return nullptr;
    if (size < *reinterpret_cast<const SizeType*>(data) + sizeof(size))
        return nullptr;
    auto packet = new Packet(data, size);
    auto ret = TcpPacket::Ptr(static_cast<TcpPacket*>(packet));
    return ret;
}

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
    if(size() < sizeof(SizeType))
        return nullptr;

    return data() + sizeof(SizeType);
}

TcpPacket::SizeType TcpPacket::contentSize() const
{
    if(size() < sizeof(SizeType))
        return 0;

    return size() - sizeof(SizeType);
}


}}
