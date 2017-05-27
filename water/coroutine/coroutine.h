/*
 * Author: LiZhaojia - waterlzj@gmail.com
 *
 * Last modified: 2017-05-27 14:38 +0800
 *
 * Description: 轻量化的c++协程库, 使用调度器对协程进行统一调度
 *              采用共享栈技术，支持开启海量协程
 */

#include <cstdint>
#include <functional>

namespace corot
{
using CorotId = uint32_t;


//创建协程
CorotId create(const std::function<void(void)>& func);

//销毁协程, 一般不需要调用, 协程与线程类似，在启动函数正常return后自动销毁
void destroy(CorotId tid);

//返回当前的协程数量
uint32_t doSchedule();


//在协程内调用的
namespace this_corot
{
CorotId getId();
void yield();
}
}

