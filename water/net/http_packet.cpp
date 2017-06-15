#include "http_packet.h"

#include "http-parser/http_parser.h"


//设计, 不用典型的ringbuf, 线性buf, 因为ringbuf在接缝处处理时, 还是需要memcpy, 没太大意义
//遇到buf结尾, 把未解析的数据memmove到buf头部, 然后继续
namespace water{
namespace net{


static HttpParser::s_setting =
{
    on_message_begin      = HttpPacket::onPacketBegin     ;
    on_url                = HttpPacket::onUrl             ;
    on_status             = HttpPacket::onStatus          ;
    on_header_field       = HttpPacket::onHeaderField     ;
    on_header_value       = HttpPacket::onHeaderValue     ;
    on_body               = HttpPacket::onBody            ;
    on_headers_complete   = HttpPacket::onHeadersComplete ;
    on_message_complete   = HttpPacket::onPacketComplete  ;
    on_chunk_header       = HttpPacket::onChunkHeader     ;
    on_chunk_complete     = HttpPacket::onChunkComplete   ;
};

HttpPacket::Ptr HttpPacket::tryParse(HttpType type, const char* data, size_t size)
{
    auto ret = HttpPacket::create(type);
    return ret->tryParse(data, size) ? ret : nullptr;
}

int32_t HttpPacket::onPacketBegin(Parser* parser)
{
    return 0;
}

int32_t HttpPacket::onUrl(Parser* parser, const char* data, size_t size)
{
    return 0;
}

int32_t HttpPacket::onStatus(Parser* parser, const char* data, size_t size)
{
    cout << "status, code=" << parser->status_code;
    auto packet = reinterpret_cast<HttpPacket*>(parser->data);
    packet->m_detial.statusCode = parser->statrus_code;
    return 0;
}

int32_t HttpPacket::onHeaderField(Parser* parser, const char* data, size_t size)
{
    return 0;
}

int32_t HttpPacket::onHeaderValue(Parser* parser, const char* data, size_t size)
{
    return 0;
}

int32_t HttpPacket::onHeadersComplete(Parser* parser)
{
    return 0;
}

int32_t HttpPacket::onBody(Parser* parser, const char* data, size_t size)
{
    cout << "body: " << std::string(data, size) << endl;
    auto packet = reinterpret_cast<HttpPacket*>(parser->data);
    packet->m_detial.body.assign(data, size);
    return 0;
}

int32_t HttpPacket::onPacketComplete(Parser* parser)
{
    cout << "packet complete" << endl;
    auto packet = reinterpret_cast<HttpPacket*>(parser->data);
    packet->m_completed = true;
    return 0;
}

int32_t HttpPacket::onChunkHeader(Parser* parser)
{
    return 0;
}

int32_t HttpPacket::onChunkComplete(Parser* parser)
{
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
HttpPacket::HttpPacket(HttpType type)
{
    http_parser_init(&m_parser, type == HttpType::request ? HTTP_REQUEST : HTTP_REQUEST);
    m_parser.data = this;
}

bool HttpPacket::tryParse(const char* data, size_t size)
{
    return m_completed;
}



}}

int main()
{

}

