#include "corotn.h"

int g = 10;
bool running = true;

void c();

corot::CorotId cop;
corot::CorotId coc;

void p()
{
    while(running)
    {
        if(g < 10)
        {
            g = g + 1;
            cout << "tid:" << corot::this_corot::getId() << ", produce 1 goods, total=" << g << endl;
        }
        //corot::this_corot::yield();
        corot::resume(coc);
    }
}

void c()
{
    while(running)
    {
        if(g > 0)
        {
            g = g - 1;
            cout << "tid:" << corot::this_corot::getId() << ", consume 1 goods, total=" << g << endl;
        }
        corot::this_corot::yield();
    //    corot::resume(cop);
    }
}



int main()
{
    cop = corot::create(p);
    coc = corot::create(c);
    corot::run(cop);
/*
    for(int i = 0; i < 100000; ++i)
        corot::create(p);

    int j;
    std::cin >> j;
*/
    while(corot::doSchedule())
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
