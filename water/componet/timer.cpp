#include "timer.h"

#include <thread>
//#include <mutex>

#include <iostream>
using namespace std;

namespace water{
namespace componet{

using LockGuard = std::lock_guard<componet::Spinlock>;

Timer::Timer()
//:m_wakeUp(EPOCH)
{
}

void Timer::operator()()
{

    TheTimePoint now = TheClock::now();
    TimePoint sysNow = Clock::now();
    {
        LockGuard lock(m_lock);
        for(auto& pair : m_eventHandlers)
        {
            TheClock::time_point emitTime = pair.second.regTime + pair.first * pair.second.counter;

            if(now >= emitTime) //执行一次定时事件
            {
                pair.second.event(sysNow);
                pair.second.counter++;
            }
        }
    }
    //std::this_thread::sleep_for(std::chrono::milliseconds(1)); //线程暂停1ms
    std::this_thread::yield(); //线程暂停一下
}

int64_t Timer::precision() const
{
    return TheClock::period::den;
}

Timer::RegID Timer::regEventHandler(std::chrono::milliseconds interval,
                            const std::function<void (const TimePoint&)>& handler)
{
    std::lock_guard<componet::Spinlock> lock(m_lock);

    auto& info = m_eventHandlers[interval];
    auto eventId = info.event.reg(handler);
    info.regTime = TheClock::now();
    info.counter = 0;
    return {interval, eventId};
}

void Timer::unregEventHandler(RegID id)
{
    std::lock_guard<componet::Spinlock> lock(m_lock);

    auto it = m_eventHandlers.find(id.first);
    if(it == m_eventHandlers.end())
        return;

    it->second.event.unreg(id.second);
}

}}
