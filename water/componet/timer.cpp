#include "timer.h"

#include <thread>
//#include <mutex>

namespace water{
namespace componet{

using LockGuard = std::lock_guard<componet::Spinlock>;

Timer::Timer()
//:m_wakeUp(EPOCH)
{
}

void Timer::tick()
{

    TimePoint now = TheClock::now();
    TimePoint nearestWakeUp = EPOCH;
    {
        LockGuard lock(m_lock);
        if(!m_eventHandlers.empty())
        {
            for(auto& pair : m_eventHandlers)
            {
                TheClock::time_point wakeUp = pair.second.lastEmitTime + pair.first;

                if(now >= wakeUp) //执行一次定时事件
                {
                    pair.second.lastEmitTime = now;
                    pair.second.event(now);
                    wakeUp = pair.second.lastEmitTime + pair.first;
                    now = TheClock::now(); //触发了定时事件, 可能耗时较长, 需要重新获取当前时间
                }
                if(nearestWakeUp == EPOCH || wakeUp < nearestWakeUp)
                    nearestWakeUp = wakeUp;
            }
        }
    }
    if(nearestWakeUp > now)
        std::this_thread::sleep_until(nearestWakeUp);
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
    info.lastEmitTime = EPOCH;
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
