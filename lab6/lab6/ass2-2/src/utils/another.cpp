#include "os_type.h"
#include "another.h"

extern "C" uint32 asm_atomic_bts(uint32 *addr) {
    uint32 oldbit;
    __asm__ __volatile__(
        "lock btsl $0, %1\n\t"
        "sbb %0, %0"
        : "=r"(oldbit), "+m"(*addr)
        :
        : "memory", "cc"
    );
    return oldbit;
}

