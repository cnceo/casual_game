#include "redis_handler.h"
#include "componet/logger.h"
#include "componet/format.h"
#include "componet/xmlparse.h"


namespace water{
namespace dbadaptcher{

using namespace water;

#define makeReply(ret) (std::shared_ptr<redisReply>(static_cast<redisReply*>(ret), freeReplyObject))


RedisHandler& RedisHandler::me()
{
    static thread_local RedisHandler* p = new RedisHandler();
    return *p;
}

RedisHandler::RedisHandler()
:m_host("127.0.0.1"), m_port(6379)
{

}

void RedisHandler::loadConfig(const std::string& cfgDir)
{
    using componet::XmlParseDoc;
    using componet::XmlParseNode;

    const std::string configFile = cfgDir + "/process.xml";

    LOG_TRACE("读取redis配置 {}", configFile);

    XmlParseDoc doc(configFile);
    XmlParseNode root = doc.getRoot();
    if(!root)
    {
        LOG_TRACE("load redis cfg file failed(1), use default values");
        return;
    }
        

    XmlParseNode redisNode = root.getChild("redis");
    if (!redisNode)
    {
        LOG_TRACE("load redis cfg file failed(2), use default values");
        return;
    }

    m_host = redisNode.getAttr<std::string>("host");
    m_port = redisNode.getAttr<size_t>("port");
    LOG_TRACE("load redis cfg successful, host={}, port={}", m_host, m_port);
}

bool RedisHandler::init()
{
    if (m_ctx == nullptr)
        m_ctx.reset(redisConnect(m_host.c_str(), m_port));

    if (m_ctx == nullptr || m_ctx->err)
    {   
        LOG_ERROR("connection to redis-server failed");
        return false;
    }   
    return true;
}

bool RedisHandler::set(const std::string& key, const std::string& data)
{
    if (!init())
        return false;

    const std::string& cmd = componet::format("set {} %b", key);
    void* ret = redisCommand(m_ctx.get(), cmd.c_str(), data.data(), data.size());
    if (ret == nullptr)
        return false;

    auto reply = makeReply(ret);
    return (reply->type == REDIS_REPLY_STATUS) && (strcasecmp(reply->str, "OK") == 0);
}

std::string RedisHandler::get(const std::string& key)
{
    if (!init())
        return ""; 

    const std::string& cmd = componet::format("get {}", key);
    void* ret = redisCommand(m_ctx.get(), cmd.c_str());
    if (ret == nullptr)
        return "";

    auto reply = makeReply(ret);
    if (reply->type != REDIS_REPLY_STRING)
        return "";

    return std::string(reply->str, reply->len);
}

bool RedisHandler::del(const std::string& key)
{
    if (!init())
        return false;

    const std::string& cmd = componet::format("del {}", key);
    void* ret = redisCommand(m_ctx.get(), cmd.c_str());
    if (ret == nullptr)
        return false;

    auto reply = makeReply(ret);
    return (reply->type == REDIS_REPLY_INTEGER);
}

bool RedisHandler::hset(const std::string table, const std::string& key, const std::string& data)
{
    if (!init())
        return false;

    const std::string& cmd = componet::format("hset {} {} %b", table, key);
    void* ret = redisCommand(m_ctx.get(), cmd.c_str(), data.data(), data.size());
    if (ret == nullptr)
        return false;

    auto reply = makeReply(ret);
    return (reply->type == REDIS_REPLY_INTEGER);
}

std::string RedisHandler::hget(const std::string table, const std::string& key)
{
    if (!init())
        return ""; 

    const std::string& cmd = componet::format("hget {} {}", table, key);
    void* ret = redisCommand(m_ctx.get(), cmd.c_str());
    if (ret == nullptr)
        return "";

    auto reply = makeReply(ret);
    if (reply->type != REDIS_REPLY_STRING)
        return "";

    return std::string(reply->str, reply->len);
}

bool RedisHandler::hdel(const std::string table, const std::string& key)
{
    if (!init())
        return false;

    const std::string& cmd = componet::format("hdel {} {}", table, key);
    void* ret = redisCommand(m_ctx.get(), cmd.c_str());
    if (ret == nullptr)
        return false;

    auto reply = makeReply(ret);
    return (reply->type == REDIS_REPLY_INTEGER);
}

bool RedisHandler::hgetall(std::unordered_map<std::string, std::string>* ht, const std::string table)
{
    if (ht == nullptr)
        return false;

    if (!init())
        return false; 

    const std::string formatStr = componet::format("hscan {} {} count 200", table);
    auto cursor = 0;
    do {
        const std::string& cmd = componet::format(formatStr, cursor);
        void* ret = redisCommand(m_ctx.get(), cmd.c_str(), cursor);
        if (ret == nullptr)
            return false;

        auto reply = makeReply(ret);
        if (reply->type != REDIS_REPLY_ARRAY)
            return false;

        if (reply->elements == 0)
            continue;

        cursor = atol(reply->element[0]->str);

        auto itemSize      = reply->element[1]->elements;
        redisReply** items = reply->element[1]->element;

        for (auto i = 0u; i < itemSize; i += 2)
        {
            std::string   key(items[i]->str,     items[i]->len);
            std::string value(items[i + 1]->str, items[i + 1]->len);
            (*ht)[key] = value;
        }
    } while(cursor != 0);

    return true;
}

int32_t RedisHandler::htraversal(const std::string& table, const std::function<bool (const std::string& key, const std::string& data)>& exec)
{
    if (!init())
        return -1;

    int32_t total = 0;
    const std::string formatStr = componet::format("hscan {} {} count 200", table);
    auto cursor = 0;
    do {
        const std::string& cmd = componet::format(formatStr, cursor);
        void* ret = redisCommand(m_ctx.get(), cmd.c_str(), cursor);
        if (ret == nullptr)
            return -1;

        auto reply = makeReply(ret);
        if (reply->type != REDIS_REPLY_ARRAY)
            return -1;

        if (reply->elements == 0)
            continue;

        cursor = atol(reply->element[0]->str);

        auto itemSize      = reply->element[1]->elements;
        redisReply** items = reply->element[1]->element;

        for (auto i = 0u; i < itemSize; i += 2)
        {
            total++;
            std::string   key(items[i]->str,     items[i]->len);
            std::string value(items[i + 1]->str, items[i + 1]->len);
            if (!exec(key, value))
                return total;
        }
    } while(cursor != 0);

    return total;
}

}}

