#include "address_pool.h"
#include "os_constant.h"
#include "stdlib.h"
#include "stdio.h"


AddressPool::AddressPool()
{
}

// 设置地址池BitMap
void AddressPool::initialize(char *bitmap, const int length, const int startAddress)
{
    resources.initialize(bitmap, length, startAddress); 
    this->startAddress = startAddress;
}

// 从地址池中分配count个连续页
int AddressPool::allocate(const int count)
{
    int index = resources.allocate(count);
    if (index != -1)
    {
        printf("[AddressPool] allocate %d page(s) at page index %d (phys addr: 0x%x)\n",
               count, index, startAddress + index * PAGE_SIZE);
    }
    else
    {
        printf("[AddressPool] failed to allocate %d page(s)\n", count);
    }
    return index;
}



// 释放若干页的空间
void AddressPool::release(const int index, const int amount)
{
    resources.release(index, amount);  // 直接传递页号下标
}

