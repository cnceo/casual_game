/*
 * Author: LiZhaojia
 *
 * Created: 2015-03-07 15:44 +0800
 *
 * Modified: 2015-03-07 15:44 +0800
 *
 * Description:  统一注册注册定时器事件
 */

#include "gateway.h"

#include "water/componet/logger.h"

namespace gateway{


void Gateway::registerTimerHandler()
{
    using namespace std::placeholders;
//    m_timer.regEventHandler(std::chrono::milliseconds(100), std::bind(&RoleManager::timerExec, &RoleManager::me(), _1));
}


}
