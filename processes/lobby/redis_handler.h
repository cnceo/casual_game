#ifndef LOBBY_REDIS_HANDLER_HPP
#define LOBBY_REDIS_HANDLER_HPP


#include "cpp_redis/cpp_redis"

namespace lobby{

enum class RedisDBType
{
    null = 0,
    clients = 1,
    rooms = 2,
};

class RedisHandler
{
public:
    RedisHandler();
    ~RedisHandler() = default;

    bool start();
    void stop();

    cpp_redis::redis_client& redisClient() const;

private:
    void loadConfig();

private:
    std::string m_redisServerHost;
    std::size_t port;
    std::unique_ptr<cpp_redis::redis_client> m_redisClient;
};

} //end namespace

#endif
