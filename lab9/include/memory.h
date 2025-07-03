#ifndef MEMORY_H
#define MEMORY_H

#include "address_pool.h"

enum AddressPoolType
{
    USER,
    KERNEL
};

class MemoryManager
{
public:
    // 可管理的内存容量
    int totalMemory;
    // 内核物理地址池
    AddressPool kernelPhysical;
    // 用户物理地址池
    AddressPool userPhysical;
    // 内核虚拟地址池
    AddressPool kernelVirtual;

public:
    MemoryManager();

    void initialize();

    int allocatePhysicalPages(enum AddressPoolType type, const int count);
    void releasePhysicalPages(enum AddressPoolType type, const int startAddress, const int count);

    int getTotalMemory();

    void openPageMechanism();

    int allocatePages(enum AddressPoolType type, const int count);
    int allocateVirtualPages(enum AddressPoolType type, const int count);
    bool connectPhysicalVirtualPage(const int virtualAddress, const int physicalPageAddress);

    int toPDE(const int virtualAddress);
    int toPTE(const int virtualAddress);

    void releasePages(enum AddressPoolType type, const int virtualAddress, const int count);
    int vaddr2paddr(int vaddr);
    void releaseVirtualPages(enum AddressPoolType type, const int vaddr, const int count);
};


extern MemoryManager memoryManager;


void* kmalloc(int size);
void kfree(void* ptr);
void initHeap();
void* expandHeap(int minSize);


void test_malloc();

#endif

