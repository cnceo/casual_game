#include "platform_handler.h"

#include "client_manager.h"
#include "game_config.h"

#include "dbadaptcher/redis_handler.h"

#include "componet/string_kit.h"
#include "componet/logger.h"

#include "protocol/protobuf/private/gm.codedef.h"


namespace lobby{

using water::dbadaptcher::RedisHandler;

void rechargeHandler()
{
    const char* redislist = "pf_lb_recharge";
    auto& redis = RedisHandler::me();
    for( std::string msg = redis.lpop(redislist); msg != ""; msg = redis.lpop(redislist) )
    {
        LOG_TRACE("recharge msg from platfrom, {}", msg);
        std::vector<std::string> nodes = componet::splitString(msg, ",");
        if (nodes.size() < 4)
        {
            LOG_ERROR("recharge msg from platfrom, drop broken msg, itemsize={}", nodes.size());
            continue;
        }
        const auto sn    = componet::fromString<int32_t>(nodes[0]);
        const auto cuid  = componet::fromString<ClientUniqueId>(nodes[1]);
        const auto money = componet::fromString<int32_t>(nodes[2]);

        ClientManager::me().rechargeMoney(sn, cuid, money, nodes[3]);
    }
}


void PlatformHandler::timerExec(const componet::TimePoint& now)
{
    rechargeHandler();
}


/*********************************************************************/


void PlatformHandler::regMsgHandler()
{
    using namespace std::placeholders;

//    auto loadGameCfg = [](ClientConnectionId ccid) { GameConfig::me().reload(); };
    REG_PROTO_PRIVATE(LoadGameConfig, std::bind( ([](ClientConnectionId ccid) { GameConfig::me().reload();}), _2 ));
}

}

