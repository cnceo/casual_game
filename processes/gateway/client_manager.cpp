#include "client_manager.h"

#include "compnect/logger.h"

namespace gateway{


ClientIdentity::ClientManager(ProcessIdentity processId)
: m_processId(processId)
{
}

void ClientManager::clientOnline()
{
    if (conn == nullptr)
        return;
    
    ClientIdentity clientId = getClientIdendity();
    Client client;


    if (!m_clients.insert({clientId, conn}))
        return INVALID_CLIENT_IDENDITY_VALUE;

    return clientId;
}

void ClientIdentity::clientOffline(net::PacketConnection::Ptr conn)
{
}


}
