/*
 * Author: LiZhaojia - waterlzj@gmail.com
 *
 * Last modified: 2017-05-19 15:10 +0800
 *
 * Description: 
 */

#include "lobby.h"
#include "room.h"

#include "water/componet/logger.h"

namespace lobby{


void Lobby::registerTimerHandler()
{
    m_timer.regEventHandler(std::chrono::seconds(1), &Room::timerExecAll);
}


}
