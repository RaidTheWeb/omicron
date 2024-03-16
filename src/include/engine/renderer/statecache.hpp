#ifndef _ENGINE__RENDERER__STATECACHE_HPP
#define _ENGINE__RENDERER__STATECACHE_HPP

#include <engine/concurrency/job.hpp>
#include <stdint.h>
#include <engine/utils.hpp>

struct statecache_robjcachepair {
    uint32_t key;
    uint64_t value;
    struct statecache_robjcachepair *next;
};

struct statecache_robjcache {
    OJob::Mutex mutex;
    struct statecache_robjcachepair *head;
    struct statecache_robjcachepair *tail;
    std::atomic<size_t> size;
    size_t capacity;
};

void statecache_robjinit(struct statecache_robjcache **cache);
uint64_t statecache_robjget(struct statecache_robjcache *cache, uint32_t key);
void statecache_robjset(struct statecache_robjcache *cache, uint32_t key, uint64_t value);

struct statecache_handlecachepair {
    uint32_t key;
    uint16_t value;
    struct statecache_handlecachepair *next;
};

struct statecache_handlecache {
    OJob::Mutex mutex;
    struct statecache_robjcachepair *head;
    struct statecache_robjcachepair *tail;
    std::atomic<size_t> size;
    size_t capacity;
};

uint16_t statecache_handleget(struct statecache_handlecache *cache, uint32_t key);
void statecache_handleset(struct statecache_handlecache *cache, uint32_t key, uint16_t value);

#endif
