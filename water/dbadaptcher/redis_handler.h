#ifndef PERMANENT_REDIS_HANDLER_HPP
#define PERMANENT_REDIS_HANDLER_HPP


#include "hiredis/hiredis.h"

#include "componet/class_helper.h"

#include <string>
#include <memory>
#include <unordered_map>

namespace water{
namespace dbadaptcher{


class RedisHandler
{
public:
    NON_COPYABLE(RedisHandler)
    ~RedisHandler() = default;

    bool set(const std::string& key, const std::string& data);
    std::string get(const std::string& key);
    bool del(const std::string& key);

    bool hset(const std::string table, const std::string& key, const std::string& data);
    std::string hget(const std::string table, const std::string& key);
    bool hdel(const std::string table, const std::string& key);
    bool hgetall(std::unordered_map<std::string, std::string>* ht, const std::string table);
    int32_t htraversal(const std::string& table, const std::function<bool (const std::string& key, const std::string& data)>& exec);

    void loadConfig(const std::string& cfgDir);

private:
    RedisHandler();
    bool init();

private:
    std::string m_host;
    std::size_t m_port;
    std::unique_ptr<redisContext> m_ctx;

public:
    static RedisHandler& me();
};


}}

#endif
