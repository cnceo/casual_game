#include <stddef.h>
#include <ucontext.h>


class Corotn
{
    static const uint32_t STACK_SIZE = 1024 * 1024 * 10; //10mb
    static const TaskId MAIN_TASK_ID = -1;
    
    using TaskId = uint32_t;

    class enum TaskStatus
    {
        null,
        ready,
        running,
        sleeping,
    };

    struct Task
    {
        std::functional<void ()> exec;
        ucontext_t ctx;
        std::vector<uint8_t> stackData;
        TaskStatus status = TaskStatus::ready;
    };

    struct Scheduler
    {
        std::array<uint8_t, STACK_SIZE> stack;
        std::vector<Task*> tasks;
        TaskId curTaskId = 0;
        std::set<TaskId> avaliableId; //当前已经回收的taskId
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
        TaskId tid = s_me.m_scheduler.curTaskId;
        if (tid == MAIN_TASK_ID)
            return nullptr;
        if (tid >= s_me.m_scheduler.tasks.size())
            return nullptr;
        return s_me.m_scheduler.tasks[tid];
    }

    template <typename RetType, typename ParamsType ...>
    static create(std::function<RetType (ParamsType...), ParamsType...) //done
    {
    }

    static create(std::function<void(void)> func)
    {
        auto& scheduler = s_me.m_scheduler;
        Task* t = new Task();
        t->exec = func;
        scheduler.tasks.push_back(t);
        return scheduler.tasks.size();
    }

private:
    static void invoke()
    {
        t->exe();


        
    }

    static bool resume(TaskId tid)
    {
        auto& scheduler = s_me.m_scheduler;
        auto& taskCtx = t->ctx;
        auto& schedulerCtx = &scheduler.ctx;
        if (tid >= scheduler.size())
            return true;
        Task* t = scheduler.tasks[tid];
        if (t == nullptr)
            return true;

//        auto& scheduler = s_me.m_scheduler;
        switch (t->status)
        {
        case TaskStatus::ready:
            if (-1 == getcontext(&taskCtx))//TODO 确认下一下，是否这个t->ctx必须这样初始化， 是否可以直接清0
                return false;
            taskCtx.uc_stack.ss_sp = scheduler.stack.data();
            taskCtx.uc_stack.ss_size = scheduler.stack.size();
            taskCtx.uc_linc = &schedulerCtx; //ptr for current context
            makecontext(&taskCtx, &Corotn::invoke);
            scheduler->curTaskId = tid;
            t->status = TaskStatus::running;
            if (-1 == swapcontext(&taskCtx, &schedulerCtx))
                return false;
            break; //当task 调用yield()时，会走到这里
        case TaskStatus::sleeping:
            memcpy(scheduler.stack.data() + scheduler.stack.size() - t.stackData.size(),  //TODO*
        default:
            break;
        }
        return true;
    }


private:
    Scheduler m_scheduler;
    static thread_local Corotn s_me;
};

thread_local Corotn::Scheduler Corotn::scheduler;

int main()
{

    return 0;
}
