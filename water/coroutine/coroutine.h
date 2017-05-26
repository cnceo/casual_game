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

    void run(CorotId coid);

    namespace this_corot
    {
        CorotId getId();
        void yield();
//        void resume(CorotId coid);
    };


//    extern thread_local Scheduler m_scheduler;
};

