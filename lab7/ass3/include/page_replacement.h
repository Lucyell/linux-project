#ifndef PAGE_REPLACEMENT_H
#define PAGE_REPLACEMENT_H

#define MAX_FRAMES 256
#define MAX_PAGES 1024

class PageReplacementManager {
public:
    PageReplacementManager(int frameCount);
    PageReplacementManager(int frameCount, int physicalStartAddress);

    bool accessPage(int pageNumber);

private:
    int frameCount;
    int front, rear, size;
    int pageQueue[MAX_FRAMES];
    bool pageMap[MAX_PAGES];

    int physicalStartAddress;
};

#endif

