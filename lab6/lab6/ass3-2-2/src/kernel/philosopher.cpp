#include "stdio.h"
#include "sync.h"
#include "program.h"
#include "os_modules.h"
#include <stdarg.h>

#define N 5

enum State { THINKING, HUNGRY, EATING };
State state[N];
Semaphore sem[N];
Semaphore mutex;

int left(int i) { return (i + N - 1) % N; }
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

void test(int i) {
    if (state[i] == HUNGRY && state[left(i)] != EATING && state[right(i)] != EATING) {
        state[i] = EATING;
        sem[i].V();
    }
}

void take_forks(int i) {
    mutex.P();
    state[i] = HUNGRY;
    printf("Philosopher %d is hungry\n", i);
    test(i);
    mutex.V();
    sem[i].P();
}

void put_forks(int i) {
    mutex.P();
    state[i] = THINKING;
    printf("Philosopher %d put down forks\n", i);
    test(left(i));
    test(right(i));
    mutex.V();
}

void philosopher(void* param) {
    int id = (int)(long)param;
    while (true) {
        printf("Philosopher %d is thinking\n", id);
        for (volatile int i = 0; i < 10000000; ++i);

        take_forks(id);
        printf("Philosopher %d is eating\n", id);
        for (volatile int i = 0; i < 10000000; ++i);

        put_forks(id);
    }
}

extern "C" void start_philosophers() {
    printf("Start philosophers\n");
    mutex.initialize(1);
    for (int i = 0; i < N; ++i) {
        state[i] = THINKING;
        sem[i].initialize(0);
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

