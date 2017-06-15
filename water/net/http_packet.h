#ifndef WATER_NET_HTTP_PACKET_H
#define WATER_NET_HTTP_PACKET_H


//#include "http-parser/http_parser.h"
#include "../componet/class_helper.h"

#include <cstdint>
#include <string>

namespace water{
namespace net{

using Parser = http_parser;

enum class HttpType
{
    request, response,
};

class HttpPacket : 
{
public:
    TYPEDEF_PTR(HttpPacket)
    CREATE_FUN_MAKE(HttpPacket)
    HttpPacket(HttpType type);

    bool tryParse(const char* data, size_t size);

private:
    Parser m_parser;
    bool m_completed = true;
    struct Detial
    {
        std::map<std::string, std::string> headers;
        enum
        {
            uint32_t statusCode = 0;
            Method method;
        };
        std::string body;
        std::string cmd;
    } m_detial;

public:
    static HttpPacket::Ptr tryParse(HttpType type, const char* data, size_t size);

private:
    struct ParserSettings : http_parser_settings {};
    static ParserSettings s_settings;

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

