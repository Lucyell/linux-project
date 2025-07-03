#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"
#include "sync.h"

// 屏幕IO处理器
STDIO stdio;
// 中断管理器
InterruptManager interruptManager;
// 程序管理器
ProgramManager programManager;

Semaphore ch0;
Semaphore ch1;
Semaphore ch2;
Semaphore ch3;
Semaphore ch4;
bool chops[5] = {true,true,true,true,true};
int cheese_burger;

void delay()
{
    for(int i = 0;i<100000000;i++);
}

void think()
{
    for(int i = 0;i<1000000;i++);
    printf("thinking\n");
}

void eat()
{
    for(int i = 0;i<1000000;i++);
}

void first_ph(void * arg)
{
    while (1)
    {
        think();
        while (1)
        {
            ch0.P();
            if (ch4.getValue() > 0)
            {
                ch4.P();
                break;
            }
            else
            {
                ch0.V();
                delay();
            }
        }
        printf("first begins to eat\n");
        eat();
        ch0.V();
        ch4.V();
    }
}

void second_ph(void * arg)
{
    while (1)
    {
        think();
        while (1)
        {
            ch1.P();
            {
                ch0.P();
                break;
            }
            else
            {
                ch1.V();
                delay();
            }
        }
        printf("second begins to eat\n");
        eat();
        ch1.V();
        ch0.V();
    }
}

void third_ph(void * arg)
{
    while (1)
    {
        think();
        while (1)
        {
            ch2.P();
            if (ch1.getValue() > 0)
            {
                ch1.P();
                break;
            }
            else
            {
                ch2.V();
                delay();
            }
        }
        printf("third begins to eat\n");
        eat();
        ch2.V();
        ch1.V();
    }
}

void forth_ph(void * arg)
{
    while (1)
    {
        think();
        while (1)
        {
            ch3.P();
            if (ch2.getValue() > 0)
            {
                ch2.P();
                break;
            }
            else
            {
                ch3.V();
                delay();
            }
        }
        printf("forth begins to eat\n");
        eat();
        ch3.V();
        ch2.V();
    }
}

void five_ph(void * arg)
{
    while (1)
    {
        think();
        while (1)
        {
            ch4.P();
            if (ch3.getValue() > 0)
            {
                ch3.P();
                break;
            }
            else
            {
                ch4.V();
                delay();
            }
        }
        printf("five begins to eat\n");
        eat();
        ch4.V();
        ch3.V();
    }
}




void first_thread(void *arg)
{
    stdio.moveCursor(0);
    for (int i = 0; i < 25 * 80; ++i)
    {
        stdio.print(' ');
    }
    stdio.moveCursor(0);

    cheese_burger = 0;
    ch0.initialize(1);
    ch2.initialize(1);
    ch3.initialize(1);
    ch4.initialize(1);
    ch1.initialize(1);

    programManager.executeThread(first_ph, nullptr, "second thread", 1);
    programManager.executeThread(second_ph, nullptr, "third thread", 1);
    programManager.executeThread(third_ph, nullptr, "forth thread", 1);
    programManager.executeThread(forth_ph, nullptr, "fifth thread", 1);
    programManager.executeThread(five_ph, nullptr, "first thread", 1);

    asm_halt();
}

extern "C" void setup_kernel()
{

    // 中断管理器
    interruptManager.initialize();
    interruptManager.enableTimeInterrupt();
    interruptManager.setTimeInterrupt((void *)asm_time_interrupt_handler);

    // 输出管理器
    stdio.initialize();

    // 进程/线程管理器
    programManager.initialize();

    // 创建第一个线程
    int pid = programManager.executeThread(first_thread, nullptr, "first thread", 1);
    if (pid == -1)
    {
        printf("can not execute thread\n");
        asm_halt();
    }

    ListItem *item = programManager.readyPrograms.front();
    PCB *firstThread = ListItem2PCB(item, tagInGeneralList);
    firstThread->status = RUNNING;
    programManager.readyPrograms.pop_front();
    programManager.running = firstThread;
    asm_switch_thread(0, firstThread);

    asm_halt();
}

