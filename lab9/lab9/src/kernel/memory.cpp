#include "memory.h"
#include "os_constant.h"
#include "stdlib.h"
#include "asm_utils.h"
#include "stdio.h"
#include "program.h"
#include "os_modules.h"

#define HEAP_INIT_SIZE (16 * 1024) // 初始堆大小：16KB
#define HEAP_EXPAND_SIZE (8 * 1024) // 每次堆不够时扩展：8KB
#define ALIGN4(size) (((size) + 3) & ~3) // 4字节对齐

struct BlockHeader {
    int size;              // 含头部的总大小
    bool isFree;           // 是否空闲
    BlockHeader* next;     // 下一块
};

static BlockHeader* heapStart = nullptr;

MemoryManager::MemoryManager()
{
    initialize();
}

void MemoryManager::initialize()
{
    this->totalMemory = 0;
    this->totalMemory = getTotalMemory();

    int usedMemory = 256 * PAGE_SIZE + 0x100000;
    if (this->totalMemory < usedMemory)
    {
        printf("memory is too small, halt.\n");
        asm_halt();
    }

    int freeMemory = this->totalMemory - usedMemory;
    int freePages = freeMemory / PAGE_SIZE;
    int kernelPages = freePages / 2;
    int userPages = freePages - kernelPages;

    int kernelPhysicalStartAddress = usedMemory;
    int userPhysicalStartAddress = usedMemory + kernelPages * PAGE_SIZE;

    int kernelPhysicalBitMapStart = BITMAP_START_ADDRESS;
    int userPhysicalBitMapStart = kernelPhysicalBitMapStart + ceil(kernelPages, 8);
    int kernelVirtualBitMapStart = userPhysicalBitMapStart + ceil(userPages, 8);

    kernelPhysical.initialize((char *)kernelPhysicalBitMapStart, kernelPages, kernelPhysicalStartAddress);
    userPhysical.initialize((char *)userPhysicalBitMapStart, userPages, userPhysicalStartAddress);
    kernelVirtual.initialize((char *)kernelVirtualBitMapStart, kernelPages, KERNEL_VIRTUAL_START);

    printf("total memory: %d bytes ( %d MB )\n", this->totalMemory, this->totalMemory / 1024 / 1024);
}

int MemoryManager::allocatePhysicalPages(enum AddressPoolType type, const int count)
{
    int start = -1;
    if (type == AddressPoolType::KERNEL)
        start = kernelPhysical.allocate(count);
    else if (type == AddressPoolType::USER)
        start = userPhysical.allocate(count);

    return (start == -1) ? 0 : start;
}

void MemoryManager::releasePhysicalPages(enum AddressPoolType type, const int paddr, const int count)
{
    if (type == AddressPoolType::KERNEL)
        kernelPhysical.release(paddr, count);
    else if (type == AddressPoolType::USER)
        userPhysical.release(paddr, count);
}

int MemoryManager::getTotalMemory()
{
    if (!this->totalMemory)
    {
        int memory = *((int *)MEMORY_SIZE_ADDRESS);
        int low = memory & 0xffff;
        int high = (memory >> 16) & 0xffff;
        this->totalMemory = low * 1024 + high * 64 * 1024;
    }
    return this->totalMemory;
}

void MemoryManager::openPageMechanism()
{
    int *directory = (int *)PAGE_DIRECTORY;
    int *page = (int *)(PAGE_DIRECTORY + PAGE_SIZE);

    memset(directory, 0, PAGE_SIZE);
    memset(page, 0, PAGE_SIZE);

    int address = 0;
    for (int i = 0; i < 256; ++i)
    {
        page[i] = address | 0x7;
        address += PAGE_SIZE;
    }

    directory[0] = ((int)page) | 0x07;
    directory[768] = directory[0];
    directory[1023] = ((int)directory) | 0x7;

    asm_init_page_reg(directory);
    printf("open page mechanism\n");
}

int MemoryManager::allocatePages(enum AddressPoolType type, const int count)
{
    int virtualAddress = allocateVirtualPages(type, count);
    if (!virtualAddress)
        return 0;

    bool flag;
    int physicalPageAddress;
    int vaddress = virtualAddress;

    for (int i = 0; i < count; ++i, vaddress += PAGE_SIZE)
    {
        flag = false;
        physicalPageAddress = allocatePhysicalPages(type, 1);
        if (physicalPageAddress)
        {
            flag = connectPhysicalVirtualPage(vaddress, physicalPageAddress);
        }

        if (!flag)
        {
            releasePages(type, virtualAddress, i);
            releaseVirtualPages(type, virtualAddress + i * PAGE_SIZE, count - i);
            return 0;
        }
    }
    return virtualAddress;
}

int MemoryManager::allocateVirtualPages(enum AddressPoolType type, const int count)
{
    int start = -1;
    if (type == AddressPoolType::KERNEL)
        start = kernelVirtual.allocate(count);
    return (start == -1) ? 0 : start;
}

bool MemoryManager::connectPhysicalVirtualPage(const int virtualAddress, const int physicalPageAddress)
{
    int *pde = (int *)toPDE(virtualAddress);
    int *pte = (int *)toPTE(virtualAddress);

    if (!(*pde & 0x00000001))
    {
        int page = allocatePhysicalPages(AddressPoolType::KERNEL, 1);
        if (!page)
            return false;

        *pde = page | 0x7;
        char *pagePtr = (char *)(((int)pte) & 0xfffff000);
        memset(pagePtr, 0, PAGE_SIZE);
    }

    *pte = physicalPageAddress | 0x7;
    return true;
}