#ifdef REDIS_UNIT_TEST
//compile cmd:

//g++ -g-std=c++14 -DREDIS_UNIT_TEST redis_handler.cpp -I/home/water/github/casual_game/water/libs/google_protobuf/installed/include -I/home/water/github/casual_game -I/home/water/github/casual_game/water -I/home/water/github/casual_game/processes -I/home/water/github/casual_game/permanent -I/home/water/github/casual_game/water/libs/hiredis/installed/include -Wl,-dn -L/home/water/github/casual_game/protocol/protobuf -lprotobuf_message -L/home/water/github/casual_game/protocol/rawmsg -lrawmsg -L/home/water/github/casual_game/base -lbase -L/home/water/github/casual_game/permanent -lpermanent -L/home/water/github/casual_game/water/net -lnet -L/home/water/github/casual_game/water/componet -lcomponet -L/home/water/github/casual_game/water/dbadaptcher -ldbadaptcher -L/home/water/github/casual_game/water/coroutine -lcoroutine  -L/home/water/github/casual_game/water/libs/google_protobuf/installed/lib -lprotobuf -L/home/water/github/casual_game/water/libs/hiredis/installed/lib -lhiredis -Wl,-dy -lpthread



#include <iostream>
using namespace std;

using namespace water;
using namespace process;

void set_get()
{
    RedisHandler& handler = RedisHandler::me();
    for (int i = 0; i < 100000; ++i)
    {
        if (!handler.set(std::to_string(i), std::to_string(i)))
        {
            cout << "set error " << i << endl;
            break;
        }
    }

    for (int i = 0; i < 100000; ++i)
    {
        if (handler.get(std::to_string(i)) != std::to_string(i))
        {
            cout << "get error " << i << endl;
            break;
        }
    }
}

int hset_hget()
{
    RedisHandler& handler = RedisHandler::me();
    for (int i = 0; i < 10000; ++i)
    {
        if (!handler.hset("h", std::to_string(i), std::to_string(i)))
        {
            cout << "set error " << i << endl;
            break;
        }
    }

    for (int i = 0; i < 10000; ++i)
    {
        if (handler.hget("h", std::to_string(i)) != std::to_string(i))
        {
            cout << "get error " << i << endl;
            break;
        }
    }

    std::unordered_map<std::string, std::string> ht;
    handler.hgetall(&ht, "h");
    cout << "size :" << ht.size() << endl;

    auto exec = [](auto key, auto value)->bool
    {
        if (key != value)
            cout << "travel key error, key=" << key << " value=" << value << endl;
        return key == value;
    };
    cout << "htraversal: " << handler.traversal("h", exec) << endl;

}

int main()
{
    hset_hget();
}

#endif


