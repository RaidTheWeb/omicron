#include <engine/concurrency/coroutine.hpp>

static __attribute__((noinline)) size_t coroutine_alignforward(size_t addr, size_t align) {
    return (addr + (align - 1)) & ~(align - 1);
}

static __thread struct coroutine *coroutine_current = NULL;

static __attribute__((noinline)) void coroutine_preparejumpin(struct coroutine *co) {
    struct coroutine *prev = coroutine_running();

    ASSERT(co->prev == NULL, "Previous coroutine is not NULL.\n");
    co->prev = prev;
    if (prev) {
        ASSERT(prev->state == COROUTINE_RUNNING, "Coroutine isn't running\n");
        prev->state = COROUTINE_NORMAL;
    }
    coroutine_current = co;
}

static __attribute__((noinline)) void coroutine_preparejumpout(struct coroutine *co) {
    struct coroutine *prev = co->prev;
    co->prev = NULL;
    if (prev) {
        prev->state = COROUTINE_RUNNING;
    }
    coroutine_current = prev;
}

static void coroutine_jumpin(struct coroutine *co);
static void coroutine_jumpout(struct coroutine *co);

static __attribute__((noinline)) void couroutine_main(struct coroutine *co) {
    co->func(co);
    co->state = COROUTINE_DEAD;
    coroutine_jumpout(co);
}

#if defined(__x86_64__) && defined(__linux__)

struct coroutine_ctx {
    void *rip;
    void *rsp;
    void *rbp;
    void *rbx;
    void *r12;
    void *r13;
    void *r14;
    void *r15;
};

extern "C" void coroutine_wrapmain(void);
extern "C" int coroutine_switch(struct coroutine_ctx *from, struct coroutine_ctx *to);

asm(
    ".text\n"
    ".globl coroutine_wrapmain\n"
    ".type coroutine_wrapmain @function\n"
    ".hidden coroutine_wrapmain\n"
    "coroutine_wrapmain:\n"
    "\tmovq %r13, %rdi\n"
    "\tjmpq *%r12\n"
);

asm(
    ".text\n"
    ".globl coroutine_switch\n"
    ".type coroutine_switch @function\n"
    ".hidden coroutine_switch\n"
    "coroutine_switch:\n"
    "\tleaq 0x3d(%rip), %rax\n"
    "\tmovq %rax, (%rdi)\n"
    "\tmovq %rsp, 8(%rdi)\n"
    "\tmovq %rbp, 16(%rdi)\n"
    "\tmovq %rbx, 24(%rdi)\n"
    "\tmovq %r12, 32(%rdi)\n"
    "\tmovq %r13, 40(%rdi)\n"
    "\tmovq %r14, 48(%rdi)\n"
    "\tmovq %r15, 56(%rdi)\n"
    "\tmovq 56(%rsi), %r15\n"
    "\tmovq 48(%rsi), %r14\n"
    "\tmovq 40(%rsi), %r13\n"
    "\tmovq 32(%rsi), %r12\n"
    "\tmovq 24(%rsi), %rbx\n"
    "\tmovq 16(%rsi), %rbp\n"
    "\tmovq 8(%rsi), %rsp\n"
    "\tjmpq *(%rsi)\n"
    "\tret\n"
);

static int coroutine_makecontext(struct coroutine *co, struct coroutine_ctx *ctx, void *stackbase, size_t stacksize) {
    stacksize = stacksize - 128; // red zone
    void **stackhigh = (void **)((size_t)stackbase + stacksize - sizeof(size_t));
    stackhigh[0] = (void *)0xdeaddeaddeaddead; // dummy return
    ctx->rip = (void *)coroutine_wrapmain; // where to jump to (wrapper function)
    ctx->rsp = (void *)stackhigh; // top of our stack
    ctx->r12 = (void *)couroutine_main; // argument for wrapper function
    ctx->r13 = (void *)co; // argument to coroutine function
    return 0;
}

#endif

struct coroutine_context {
    struct coroutine_ctx ctx;
    struct coroutine_ctx backctx;
};

static void coroutine_jumpin(struct coroutine *co) {
    struct coroutine_context *ctx = (struct coroutine_context *)co->ctx;
    coroutine_preparejumpin(co);
    coroutine_switch(&ctx->backctx, &ctx->ctx);
}

static void coroutine_jumpout(struct coroutine *co) {
    struct coroutine_context *ctx = (struct coroutine_context *)co->ctx;
    coroutine_preparejumpout(co);
    coroutine_switch(&ctx->ctx, &ctx->backctx);
}

static int coroutine_createctx(struct coroutine *co, struct coroutine_desc *desc) {
    size_t coaddr = (size_t)co;
    size_t ctxaddr = coroutine_alignforward(coaddr + sizeof(struct coroutine), 16);
    size_t storageaddr = coroutine_alignforward(ctxaddr + sizeof(struct coroutine_context), 16);
    size_t stackaddr = coroutine_alignforward(storageaddr + desc->size, 16);
    struct coroutine_context *ctx = (struct coroutine_context *)ctxaddr;
    memset(ctx, 0, sizeof(struct coroutine_context));
    uint8_t *storage = (uint8_t *)storageaddr;
    memset(storage, 0, desc->size);
    void *stackbase = (void *)stackaddr;
    size_t stacksize = desc->stacksize;
    memset(stackbase, 0, stacksize);
    int res = coroutine_makecontext(co, &ctx->ctx, stackbase, stacksize);
    if (res != 0) {
        return res;
    }
    co->ctx = ctx;
    co->stackbase = stackbase;
    co->stacksize = stacksize;
    co->storage = storage;
    co->size = desc->size;
    return 0;
}

