#include <stddef.h>
#include <string.h>
#include <functional>
#include <vector>
#include <list>
#include <ucontext.h>
#include <assert.h>

#include <thread>
#include <iostream>
using std::cout;
using std::endl;


class Corotn
{
    using TaskId = uint32_t;

    static const uint32_t STACK_SIZE = 1024 * 1024 * 10; //10mb
    static const TaskId SCHEDULE_TASK_ID = -1;
    

    enum class TaskStatus
    {
        null,
        ready,
        running,
        sleeping,
    };

    struct Task
    {
 /*       void start()
        {
            status = TaskStatus::ready;
        }
*/
        std::function<void ()> exec;
        ucontext_t ctx;
        std::vector<uint8_t> stackData;
        TaskStatus status = TaskStatus::null;
    };

    struct Scheduler
    {
        std::array<uint8_t, STACK_SIZE> stack;
        std::vector<Task*> tasks;
        TaskId runningTid = SCHEDULE_TASK_ID;
        std::list<TaskId> avaliableIds; //当前已经回收的taskId
        ucontext_t ctx;
    };

    Corotn()
    {
    }
    ~Corotn() //done
    {
        for(auto& t : m_scheduler.tasks)
        {
            if(t == nullptr)
                continue;
            delete t;
            t = nullptr;
        }
    }

public:
    static Task* curTask() //done
    {
        auto& scheduler = m_scheduler;
        TaskId tid = scheduler.runningTid;
        return getTask(tid);
    }

    static Task* getTask(TaskId tid)
    {
        if (tid == SCHEDULE_TASK_ID)
            return nullptr;

        auto& scheduler = m_scheduler;
        if (tid >= scheduler.tasks.size())
            return nullptr;
        return scheduler.tasks[tid];
    }

    static TaskId launch(std::function<void(void)> func)
    {
        TaskId tid = create(func);
        Task* t = getTask(tid);
        t->status = TaskStatus::ready;
    }

    static TaskId create(std::function<void(void)> func)
    {
        auto& scheduler = m_scheduler;
        Task* t = new Task();
        t->exec = func;

        if (scheduler.avaliableIds.empty())
        {
            scheduler.tasks.push_back(t);
            return scheduler.tasks.size() - 1;
        }
        else
        {
            TaskId tid = scheduler.avaliableIds.front();
            scheduler.avaliableIds.pop_front();
            scheduler.tasks[tid] = t;
            return tid;
        }
        return SCHEDULE_TASK_ID;
    }

    static void destroy(TaskId tid)
    {
        auto& scheduler = m_scheduler;
        delete scheduler.tasks[tid];
        scheduler.tasks[tid] = nullptr;
        scheduler.avaliableIds.push_back(tid);
    }

    static void invoke()
    {
        auto& scheduler = m_scheduler;
        TaskId tid = scheduler.runningTid;
        Task* t = getTask(tid);
        t->exec();

        //t->exe()没有调用yield()而调用return才会走到这里
        cout << "task exit, tid=" << tid << endl;
        destroy(tid); //协程执行完毕
        scheduler.runningTid = SCHEDULE_TASK_ID; //控制权即将返回调度器
    }

    static bool resume(TaskId tid)
    {
        auto& scheduler = m_scheduler;
        if (tid >= scheduler.tasks.size())
            return true;
        Task* t = scheduler.tasks[tid];
        if (t == nullptr)
            return true;

        auto& taskCtx = t->ctx;
        auto& schedulerCtx = scheduler.ctx;

//        auto& scheduler = m_scheduler;
        switch (t->status)
        {
        case TaskStatus::ready:
            {
                if (-1 == getcontext(&taskCtx)) //构造一个ctx, ucontext函数族要求这样来初始化一个供makecontext函数使用的ctx
                    return false;
                taskCtx.uc_stack.ss_sp = scheduler.stack.data();   //这里使用scheduler持有的buf作为task的执行堆栈, 每个task都用这个栈来执行
                taskCtx.uc_stack.ss_size = scheduler.stack.size();
                taskCtx.uc_link = &schedulerCtx; //taskCtx的继承者ctx的存储地址, 这个ctx中现在还是无效数据，将在下面调用swapcontext时将其填充为当前上下文
                makecontext(&taskCtx, &Corotn::invoke, 0); //此函数要求本行必须在以上三行之后调用
                scheduler.runningTid = tid;
                t->status = TaskStatus::running;
                if (-1 == swapcontext(&schedulerCtx, &taskCtx)) //切换上下文，调用成功的话，该函数不会return，而是把当前的ctx保存在第二参数中，把第一参数设定为活动上下文并继续执行
                    return false;
            }
            break; //当task 调用yield()或return时，schedulerCtx被swap或自动载入cpu, 然后就会走到这里
        case TaskStatus::sleeping:
            {
                uint8_t* stackBottom = scheduler.stack.data() + scheduler.stack.size();
                uint8_t* stackTop = stackBottom - t->stackData.size();
                memcpy(stackTop, t->stackData.data(), t->stackData.size()); /*恢复堆栈数据到运行堆栈, 这里恢复后不调用vector::clear(), 
                                                                              因为clear()后只能节省最大一个STACK_SIZE的空间，与每次切换都重新内存的开销对比性价比太低
                                                                              */
                scheduler.runningTid = tid;
                t->status = TaskStatus::running;
                if (-1 == swapcontext(&schedulerCtx, &taskCtx)) //切换上下文，调用成功的话，该函数不会return，而是把当前的ctx保存在第二参数中，把第一参数设定为活动上下文并继续执行
                    return false;
            }
            break; //当task 调用yield()或return时，schedulerCtx被swap或自动载入cpu, 然后就会走到这里
        case TaskStatus::null:
            break;
        default:
            break;
        }
        return true;
    }

