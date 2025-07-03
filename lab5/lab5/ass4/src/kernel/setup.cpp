#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"

// 屏幕IO处理器
STDIO stdio;
// 中断管理器
InterruptManager interruptManager;
// 程序管理器
ProgramManager programManager;

void busy_wait(int count) {
    for (volatile int i = 0; i < count; ++i) {
        asm volatile ("nop");
    }
}

void thread_short(void *arg) {
    printPCBInfo(programManager.running);
    printf("[Short Thread] Start! pid=%d\n", programManager.running->pid);
    busy_wait(10000); // 短时间忙碌
    printf("[Short Thread] Done! pid=%d\n", programManager.running->pid);
}

void thread_medium(void *arg) {
    printPCBInfo(programManager.running);
    printf("[Medium Thread] Start! pid=%d\n", programManager.running->pid);
    busy_wait(50000); // 中等时间忙碌
    printf("[Medium Thread] Done! pid=%d\n", programManager.running->pid);
}

void thread_long(void *arg) {
    printPCBInfo(programManager.running);
    printf("[Long Thread] Start! pid=%d\n", programManager.running->pid);
    busy_wait(100000); // 长时间忙碌
    printf("[Long Thread] Done! pid=%d\n", programManager.running->pid);
}

extern "C" void setup_kernel()
{
    interruptManager.initialize();
    interruptManager.enableTimeInterrupt();
    interruptManager.setTimeInterrupt((void *)asm_time_interrupt_handler);

    stdio.initialize();
    programManager.initialize();

    // 创建不同“长度”的线程
    int pid1 = programManager.executeThread(thread_long, nullptr, "long thread", 3);
    int pid2 = programManager.executeThread(thread_short, nullptr, "short thread", 1);
    int pid3 = programManager.executeThread(thread_medium, nullptr, "medium thread", 2);

    if (pid1 == -1 || pid2 == -1 || pid3 == -1) {
        printf("cannot execute threads\n");
        asm_halt();
    }

    // 手动选一个线程开始跑（调度器随后会接管）
    ListItem *item = programManager.readyPrograms.front();
    PCB *firstThread = ListItem2PCB(item, tagInGeneralList);

    programManager.readyPrograms.pop_front();
    firstThread->status = RUNNING;
    programManager.running = firstThread;

    asm_switch_thread(nullptr, firstThread);

    asm_halt();
}

