#ifndef _CONCURRENCY__COROUTINE_HPP
#define _CONCURRENCY__COROUTINE_HPP

#include <assertion.hpp>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define COROUTINE_MINSTACKSIZE 32768 // 32KiB minimum stack size
#define COROUTINE_DEFAULTSTACKSIZE 57344 // 64KiB stack size

#define COROUTINE_MAGICNUMBER 0x7e3cb1a9

enum {
    COROUTINE_DEAD,
    COROUTINE_NORMAL,
    COROUTINE_RUNNING,
    COROUTINE_SUSPENDED
};

struct coroutine {
    void *ctx;
    int state;
    void (*func)(struct coroutine *co);
    struct coroutine *prev;
    void *userdata;
    void *stackbase;
    size_t stacksize;
    uint8_t *storage;
    size_t stored;
    size_t size;
    void *prevstack;
    void *prevfibre;
    void *fibre;
    size_t magic;
    pthread_spinlock_t lock;
};

struct coroutine_desc {
    void (*func)(struct coroutine *co);
    void *userdata;
    size_t size;
    size_t corosize;
    size_t stacksize;
};

struct coroutine_desc coroutine_initdesc(void (*func)(struct coroutine *co), size_t stacksize);
int coroutine_init(struct coroutine *co, struct coroutine_desc *desc);
int coroutine_uninit(struct coroutine *co);
int coroutine_create(struct coroutine **co, struct coroutine_desc *desc);
int coroutine_destroy(struct coroutine *co);
int coroutine_resume(struct coroutine *co);
int coroutine_yield(struct coroutine *co);
int coroutine_status(struct coroutine *co);
void *coroutine_userdata(struct coroutine *co);

struct coroutine *coroutine_running(void);

#endif
