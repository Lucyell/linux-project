#include "stdio.h"
#include "sync.h"
#include "program.h"
#include "os_modules.h"
#include <stdarg.h>

#define N 5

Semaphore forks[N];

int left(int i) { return i; }
int right(int i) { return (i + 1) % N; }

int sprintf(char *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char *str = buf;
    const char *p = fmt;
    while (*p) {
        if (*p != '%') {
            *str++ = *p++;
            continue;
        }
        p++;
        if (*p == 'd') {
            int num = va_arg(args, int);
            if (num < 0) {*str++ = '-'; num = -num;}
            char temp[16]; int i = 0;
            if (num == 0) temp[i++] = '0';
            while (num) { temp[i++] = '0' + num % 10; num /= 10; }
            while (i--) *str++ = temp[i];
        } else if (*p == 's') {
            const char *s = va_arg(args, const char*);
            while (*s) *str++ = *s++;
        } else {
            *str++ = '%'; *str++ = *p;
        }
        p++;
    }
    *str = '\0';
    va_end(args);
    return str - buf;
}


void take_forks(int i) {
    printf("Philosopher %d is trying to pick up forks\n", i);

    forks[left(i)].P();
    printf("Philosopher %d picked up left fork %d\n", i, left(i));

    forks[right(i)].P();
    printf("Philosopher %d picked up right fork %d\n", i, right(i));

    printf("Philosopher %d starts eating\n", i);
}

void put_forks(int i) {
    forks[right(i)].V();
    forks[left(i)].V();
    printf("Philosopher %d put down forks %d and %d\n", i, left(i), right(i));
}

void philosopher(void* param) {
    int i = (int)(long)param;
    while (true) {
        printf("Philosopher %d is thinking\n", i);

        for (volatile int t = 0; t < 10000000; ++t);

        take_forks(i);

        for (volatile int t = 0; t < 10000000; ++t);

        put_forks(i);
    }
}

extern "C" void start_philosophers_thread(void*) {
    for (int i = 0; i < N; ++i) {
        forks[i].initialize(1);
    }

    for (int i = 0; i < N; ++i) {
        char name[20];
        sprintf(name, "philosopher_%d", i);
        int pid = programManager.executeThread(philosopher, (void*)(long)i, name, 1);
        if (pid == -1) {
            printf("Failed to create philosopher %d\n", i);
        } else {
            printf("Created philosopher %d (pid=%d)\n", i, pid);
        }
    }
}
