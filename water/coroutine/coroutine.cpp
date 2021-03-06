#include "coroutine.h"

#include <stddef.h>
#include <string.h>
#include <functional>
#include <vector>
#include <list>
#include <ucontext.h>
#include <assert.h>

#include <iostream>

using std::cout;
using std::endl;

namespace corot
{
    const uint32_t STACK_SIZE = 1024 * 1024 * 10; //10mb
    const CorotId SCHEDULE_TASK_ID = -1;

    enum class CorotStatus
    {
        null,
        ready,
        running,
        sleeping,
    };
    struct Corot
    {
        friend class Scheduler;
        CorotId id = SCHEDULE_TASK_ID;
        //        bool isDetached = false;
        std::function<void ()> exec;
        ucontext_t ctx;
        std::vector<uint8_t> stackData;
        CorotStatus status = CorotStatus::null;
    };

    class Scheduler
    {
    public:
        ~Scheduler()
        {
            for(auto& co : tasks)
            {
                if(co == nullptr)
                    continue;
                delete co;
                co = nullptr;
            }
        }

        inline CorotId create(const std::function<void(void)>& func)
        {
            Corot* co = new Corot();
            co->exec = func;

            if (this->avaliableIds.empty())
            {
                this->tasks.push_back(co);
                co->id = this->tasks.size() - 1;
            }
            else
            {
                co->id = this->avaliableIds.front();
                this->avaliableIds.pop_front();
                this->tasks[co->id] = co;
            }
            co->status = CorotStatus::ready;
            return co->id;
        }

        inline void destroy(CorotId coid)
        {
            delete this->tasks[coid];
            this->tasks[coid] = nullptr;
            this->avaliableIds.push_back(coid);
        }

        inline void destroy(Corot* co)
        {
            CorotId coid = co->id;
            delete this->tasks[coid];
            this->tasks[coid] = nullptr;
            this->avaliableIds.push_back(coid);
        }

        inline bool resumeCorot(CorotId coid)
        {
            Corot* co = getCorot(coid);
            return resumeCorot(co);
        }

        inline bool resumeCorot(Corot* co)
        {
            if (co == nullptr)
                return true;

            CorotId coid = co->id;
            auto& taskCtx = co->ctx;
            auto& schedulerCtx = this->ctx;

            Corot* runningCorot = getRunningCorot();
            if (runningCorot != nullptr)
                runningCorot->status = CorotStatus::sleeping;

            switch (co->status)
            {
            case CorotStatus::ready:
                {
                    if (-1 == getcontext(&taskCtx)) //构造一个ctx, ucontext函数族要求这样来初始化一个供makecontext函数使用的ctx
                        return false;
                    taskCtx.uc_stack.ss_sp = this->stack.data();   //这里使用scheduler持有的buf作为task的执行堆栈, 每个task都用这个栈来执行
                    taskCtx.uc_stack.ss_size = this->stack.size();
                    taskCtx.uc_link = &schedulerCtx; //taskCtx的继承者ctx的存储地址, 这个ctx中现在还是无效数据，将在下面调用swapcontext时将其填充为当前上下文
                    makecontext(&taskCtx, &Scheduler::invoke, 0); //此函数要求本行必须在以上三行之后调用
                    this->runningCoid = coid;
                    co->status = CorotStatus::running;
                    if (-1 == swapcontext(&schedulerCtx, &taskCtx)) //切换上下文到taskCtx，并把当前的ctx保存在schedulerCtx中, 实现longjump
                        return false;
                }
                break; //当task 调用yield()时，会走到这里
            case CorotStatus::sleeping:
                {
                    uint8_t* stackBottom = this->stack.data() + this->stack.size();
                    uint8_t* stackTop = stackBottom - co->stackData.size();
                    memcpy(stackTop, co->stackData.data(), co->stackData.size()); /*恢复堆栈数据到运行堆栈, 这里恢复后不调用vector::clear(), 
                                                                                    因为clear()后只能节省最大一个STACK_SIZE的空间，与每次切换都重新内存的开销对比性价比太低
                                                                                   */
                    //cout << "coro resume, restore stack, stack size=" << co->stackData.size() << endl;
                    this->runningCoid = coid;
                    co->status = CorotStatus::running;
                    if (-1 == swapcontext(&schedulerCtx, &taskCtx)) //切换上下文到taskCtx，并把当前的ctx保存在schedulerCtx中, 实现longjump
                        return false;
                }
                break; //当task 调用yield()时，会走到这里
            case CorotStatus::null:
                break;
            default:
                break;
            }
            return true;
        }

