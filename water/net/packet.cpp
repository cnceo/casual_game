#include "packet.h"
#include <cstring>

namespace water{
namespace net{

const Packet::SizeType Packet::MIN_SIZE;
const Packet::SizeType Packet::MAX_SIZE;
const Packet::SizeType Packet::MIN_CONTENT_SIZE;
const Packet::SizeType Packet::MAX_CONTENT_SIZE;
const Packet::SizeType Packet::HEAD_SIZE;

Packet::Packet()
{
}

Packet::Packet(SizeType size)
: m_buf(size)
{
}

Packet::Packet(const void* data, SizeType size)
: m_buf(reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + size)
{
}

uint8_t* Packet::data()
{
    return m_buf.data();
}

const uint8_t* Packet::data() const
{
    return m_buf.data();
}

void Packet::assign(const void* data, SizeType size)
{
    auto tmp = reinterpret_cast<const uint8_t*>(data);
    m_buf.assign(tmp, tmp + size);
}

Packet::SizeType Packet::copy(void* buff, SizeType maxSize)
{
    SizeType copySize = maxSize > m_buf.size() ? maxSize : m_buf.size();
    std::memcpy(buff, m_buf.data(), copySize);
    return copySize;
}

void Packet::resize(SizeType size)
{
    m_buf.resize(size);
}

Packet::SizeType Packet::size() const
{
    return m_buf.size();
}


}}

