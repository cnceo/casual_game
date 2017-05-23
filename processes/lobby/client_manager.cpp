#include "client_manager.h"

#include "redis_handler.h"

namespace lobby{


ClientManager* ClientManager::s_me = nullptr;

ClientManager& ClientManager::getMe()
{
    if(s_me == nullptr)
        s_me = new ClientManager();
    return *s_me;
}

////////////////////


void ClientManager::recoveryFromRedis()
{
    std::vector<std::string> redisCmd = {"info"};
}


}
