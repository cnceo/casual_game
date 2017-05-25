#include <iostream>
#include <vector>

namespace water{
namespace componet{


class Corotn
{

public:
    void schedule();

    static bool yield();
    //static bool yield(Task* other);
    static Task* thisTask();
private:
    using TaskId = std::vector<Task*>::size_type;
    class Task
    {
        uint8_t* stackData;
    };

    Corotn() = default;
    ~Corotn() = default;
    


    private

private:
    static bool invokeTask();
    static bool resume(Task*);

    bool getCtx(Task*);
    bool makeCtx(Task*);
    bool swapCtx(Task*);

private:
    std::vector<Task*> tasks;
    uint8_t* stack;
    std::list<TaskId> taskId;
    TaskId currentTaskId;
   
    static thread_local Corotn t_me;
};


}}
