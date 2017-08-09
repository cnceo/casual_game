#include "platform_handler.h"

#include "client_manager.h"

#include "dbadaptcher/redis_handler.h"

#include "componet/string_kit.h"
#include "componet/logger.h"

namespace lobby{

using water::dbadaptcher::RedisHandler;

void rechargeHandler()
{
    const char* redislist = "pf_recharge";
    auto& redis = RedisHandler::me();
    for( std::string msg = redis.lpop(redislist); msg != ""; msg = redis.lpop(redislist) )
    {
        LOG_TRACE("recharge msg from platfrom, {}", msg);
        std::vector<std::string> nodes = componet::splitString(msg, ",");
        const uint32_t sn   = componet::fromString<int32_t>(nodes[0]);
        const auto cuid     = componet::fromString<ClientUniqueId>(nodes[1]);
        const int32_t money = componet::fromString<int32_t>(nodes[2]);

        ClientManager::me().rechargeMoney(sn, cuid, money, nodes[3]);
    }
}


void PlatformHandler::timerExec(const componet::TimePoint& now)
{
    rechargeHandler();
}


}

