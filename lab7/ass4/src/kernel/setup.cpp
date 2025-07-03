#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"
#include "sync.h"
#include "memory.h"

// 屏幕IO处理器
STDIO stdio;
// 中断管理器
InterruptManager interruptManager;
// 程序管理器
ProgramManager programManager;
// 内存管理器
MemoryManager memoryManager;

void first_thread(void *arg)
{
    // 第1个线程不可以返回
    // stdio.moveCursor(0);
    // for (int i = 0; i < 25 * 80; ++i)
    // {
    //     stdio.print(' ');
    // }
    // stdio.moveCursor(0);

    char *p1 = (char *)memoryManager.allocatePages(AddressPoolType::KERNEL, 100);
    char *p2 = (char *)memoryManager.allocatePages(AddressPoolType::KERNEL, 10);
    char *p3 = (char *)memoryManager.allocatePages(AddressPoolType::KERNEL, 100);

    printf("%x %x %x\n", p1, p2, p3);

    memoryManager.releasePages(AddressPoolType::KERNEL, (int)p2, 10);
    p2 = (char *)memoryManager.allocatePages(AddressPoolType::KERNEL, 100);

    printf("%x\n", p2);

    p2 = (char *)memoryManager.allocatePages(AddressPoolType::KERNEL, 10);
    
    printf("%x\n", p2);
    
    printf("New Test\n");

    // 分配三组虚拟页
    int vaddr1 = memoryManager.allocatePages(AddressPoolType::KERNEL, 3);
    int vaddr2 = memoryManager.allocatePages(AddressPoolType::KERNEL, 5);
    int vaddr3 = memoryManager.allocatePages(AddressPoolType::KERNEL, 1);

    printf("Allocated vaddr1: 0x%x\n", vaddr1);
    printf("Allocated vaddr2: 0x%x\n", vaddr2);
    printf("Allocated vaddr3: 0x%x\n", vaddr3);

    // 检查是否返回合理地址（非0、对齐、无重叠）
    if (vaddr1 == 0 || vaddr2 == 0 || vaddr3 == 0)
        printf("ERROR: allocation returned 0\n");

    if ((vaddr1 < vaddr2 && vaddr1 + 3 * PAGE_SIZE > vaddr2) ||
        (vaddr2 < vaddr3 && vaddr2 + 5 * PAGE_SIZE > vaddr3) ||
        (vaddr3 < vaddr1 && vaddr3 + PAGE_SIZE > vaddr1))
    {
        printf("ERROR: overlapping allocations\n");
    }

    // 尝试释放
    memoryManager.releasePages(AddressPoolType::KERNEL, vaddr1, 3);
    memoryManager.releasePages(AddressPoolType::KERNEL, vaddr2, 5);
    memoryManager.releasePages(AddressPoolType::KERNEL, vaddr3, 1);
    printf("Released all allocated virtual pages.\n");

    // 再次分配同样数量页，看是否地址回收（是否相同地址）
    int vaddr4 = memoryManager.allocatePages(AddressPoolType::KERNEL, 3);
    printf("Reallocated vaddr4: 0x%x (should match vaddr1 if memory reused)\n", vaddr4);

    memoryManager.releasePages(AddressPoolType::KERNEL, vaddr4, 3);

    printf("Test Completed\n");

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

    // 内存管理器
    memoryManager.openPageMechanism();
    memoryManager.initialize();

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
