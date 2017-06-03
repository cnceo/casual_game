#include "client_manager.h"
#include "gateway.h"

#include "protocol/protobuf/proto_manager.h"
#include "protocol/protobuf/public/client.codedef.h"

namespace gateway{

using namespace std::placeholders;

static ProcessId lobbyPid;
#define PUBLIC_MSG_TO_LOBBY(clientMsgName) \
ProtoManager::me().regHandler(PROTO_CODE_PUBLIC(clientMsgName), std::bind(&ClientManager::relayClientMsgToServer, this, lobbyPid, PROTO_CODE_PUBLIC(clientMsgName),  _1, _2));

static ProcessId hallPid("hall", 1);
#define PUBLIC_MSG_TO_HALL(clientMsgName) \
ProtoManager::me().regHandler(PROTO_CODE_PUBLIC(clientMsgName), std::bind(&ClientManager::relayClientMsgToServer, this, hallPid, PROTO_CODE_PUBLIC(clientMsgName),  _1, _2));

#define PUBLIC_MSG_TO_CLIENT(clientMsgName)\
ProtoManager::me().regHandler(PROTO_CODE_PUBLIC(clientMsgName), std::bind(&ClientManager::relayClientMsgToClient, this, PROTO_CODE_PUBLIC(clientMsgName), _1, _2));


void ClientManager::regClientMsgRelay()
{
    lobbyPid = ProcessId("lobby", 1);
    hallPid  = ProcessId("hall", 1);

    /************转发到lobby************/
    PUBLIC_MSG_TO_LOBBY(C_G13_CreateGame)
    PUBLIC_MSG_TO_LOBBY(C_G13_JionGame)
    PUBLIC_MSG_TO_LOBBY(C_G13_GiveUp)
    PUBLIC_MSG_TO_CLIENT(C_G13_VoteFoAbortGame)
    PUBLIC_MSG_TO_LOBBY(C_G13_ReadyFlag)
    PUBLIC_MSG_TO_LOBBY(C_G13_BringOut)

    /************转发到hall*************/
//    PUBLIC_MSG_TO_HALL()
    /*************转发到client**********/
    PUBLIC_MSG_TO_CLIENT(S_Notice)
    PUBLIC_MSG_TO_CLIENT(S_G13_RoomAttr)
    PUBLIC_MSG_TO_CLIENT(S_G13_AbortGameOrNot)
    PUBLIC_MSG_TO_CLIENT(S_G13_PlayerQuited)
    PUBLIC_MSG_TO_CLIENT(S_G13_PlayersInRoom)
    PUBLIC_MSG_TO_CLIENT(S_G13_HandOfMine)
    PUBLIC_MSG_TO_CLIENT(S_G13_AllHands)

};

}
