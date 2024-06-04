#include <engine/event.hpp>
#include <engine/utils/memory.hpp>

OUtils::MPMCQueue event_events = OUtils::MPMCQueue(256); // event queue
OJob::Mutex event_listenermutex; // spinlock so synchronisation doesn't break

// XXX: Allocate these better
std::vector<struct event_listener> event_listeners;
OUtils::PoolAllocator event_allocator = OUtils::PoolAllocator(sizeof(struct event), 512); // allocator for events, with the 512 hard limit giving us a maximum of 512 actively queued events (hopefully we never reach this number) this pool allocator will consume 16KB in memory.

struct event_listener *event_listen(uint32_t type, void (*func)(void *self, struct event *event), void *self) {
    OJob::ScopedMutex mutex(&event_listenermutex);

    struct event_listener listener;
    listener.self = self;
    listener.func = func;
    listener.type = type;

    event_listeners.push_back(listener);
    // struct event_listenernode *node = (struct event_listenernode *)malloc(sizeof(struct event_listenernode));
    // node->data = listener;
    // node->next = NULL;
    // if (event_listeners->tail) {
        // ((struct event_listenernode *)event_listeners->tail)->next = node;
    // }
    // event_listeners->tail = node;
    // if (!event_listeners->head) {
        // event_listeners->head = event_listeners->tail;
    // }
    // atomic_fetch_add(&event_listeners->size, 1);
    return &(event_listeners[event_listeners.size() - 1]);
}

void event_relinquish(struct event_listener *listener) {
    OJob::ScopedMutex mutex(&event_listenermutex);
    for (auto it = event_listeners.begin(); it != event_listeners.end(); it++) {
        struct event_listener value = *it;
        if (!memcmp(&value, listener, sizeof(struct event_listener))) {
            it = event_listeners.erase(it); // remove listener from list
        }
    }
}

static void event_listenerwrap(OJob::Job *job) {
    // wrapper function to make it nice and simple for the user (rather than them having to do all this multi-arg shenanigans)
    uint64_t *args = (uint64_t *)job->param;
    struct event_listener *listener = (struct event_listener *)args[0];
    void (*func)(void *self, struct event *event) = listener->func;
    func(listener->self, (struct event *)args[1]);
}

void event_dispatch(struct event *event) {
    // XXX: This should be allocated from the current frame's allocator rather than `new`
    OJob::Counter counter = OJob::Counter(); // No need to use new as the counter will only exist for the duration of the function scope.

    event_listenermutex.lock();

    std::vector<struct event_listener> listeners;
    // struct event_listener *listeners = (struct event_listener *)malloc(sizeof(struct event_listener));
    // size_t listenersidx = 0;
    // while (node) {
        // if (node->data.type == event->type && (node->data.self != NULL ? event->self == node->data.self : true)) {
            // listeners[listenersidx++] = node->data;
            // listeners = (struct event_listener *)realloc(listeners, sizeof(struct event_listener) * (listenersidx + 1));
            // break;
        // }
        // node = node->next;
    // }
    for (auto it = event_listeners.begin(); it != event_listeners.end(); it++) {
        struct event_listener value = *it;
        if (value.type == event->type && (value.self != NULL ? value.self == event->self : true)) {
            listeners.push_back(value);
        }
    }

    if (listeners.size() != 0) {
        for (auto it = listeners.begin(); it != listeners.end(); it++) { // send the event we're dispatching to all listeners
            struct event_listener listener = *it;
            // arguments, the job system only supports one argument so we just pass a pointer to the rest of the arguments in that first argument (stack scope only, will go out of scope on function return (hence why we must wait on the counters before returning, among other reasons))
            uint64_t args[2] = { (uintptr_t)&listener, (uintptr_t)event }; // it's ok to have this in stack scope because we wait for its completion before returning (and thusly this stack data from going out of scope)

            // XXX: Same story with the allocations here
            OJob::Job *job = new OJob::Job(event_listenerwrap, (uintptr_t)args);
            job->counter = &counter;
            job->priority = OJob::Job::PRIORITY_NORMAL; // events are high priority, but we don't want to clog the high priority queue
            OJob::kickjob(job); // kick the job and continue working asynchronously
        }

        event_listenermutex.unlock();

        counter.wait(); // we expect event_dispatch to be run inside a job, we just wait until all the jobs we just scheduled are done
    } else {
        event_listenermutex.unlock();
    }

    // free(event);
    event_allocator.free(event);
}

void event_dispatchevents(uint64_t time) {
    struct event *requeue[1024];
    size_t requeueidx;
    while (true) {
        struct event *event = (struct event *)event_events.pop();
        if (event == NULL) {
            break; // reached the end of the event queue
        }

        if (event->deliverytime <= time) {
            event_dispatch(event); // dispatch the event
        } else {
            requeue[requeueidx++] = event; // we want to queue this again
        }
    }

    // requeue anything we passed over
    for (size_t i = 0; i < requeueidx; i++) {
        event_events.push(requeue[i]);
    }
}

void event_queue(struct event event) {

    // XXX: Pool allocator for events perhaps, would work nicely when we free the event afterwards
    // struct event *ev = (struct event *)malloc(sizeof(struct event));
    struct event *ev = (struct event *)event_allocator.alloc();
    memcpy(ev, &event, sizeof(struct event)); // deep copy
    event_events.push(ev); // queue onto event queue
}

void event_init(void) {
    event_listeners.clear();
}
