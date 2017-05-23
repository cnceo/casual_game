/*
 * Author: LiZhaojia - waterlzj@gmail.com
 *
 * Last modified: 2017-05-19 15:10 +0800
 *
 * Description: 
 */

#include "lobby.h"

#include "water/componet/logger.h"
//#include "protocol/protobuf/proto_manager.h"
#include "protocol/protobuf/private/gm.codedef.h"

namespace lobby{

void testPingHandler(const ProtoMsgPtr& proto, ProcessId pid)
{
//    auto msg = std::static_pointer_cast<PrivateProto::Ping>(proto);
    auto msg = PROTO_PTR_CAST_PRIVATE(Ping, proto);
    LOG_DEBUG("recv 'Ping' from {}, ping.msg = '{}'", pid, msg->msg());
    return ;
}


void Lobby::registerTcpMsgHandler()
{
   using namespace std::placeholders;
   REG_PROTO_PRIVATE(Ping, std::bind(testPingHandler, _1, _2));
}


}
