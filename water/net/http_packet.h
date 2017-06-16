#ifndef WATER_NET_HTTP_PACKET_H
#define WATER_NET_HTTP_PACKET_H


//#include "http-parser/http_parser.h"
#include "../componet/class_helper.h"
#include "packet.h"

#include <cstdint>
#include <string>
#include <map>


class http_parser;
class http_parser_settings;

namespace water{
namespace net{


enum class HttpType
{
    request, response,
};

class HttpPacket //: public Packet
{
    using Parser = http_parser;
public:
    TYPEDEF_PTR(HttpPacket)
    CREATE_FUN_MAKE(HttpPacket)
    HttpPacket(HttpType type);
    ~HttpPacket();

    bool complete() const;
    HttpType type() const;
    bool keepAlive() const;

    size_t tryParse(const char* data, size_t size);

private:
    std::unique_ptr<http_parser> m_parser;
    bool m_completed = true;
    struct Detial
    {
        HttpType type = HttpType::request;
        std::string curHeaderFiled;
        std::map<std::string, std::string> headers;
        union
        {
            uint32_t statusCode = 0;
            uint32_t method;
        };
        bool keepAlive = false;
        std::string body;
        std::string url;
    } m_detial;

public:
    static std::pair<HttpPacket::Ptr, size_t> tryParse(HttpType type, const char* data, size_t size);

private:
    struct ParserSettings;
    static std::unique_ptr<http_parser_settings> s_settings;

    static int32_t onPacketBegin(Parser* parser);
    static int32_t onUrl(Parser* parser, const char* data, size_t size);
    static int32_t onStatus(Parser* parser, const char* data, size_t size);
    static int32_t onHeaderField(Parser* parser, const char* data, size_t size);
    static int32_t onHeaderValue(Parser* parser, const char* data, size_t size);
    static int32_t onHeadersComplete(Parser* parser);
    static int32_t onBody(Parser* parser, const char* data, size_t size);
    static int32_t onPacketComplete(Parser* parser);
    static int32_t onChunkHeader(Parser* parser);
    static int32_t onChunkComplete(Parser* parser);
};


}}


#endif

