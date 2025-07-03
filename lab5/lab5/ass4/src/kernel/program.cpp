#include "program.h"
#include "stdlib.h"
#include "interrupt.h"
#include "asm_utils.h"
#include "stdio.h"
#include "thread.h"
#include "os_modules.h"

const int PCB_SIZE = 4096;
char PCB_SET[PCB_SIZE * MAX_PROGRAM_AMOUNT];
bool PCB_SET_STATUS[MAX_PROGRAM_AMOUNT];


void printPCBInfo(PCB *pcb);
const char* getStatusName(ProgramStatus status);

ProgramManager::ProgramManager()
{
    initialize();
}

void ProgramManager::initialize()
{
    allPrograms.initialize();
    readyPrograms.initialize();  // 新加：初始化readyPrograms单链表
    running = nullptr;

    for (int i = 0; i < MAX_PROGRAM_AMOUNT; ++i) {
        PCB_SET_STATUS[i] = false;
    }
}

int ProgramManager::executeThread(ThreadFunction function, void *parameter, const char *name, int priority)
{
    bool status = interruptManager.getInterruptStatus();
    interruptManager.disableInterrupt();

    PCB *thread = allocatePCB();
    if (!thread) {
        interruptManager.setInterruptStatus(status);
        return -1;
    }

    ::memset(thread, (char)0, PCB_SIZE);

    for (int i = 0; i < MAX_PROGRAM_NAME - 1 && name[i]; ++i) {
        thread->name[i] = name[i];
    }

    thread->status = ProgramStatus::READY;
    thread->priority = priority;  // SJF中，priority代表“执行时间长短”，值越小优先级越高
    thread->ticksPassedBy = 0;
    thread->parentPid = running ? running->pid : -1;
    thread->isFinished = 0;
    thread->pid = ((int)thread - (int)PCB_SET) / PCB_SIZE;

     // 线程栈
    thread->stack = (int *)((int)thread + PCB_SIZE);
    thread->stack -= 7;
    thread->stack[0] = 0;
    thread->stack[1] = 0;
    thread->stack[2] = 0;
    thread->stack[3] = 0;
    thread->stack[4] = (int)function;
    thread->stack[5] = (int)program_exit;
    thread->stack[6] = (int)parameter;



    allPrograms.push_back(&(thread->tagInAllList));
    readyPrograms.push_back(&(thread->tagInGeneralList));  // 新加：进readyPrograms链表

    interruptManager.setInterruptStatus(status);
    return thread->pid;
}

void ProgramManager::schedule()
{
    bool status = interruptManager.getInterruptStatus();
    interruptManager.disableInterrupt();

    // -- 选择最短作业 --
    if (readyPrograms.empty()) {
        interruptManager.setInterruptStatus(status);
        return;
    }

    ListItem* bestItem = readyPrograms.front();
    PCB* bestPCB = ListItem2PCB(bestItem, tagInGeneralList);

    for (ListItem* item = readyPrograms.front(); item != nullptr; item = item->next) {
        PCB* pcb = ListItem2PCB(item, tagInGeneralList);
        if (pcb->priority < bestPCB->priority) {  // priority小的优先
            bestItem = item;
            bestPCB = pcb;
        }
    }

    // 把最优的线程从就绪队列里拿出来
    if (readyPrograms.front() == bestItem) {
        readyPrograms.pop_front();
    } else {
        ListItem* prev = readyPrograms.front();
        for (ListItem* cur = prev->next; cur != nullptr; prev = cur, cur = cur->next) {
            if (cur == bestItem) {
                prev->next = cur->next;
                break;
            }
        }
    }

    PCB* cur = running;
    if (cur) {
        if (cur->status == ProgramStatus::RUNNING) {
            cur->status = ProgramStatus::READY;
            readyPrograms.push_back(&(cur->tagInGeneralList));  // 重新放回队列
        } else if (cur->status == ProgramStatus::DEAD) {
            releasePCB(cur);
        }
    }

    bestPCB->status = ProgramStatus::RUNNING;
    running = bestPCB;
    asm_switch_thread(cur, bestPCB);

    printPCBInfo(bestPCB);
    interruptManager.setInterruptStatus(status);
}

void program_exit()
{
    PCB *thread = programManager.running;
    thread->status = ProgramStatus::DEAD;

    if (thread->pid) {
        programManager.schedule();
    } else {
        interruptManager.disableInterrupt();
        printf("halt\n");
        asm_halt();
    }
}

PCB *ProgramManager::allocatePCB()
{
    for (int i = 0; i < MAX_PROGRAM_AMOUNT; ++i) {
        if (!PCB_SET_STATUS[i]) {
            PCB_SET_STATUS[i] = true;
            return (PCB*)((int)PCB_SET + PCB_SIZE * i);
        }
    }
    return nullptr;
}

void ProgramManager::releasePCB(PCB *program)
{
    int index = ((int)program - (int)PCB_SET) / PCB_SIZE;
    PCB_SET_STATUS[index] = false;
}

const char* getStatusName(ProgramStatus status) {
    switch (status) {
        case CREATED: return "CREATED";
        case RUNNING: return "RUNNING";
        case READY:   return "READY";
        case BLOCKED: return "BLOCKED";
        case DEAD:    return "DEAD";
        default:      return "UNKNOWN";
    }
}

void printPCBInfo(PCB *thread) {
    printf("=== PCB Info ===\n");
    printf("Name: %s\n", thread->name);
    printf("Status: %s\n", getStatusName(thread->status));
    printf("Priority: %d\n", thread->priority);  // SJF关键字段
    printf("Parent PID: %d\n", thread->parentPid);
    printf("Is Finished: %d\n", thread->isFinished);
    printf("===============\n");
}

