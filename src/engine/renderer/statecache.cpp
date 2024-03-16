#include <engine/renderer/statecache.hpp>

void statecache_robjinit(struct statecache_robjcache **cache) {
    *cache = (struct statecache_robjcache *)malloc(sizeof(struct statecache_robjcache));
    (*cache)->head = NULL;
    (*cache)->tail = NULL;
    (*cache)->size.store(0);
    (*cache)->capacity = 1024;
    // job_initmutex(&(*cache)->mutex);
}

void statecache_robjset(struct statecache_robjcache *cache, uint32_t key, uint64_t value) {
JOB_MUTEXSAFE(&cache->mutex,
    struct statecache_robjcachepair *node = cache->head;
    while (node) {
        if (node->key == key) {
            node->value = value;
            break;
        }
        node = node->next;
    }

    ASSERT(cache->size.load() < cache->capacity, "Pipeline global storage ran out of space.\n");
    struct statecache_robjcachepair *pair = (struct statecache_robjcachepair *)malloc(sizeof(struct statecache_robjcachepair));
    pair->key = key;
    pair->value = value;
    pair->next = NULL;
    if (cache->tail) {
        cache->tail->next = pair;
    }
    cache->tail = pair;
    if (!cache->head) {
        cache->head = cache->tail;
    }
    cache->size.fetch_add(1);
);
}

uint64_t statecache_robjget(struct statecache_robjcache *cache, uint32_t key) {
JOB_MUTEXSAFE(&cache->mutex,
    struct statecache_robjcachepair *node = cache->head;
    while (node) {
        if (node->key == key) {
            JOB_MUTEXSAFEEARLY(&cache->mutex);
            return node->value;
        }
        node = node->next;
    }
    return UINT64_MAX;
);
}

void statecache_handleset(struct statecache_handlecache *cache, uint32_t key, uint16_t value) {
JOB_MUTEXSAFE(&cache->mutex,
    struct statecache_robjcachepair *node = cache->head;
    while (node) {
        if (node->key == key) {
            node->value = value;
            break;
        }
        node = node->next;
    }

    ASSERT(atomic_load(&cache->size) < cache->capacity, "Pipeline global storage ran out of space.\n");
    struct statecache_robjcachepair *pair = (struct statecache_robjcachepair *)malloc(sizeof(struct statecache_robjcachepair));
    pair->key = key;
    pair->value = value;
    pair->next = NULL;
    if (cache->tail) {
        cache->tail->next = pair;
    }
    cache->tail = pair;
    if (!cache->head) {
        cache->head = cache->tail;
    }
    atomic_fetch_add(&cache->size, 1);
);
}

uint16_t statecache_handleget(struct statecache_handlecache *cache, uint32_t key) {
JOB_MUTEXSAFE(&cache->mutex,
    struct statecache_robjcachepair *node = cache->head;
    while (node) {
        if (node->key == key) {
            JOB_MUTEXSAFEEARLY(&cache->mutex);
            return node->value;
        }
        node = node->next;
    }
    return UINT16_MAX;
);
}

