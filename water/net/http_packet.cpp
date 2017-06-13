#include "http_parser.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h> /* rand */
#include <string.h>
#include <stdarg.h>


//设计, 不用典型的ringbuf, 线性buf, 因为ringbuf在接缝处处理时, 还是需要memcpy, 没太大意义
//遇到buf结尾, 把未解析的数据memmove到buf头部, 然后继续
/*
bool connection::tryRecv()
{
    int size = read(sock, buf, maxSize);
    if (size == -1)
        execption;

    int packetDataSize = m_recvPacket.tryParse(data.begin(), data.end());
    
}
*/

using HttpParser = http_parser;

class HttpPacket
{
public:
    HttpPacket();
    bool tryParse(const char* data, size_t size);

private:
    HttpParser m_parser;

private:
    static http_parser_settings s_settings;

private:
    static void onPacketBegin(HttpParser* parser);
    static void onUrl(HttpParser* parser, const char* data, size_t size);
    static void onStatus(HttpParser* parser, const char* data, size_t size);
    static void onHeaderField(HttpParser* parser, const char* data, size_t size);
    static void onHeaderValue(HttpParser* parser, const char* data, size_t size);
    static void onHeadersComplete(HttpParser* parser);
    static void onBody(HttpParser* parser, const char* data, size_t size);
    static void onPacketComplete(HttpParser* parser);
    static void onChunkHeader(HttpParser* parser);
    static void onChunkComplete(HttpParser* parser);
};

static HttpParser::s_setting =
{
     .on_message_begin      = HttpPacket::onPacketBegin
    ,.on_url                = HttpPacket::onUrl
    ,.on_status             = HttpPacket::onStatus
    ,.on_header_field       = HttpPacket::onHeaderField
    ,.on_header_value       = HttpPacket::onHeaderValue
    ,.on_body               = HttpPacket::onBody
    ,.on_headers_complete   = HttpPacket::onHeadersComplete
    ,.on_message_complete   = HttpPacket::onPacketComplete
    ,.on_chunk_header       = HttpPacket::onChunkHeader
    ,.on_chunk_complete     = HttpPacket::onChunkComplete
};

HttpParser::HttpParser()
{
    http_parser_init(&m_parser, HTTP_REQUEST : HTTP_REQUEST);
}

int main()
{

    return 0;
}
