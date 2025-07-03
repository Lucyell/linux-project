#include "bitmap.h"
#include "stdlib.h"
#include "stdio.h"

BitMap::BitMap()
{
}

void BitMap::initialize(char *bitmap, const int length, const int startAddress)
{
    this->bitmap = bitmap;
    this->length = length;
    this->startAddress = startAddress;

    int bytes = ceil(length, 8);
    for (int i = 0; i < bytes; ++i)
    {
        bitmap[i] = 0;
    }
}

int BitMap::getStartAddress() const
{
    return this->startAddress;
}

bool BitMap::get(const int index) const
{
    int pos = index / 8;
    int offset = index % 8;

    return (bitmap[pos] & (1 << offset));
}

void BitMap::set(const int index, const bool status)
{
    int pos = index / 8;
    int offset = index % 8;

    // 清0
    bitmap[pos] = bitmap[pos] & (~(1 << offset));

    // 置1
    if (status)
    {
        bitmap[pos] = bitmap[pos] | (1 << offset);
    }
}

int BitMap::allocate(const int count)
{
    if (count <= 0) return -1;

    for (int i = 0; i <= length - count; ++i)
    {
        bool canAllocate = true;

        for (int j = 0; j < count; ++j)
        {
            if (get(i + j))
            {
                canAllocate = false;
                i = i + j;
                break;
            }
        }

        if (canAllocate)
        {
            for (int j = 0; j < count; ++j)
            {
                set(i + j, true);
            }

            printf("First Fit Allocation: Starting from page% d and continuing for% d pages\n", i, count);
            return i;
        }
    }

    printf("First Fit allocation failed: Request% d pages\n", count);
    return -1;
}


void BitMap::release(const int index, const int count)
{
    for (int i = 0; i < count; ++i)
    {
        set(index + i, false);
    }
    printf("Release: Starting from page% d and continuing for% d pages\n", index, count);
}

char *BitMap::getBitmap()
{
    return (char *)bitmap;
}

int BitMap::size() const
{
    return length;
}


