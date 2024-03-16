#ifndef _ENGINE__EVENT_HPP
#define _ENGINE__EVENT_HPP

#include <engine/concurrency/job.hpp>
#include <pthread.h>
#include <engine/utils/queue.hpp>
#include <stdint.h>

#define EVENT_MAXARGS 8

struct event {
    uint32_t type;
    void *self;
    uint32_t numargs;
    uintptr_t args[EVENT_MAXARGS];
    uint64_t deliverytime; // future (microseonds)
};

extern OUtils::MPMCQueue event_events;

// dispatch event to receiver
void event_dispatch(struct event *event);
// dispatch all events queued (at delivery time)
void event_dispatchevents(uint64_t time);

// queue event (deep copy event as well)
void event_queue(struct event event);

struct event_listener {
    void (*func)(void *self, struct event *event);
    void *self;
    uint32_t type;
};

extern OJob::Mutex event_listenermutex;
extern std::vector<struct event_listener> event_listeners;

// assign a listener function with optional argument for self
struct event_listener *event_listen(uint32_t type, void (*func)(void *self, struct event *event), void *self);
// give up listener (called on the destruction of the event receiver)
void event_relinquish(struct event_listener *listener);

void event_init(void);

#endif
