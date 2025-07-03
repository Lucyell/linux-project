#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"
#include "sync.h"

STDIO stdio;
InterruptManager interruptManager;
ProgramManager programManager;

extern "C" void start_philosophers_thread(void*);

extern "C" void setup_kernel() {
    interruptManager.initialize();
    interruptManager.enableTimeInterrupt();
    interruptManager.setTimeInterrupt((void *)asm_time_interrupt_handler);

    stdio.initialize();
    programManager.initialize();

    programManager.executeThread([](void*) {
        start_philosophers_thread(nullptr);
        while (true); // idle loop
    }, nullptr, "idle", 1);

    ListItem *item = programManager.readyPrograms.front();
    PCB *firstThread = ListItem2PCB(item, tagInGeneralList);
    firstThread->status = RUNNING;
    programManager.readyPrograms.pop_front();
    programManager.running = firstThread;
    asm_switch_thread(0, firstThread);
}


