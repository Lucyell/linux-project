#include "page_replacement.h"
#include "stdio.h"
#include "os_constant.h"


PageReplacementManager::PageReplacementManager(int frameCount, int physicalStartAddress)
{
    this->frameCount = frameCount;
    this->physicalStartAddress = physicalStartAddress;
    this->front = 0;
    this->rear = 0;
    this->size = 0;

    for (int i = 0; i < MAX_FRAMES; ++i)
        pageQueue[i] = -1;

    for (int i = 0; i < MAX_PAGES; ++i)
        pageMap[i] = false;
}

bool PageReplacementManager::accessPage(int pageNumber)
{
    if (pageNumber < 0 || pageNumber >= MAX_PAGES)
        return false;

    if (pageMap[pageNumber]) {
        printf("[HIT] page %d is already in memory.\n", pageNumber);
        return true;
    }

    int victim = -1;
    if (size < frameCount) {
        pageQueue[rear] = pageNumber;
        rear = (rear + 1) % frameCount;
        ++size;
    } else {
        victim = pageQueue[front];
        front = (front + 1) % frameCount;
        pageMap[victim] = false;

        pageQueue[rear] = pageNumber;
        rear = (rear + 1) % frameCount;
    }

    pageMap[pageNumber] = true;

    int virtualAddress = pageNumber * PAGE_SIZE;
    int physicalFrameIndex = (rear - 1 + frameCount) % frameCount;
    int physicalAddress = physicalStartAddress + physicalFrameIndex * PAGE_SIZE;

    printf("[MISS] load page %d into memory.\n", pageNumber);
    printf("       virtual address: 0x%x, physical address: 0x%x\n",
           virtualAddress, physicalAddress);

    return false;
}