    static bool yield()
    {
        auto& scheduler = m_scheduler;
        TaskId tid = scheduler.runningTid;
        Task* t = scheduler.tasks[tid];  //task运行期间如果runningTid一定是当前调用者自己的id
        assert(static_cast<void*>(&t) > static_cast<void*>(scheduler.stack.data())); //栈溢出检查
        /*
          下面计算当前的堆栈使用情况，并保存当前的栈注意：
          这里的stackTop不是真正的栈顶，不包含比&stackBottom变量所在的内存更高地址上的栈空间。
          同时由于现代编译器默认启用栈溢出保护(对于gcc，叫stack-protector)的特性，会调整局部变量的压栈顺序，
          所以具体哪些局部变量在比&stackBottom更高的栈上，也不一定。
          所以堆栈恢复后，不能再访问此函数内部的局部变量。简单的讲，就是这里保存的栈内存，对于当前函数而言不完整。
          但是此函数最后一行是swapcontext(), 这就保证了下次被其它地方的swapcontext切换回来后，本函数直接就要return退出了，
          本函数使用的栈内存也就不再有用了，所以本函自身的堆栈保存残缺不影响正常执行
          */
        uint8_t* stackBottom = scheduler.stack.data() + scheduler.stack.size(); //整个栈空间的栈底指针
        uint8_t* stackTop = reinterpret_cast<uint8_t*>(&stackBottom);    
        ptrdiff_t stackSize = stackBottom - stackTop;
        t->stackData.resize(stackSize);
        memcpy(t->stackData.data(), stackTop, stackSize);
        t->status = TaskStatus::sleeping;
        scheduler.runningTid = SCHEDULE_TASK_ID;
        if (-1 == swapcontext(&(t->ctx), &(scheduler.ctx))) //此行执行完必须return
            return false;
        return true;
    }

    static uint32_t doSchedule()
    {
        if (m_scheduler.runningTid != SCHEDULE_TASK_ID)
        {
            cout << "error, scheduler is not running is main task" << endl;
            return 0;
        }
        uint32_t ret = 0;
        for (TaskId tid = 0; tid < m_scheduler.tasks.size(); ++tid)
        {
            Task* t = m_scheduler.tasks[tid];
            if (t == nullptr)
               continue;
            if(!resume(tid))
            {
                cout << "resume failed, tid=" << tid << endl;
                destroy(tid);
                continue;
            }
            ret++;
        }
        return ret;
    }

    struct ThisTask
    {
        static TaskId tid()
        {
            return m_scheduler.runningTid;
        }
    };



private:
    static thread_local Scheduler m_scheduler;
};

thread_local Corotn::Scheduler Corotn::m_scheduler;

int g = 10;
bool running = true;

void p()
{
    while(running)
    {
        if(g < 10)
        {
            g = g + 1;
            cout << "tid:" << Corotn::ThisTask::tid() << ", produce 1 goods, total=" << g << endl;
        }
        Corotn::yield();
    }
}

void c()
{
    while(running)
    {
        if(g > 0)
        {
            g = g - 1;
            cout << "tid:" << Corotn::ThisTask::tid() << ", consume 1 goods, total=" << g << endl;
        }
        Corotn::yield();
    }
}

int main()
{
    Corotn::launch(p);
    Corotn::launch(c);
    Corotn::launch(c);

    while(Corotn::doSchedule())
    {
        if(g == 0)
        {
            running = false;
            continue;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return 0;
}