        inline void yieldRunningCorot()
        {
            Corot* co = this->getRunningCorot();
            if (co == nullptr)
                return;
            assert(static_cast<void*>(&co) > static_cast<void*>(this->stack.data())); //栈溢出检查
            /*
               下面计算当前的堆栈使用情况，并保存当前的栈。注意：
               这里的stackTop不是真正的栈顶，不包含比stackBottom变量所在的内存更高地址上的栈空间。
               同时由于现代编译器默认启用栈溢出保护(对于gcc，叫stack-protector)的特性，会调整局部变量的压栈顺序，
               所以具体哪些局部变量在比&stackBottom更高的栈上，也不一定。
               所以堆栈恢复后，不能再访问此函数内部的局部变量。简单的讲，就是这里保存的栈内存，对于当前函数而言不完整。
               但是此函数最后一行是swapcontext(), 这就保证了下次被其它地方的swapcontext切换回来后，本函数直接就要return退出了，
               本函数使用的栈内存也就不再有用了，所以本函自身的堆栈保存残缺不影响正常执行
             */
            uint8_t* stackBottom = this->stack.data() + this->stack.size(); //整个栈空间的栈底指针
            uint8_t* stackTop = reinterpret_cast<uint8_t*>(&stackBottom);    
            ptrdiff_t stackSize = stackBottom - stackTop;
            co->stackData.resize(stackSize);
            memcpy(co->stackData.data(), stackTop, stackSize);
            co->status = CorotStatus::sleeping;
            this->runningCoid = SCHEDULE_TASK_ID;
            //cout << "coro yield, save stack, stack size=" << stackSize << endl;
            if (-1 == swapcontext(&(co->ctx), &(this->ctx))) //此行执行完必须return
                //cout << "coro yield, swapcontext failed" << endl; //TODO log
            return;
        }

        inline uint32_t doSchedule()
        {
            if (this->runningCoid != SCHEDULE_TASK_ID)
            {
                //cout << "error, scheduler is not running is main task" << endl;
                return 0;
            }
            uint32_t ret = 0;
            for (CorotId coid = 0; coid < this->tasks.size(); ++coid)
            {
                Corot* co = this->tasks[coid];
                if (co == nullptr)
                    continue;
                /*
                   if (co->isDetached)
                   continue;
                 */
                if(!resumeCorot(coid))
                {
                    //cout << "resume failed, coid=" << coid << endl;
                    destroy(coid);
                    continue;
                }
                ret++;
            }
            return ret;
        }

        inline CorotId getRunningCorotId() const
        {
            return runningCoid;
        }

        inline Corot* getRunningCorot() //done
        {
            CorotId coid = this->runningCoid;
            return getCorot(coid);
        }

        Corot* getCorot(CorotId coid)
        {
            if (coid == SCHEDULE_TASK_ID)
                return nullptr;

            if (coid >= this->tasks.size())
                return nullptr;
            return this->tasks[coid];
        }

    private:
        static void invoke()
        {
            CorotId coid = s_me.runningCoid;
            Corot* co = s_me.getCorot(coid);
            co->exec();

            //co->exe()没有调用yield()而调用return才会走到这里
            //cout << "task exit, coid=" << coid << endl;
            s_me.destroy(coid); //协程执行完毕
            s_me.runningCoid = SCHEDULE_TASK_ID; //控制权即将返回调度器
        }


    private:
        std::array<uint8_t, STACK_SIZE> stack;
        std::vector<Corot*> tasks;
        CorotId runningCoid = SCHEDULE_TASK_ID;
        std::list<CorotId> avaliableIds; //当前已经回收的taskId
        ucontext_t ctx;

        static thread_local Scheduler s_me;
    public:
        static Scheduler& me()
        {
            return s_me;
        }
    };
    thread_local Scheduler Scheduler::s_me;

    uint32_t doSchedule()
    {
        return Scheduler::me().doSchedule();
    }


    CorotId create(const std::function<void(void)>& func)
    {
        return Scheduler::me().create(std::forward<const std::function<void(void)>&>(func));
    }
    /*
       void destroy(CorotId coid);
       void destroy(Corot* co);
     */

    void run(CorotId coid)
    {
        Corot* co = Scheduler::me().getCorot(coid);
        co->exec();
    }

    namespace this_corot
    {
        CorotId getId()
        {
            return Scheduler::me().getRunningCorotId();
        }

        void yield()
        {
            Scheduler::me().yieldRunningCorot();
        }
        void resume(CorotId coid)
        {
            Scheduler::me().resumeCorot(coid);
        }
        void resume(Corot* co)
        {
            Scheduler::me().resumeCorot(co);
        }
    }


};