int MemoryManager::toPDE(const int virtualAddress)
{
    return (0xfffff000 + (((virtualAddress & 0xffc00000) >> 22) * 4));
}

int MemoryManager::toPTE(const int virtualAddress)
{
    return (0xffc00000 + ((virtualAddress & 0xffc00000) >> 10) + (((virtualAddress & 0x003ff000) >> 12) * 4));
}

void MemoryManager::releasePages(enum AddressPoolType type, const int virtualAddress, const int count)
{
    int vaddr = virtualAddress;
    int *pte;
    for (int i = 0; i < count; ++i, vaddr += PAGE_SIZE)
    {
        releasePhysicalPages(type, vaddr2paddr(vaddr), 1);
        pte = (int *)toPTE(vaddr);
        *pte = 0;
    }
    releaseVirtualPages(type, virtualAddress, count);
}

int MemoryManager::vaddr2paddr(int vaddr)
{
    int *pte = (int *)toPTE(vaddr);
    int page = (*pte) & 0xfffff000;
    int offset = vaddr & 0xfff;
    return (page + offset);
}

void MemoryManager::releaseVirtualPages(enum AddressPoolType type, const int vaddr, const int count)
{
    if (type == AddressPoolType::KERNEL)
        kernelVirtual.release(vaddr, count);
}

// ================================
//        Heap memory support
// ================================
void* kmalloc(int size)
{
    printf("[kmalloc] ENTER: size=%d, heapStart=0x%x\n", size, (int)heapStart);
    if (!heapStart) {
        printf("[kmalloc] ERROR: heapStart is NULL!\n");
        return nullptr;
    }
    size = ALIGN4(size);
    BlockHeader* current = heapStart;
    printf("[kmalloc] first BlockHeader at 0x%x, size=%d, isFree=%d\n",
           (int)current, current->size, current->isFree);
    while (current)
    {
        if (current->isFree && current->size >= size + (int)sizeof(BlockHeader))
        {
            if (current->size - size > (int)(sizeof(BlockHeader) + 4)) // 分裂
            {
                BlockHeader* newBlock = (BlockHeader*)((char*)current + sizeof(BlockHeader) + size);
                newBlock->size = current->size - sizeof(BlockHeader) - size;
                newBlock->isFree = true;
                newBlock->next = current->next;

                current->size = size + sizeof(BlockHeader);
                current->next = newBlock;
            }
            current->isFree = false;
            return (char*)current + sizeof(BlockHeader);
        }
        current = current->next;
    }

    // 没找到合适块，扩展堆
    BlockHeader* newBlock = (BlockHeader*)expandHeap(HEAP_EXPAND_SIZE);
    if (!newBlock) return nullptr;
    newBlock->size = HEAP_EXPAND_SIZE;
    newBlock->isFree = true;
    newBlock->next = nullptr;

    // 加到末尾
    current = heapStart;
    while (current->next) current = current->next;
    current->next = newBlock;

    return kmalloc(size); // 递归调用再次尝试分配
}

void kfree(void* ptr)
{
    if (!ptr) return;
    BlockHeader* block = (BlockHeader*)((char*)ptr - sizeof(BlockHeader));
    block->isFree = true;

    // 合并相邻空闲块
    BlockHeader* current = heapStart;
    while (current && current->next)
    {
        if (current->isFree && current->next->isFree)
        {
            current->size += current->next->size;
            current->next = current->next->next;
        }
        else
        {
            current = current->next;
        }
    }
}

void initHeap()
{
    if (heapStart != nullptr)
        return;

    heapStart = (BlockHeader*)expandHeap(HEAP_INIT_SIZE);
    printf("heapStart = 0x%x\n", heapStart);
    printf("heapStart physical = 0x%x\n", memoryManager.vaddr2paddr((int)heapStart));

    if (!heapStart)
    {
        printf("initHeap failed\n");
        return;
    }

    heapStart->size = HEAP_INIT_SIZE;
    heapStart->isFree = true;
    heapStart->next = nullptr;
}

void* expandHeap(int minSize)
{
    int pages = ceil(minSize, PAGE_SIZE);
    int addr = memoryManager.allocatePages(AddressPoolType::KERNEL, pages);
    if (!addr) {
        printf("expandHeap: failed to allocate pages\n");
        return nullptr;
    }
    printf("expandHeap returns 0x%x\n", addr);
    return (void*)addr;
}

void test_malloc()
{

    printf("== Begin kmalloc/kfree test ==\n");

    printf("step 1: about to call initHeap()\n");
    initHeap();
    printf("step 2: initHeap() returned\n");

    printf("step 3: about to call kmalloc(16)\n");
    void* t = kmalloc(16);
    printf("step 4: kmalloc(16) returned 0x%x\n", (int)t);

    initHeap();

    void* p1 = kmalloc(1000);
    void* p2 = kmalloc(2000);
    void* p3 = kmalloc(3000);

    printf("p1 = 0x%x\n", (int)p1);
    printf("p2 = 0x%x\n", (int)p2);
    printf("p3 = 0x%x\n", (int)p3);

    kfree(p2);
    void* p4 = kmalloc(1800);
    printf("p4 = 0x%x (should be near p2)\n", (int)p4);

    kfree(p1);
    kfree(p3);
    kfree(p4);

    void* p5 = kmalloc(5000);
    printf("p5 = 0x%x (should be reused)\n", (int)p5);

    kfree(p5);

    printf("== kmalloc/kfree test complete ==\n");
}


