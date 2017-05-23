#include "redis_handler.h"

#include "componet/logger.h"

namespace lobby{

RedisHandler::RedisHandler()
    :m_redisServerHost("127.0.0.1"), port(6379)
{

}

bool RedisHandler::start()
{
    try
    {
        m_redisClient.reset(new cpp_redis::redis_client);
        m_redisClient->connect();
    }
    catch (const cpp_redis::redis_error& ex)
    {
        LOG_ERROR("RedisHandler::start failed, redis_exception.what()={}", ex.what());
        return false;
    }
    return true;
}

void RedisHandler::stop()
{
}

cpp_redis::redis_client& RedisHandler::redisClient() const
{
    return *(m_redisClient.get());
}

}