static __attribute__((noinline)) void coroutine_initdescsizes(struct coroutine_desc *desc, size_t stacksize) {
    desc->corosize = coroutine_alignforward(sizeof(struct coroutine), 16) + coroutine_alignforward(sizeof(struct coroutine_context), 16) + coroutine_alignforward(desc->size, 16) + stacksize + 16;
    desc->stacksize = stacksize;
}

struct coroutine_desc coroutine_initdesc(void (*func)(struct coroutine *co), size_t stacksize) {
    if (stacksize != 0) {
        if (stacksize < COROUTINE_MINSTACKSIZE) {
            stacksize = COROUTINE_MINSTACKSIZE;
        }
    } else {
        stacksize = COROUTINE_DEFAULTSTACKSIZE;
    }

    stacksize = coroutine_alignforward(stacksize, 16);
    struct coroutine_desc desc;
    memset(&desc, 0, sizeof(struct coroutine_desc));
    desc.func = func;
    desc.size = COROUTINE_DEFAULTSTACKSIZE;
    coroutine_initdescsizes(&desc, stacksize);
    return desc;
}

int coroutine_init(struct coroutine *co, struct coroutine_desc *desc) {
    ASSERT(co, "Attempted to initialise invalid coroutine.\n");
    memset(co, 0, sizeof(struct coroutine));
    int res = coroutine_createctx(co, desc);
    if (res != 0) {
        return res;
    }

    co->state = COROUTINE_SUSPENDED;
    co->func = desc->func;
    co->userdata = desc->userdata;
    co->magic = COROUTINE_MAGICNUMBER;
    pthread_spin_init(&co->lock, true);
    return 0;
}

int coroutine_uninit(struct coroutine *co) {
    ASSERT(co, "Attempted to uninitialise invalid coroutine.\n");

    ASSERT(co->state == COROUTINE_SUSPENDED || co->state == COROUTINE_DEAD, "Attempted to uninitialise a coroutine that isn't dead of suspended.\n");

    co->state = COROUTINE_DEAD;
    return 0;
}

int coroutine_create(struct coroutine **co, struct coroutine_desc *desc) {
    ASSERT(co, "Output pointer is NULL.\n");

    struct coroutine *pco = (struct coroutine *)malloc(desc->corosize);
    ASSERT(pco, "Coroutine allocation failed.\n");

    int res = coroutine_init(pco, desc);
    if (res != 0) {
        free(pco);
        *co = NULL;
        return res;
    }
    *co = pco;
    return 0;
}

int coroutine_destroy(struct coroutine *co) {
    ASSERT(co, "Attempted to resume invalid coroutine.\n");

    int res = coroutine_uninit(co);
    if (res != 0) {
        return res;
    }

    free(co);
    return 0;
}

int coroutine_resume(struct coroutine *co) {
    ASSERT(co->magic == COROUTINE_MAGICNUMBER, "Attempted to resume a corrupt coroutine.\n");
    ASSERT(co, "Attempted to resume invalid coroutine.\n");
    ASSERT(co->state == COROUTINE_SUSPENDED, "Attempted to resume a coroutine that isn't suspended (potential resume-before-yield race condition, check code to see if a coroutine has any possibility of being resumed before it gets to coroutine_yield()).\n");
    co->state = COROUTINE_RUNNING;
    coroutine_jumpin(co);
    return 0;
}

int coroutine_yield(struct coroutine *co) {
    ASSERT(co, "Attempt to yield invalid coroutine.\n");
    volatile size_t dummy;
    size_t stackaddr = (size_t)&dummy;
    size_t stackmin = (size_t)co->stackbase;
    size_t stackmax = stackmin + co->stacksize;
    ASSERT(co->magic == COROUTINE_MAGICNUMBER && stackaddr > stackmin && stackaddr < stackmax, "Coroutine stack overflow.\n");
    ASSERT(co->state == COROUTINE_RUNNING, "Attempt to yield coroutine that isn't running.\n");
    co->state = COROUTINE_SUSPENDED;
    coroutine_jumpout(co);
    return 0;
}

int coroutine_status(struct coroutine *co) {
    if (co != NULL) {
        return co->state;
    }
    return COROUTINE_DEAD;
}

void *coroutine_getuserdata(struct coroutine *co) {
    if (co != NULL) {
        return co->userdata;
    }
    return NULL;
}

static __attribute__((noinline)) struct coroutine *coroutine_runningintern(void) {
    return coroutine_current;
}

struct coroutine *coroutine_running(void) {
    struct coroutine *(* volatile func)(void) = coroutine_runningintern;
    return func();
}
