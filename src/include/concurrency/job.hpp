#ifndef _CONCURRENCY__JOBS_HPP
#define _CONCURRENCY__JOBS_HPP

#include <atomic>
#include <concurrency/coroutine.hpp>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <utils.hpp>
#include <utils/queue.hpp>
#include <vector>

namespace OJob {

#define JOB_MAXWORKERS 512 // probably unlikely a CPU will ever need 512 cores to run this engine, but this is of course a wild number that has no logical sense in becoming needed

    class Fibre;

#define JOB_WAITLISTSIZE 16 // 16 jobs may wait on a single counter at any one time (seeing as in all but the most uncommon use cases we'd only need one job to wait on a counter, this works nicely)

    class Counter {
        public:
            std::atomic<size_t> ref;
            std::vector<OJob::Fibre *> waitlist;
            pthread_spinlock_t waitlistlock;
            pthread_spinlock_t lock;

            void reference(void) {
                this->ref.fetch_add(1);
            }
            void unreference(void);
            void wait(void);

            Counter(void) {
                this->ref.store(0);
                this->waitlist.clear();
                pthread_spin_init(&this->lock, true);
                pthread_spin_init(&this->waitlistlock, true);
            }
            ~Counter(void) {
                pthread_spin_destroy(&this->lock);
                pthread_spin_destroy(&this->waitlistlock);
            }
    };

    extern std::atomic<size_t> id;

    class Job {
        public:
            enum status {
                STATUS_DONE,
                STATUS_YIELD,
                STATUS_WAIT
            };
            enum priority {
                PRIORITY_NORMAL, // do this at a normal pace please!
                PRIORITY_HIGH, // we need this done now!
                PRIORITY_COUNT
            };

            OJob::Fibre *fibre;
            enum priority priority = PRIORITY_NORMAL;
            std::atomic<bool> running;
            std::atomic<bool> waiting;
            std::atomic<bool> done;
            OJob::Counter *counter;
            size_t id;
            pthread_spinlock_t lock;
            std::atomic<int> state;
            void (*entry)(OJob::Job *job);
            uintptr_t param;

            Job(void (*entry)(OJob::Job *job), uintptr_t param) {
                this->waiting.store(false);
                this->running.store(false);
                this->done.store(false);

                this->entry = entry;
                this->param = param;
                this->counter = NULL;
                this->fibre = NULL;
                this->id = OJob::id.fetch_add(1);
                pthread_spin_init(&this->lock, true);
            }
            ~Job(void) {
                pthread_spin_destroy(&this->lock);
            }

            // Overrides so that we use the pool allocator rather than
            void *operator new(size_t _);
            void operator delete(void *ptr);

    };

#define FIBRE_NAMELEN 64
#define FIBRE_COUNT 128 // total fibres

    class Mutex;

    class Fibre {
        public:
            size_t id;
            enum OJob::Job::status yieldstatus;
            struct coroutine *co;
            OJob::Mutex *waitmutex;
            OJob::Counter *waitfor;
            OJob::Job *job;
    };

    extern thread_local OJob::Fibre *currentfibre;

    struct worker {
        int id;
        std::atomic<int> state; // current worker state
        OJob::Fibre *current; // current fibre
        pthread_t thread; // thread running the worker
    };

    extern OUtils::MPMCQueue freefibres;

    extern struct OJob::worker workers[JOB_MAXWORKERS];


    // Jobs are ordered by a queue of priorities
    extern OUtils::MPMCQueue queues[Job::PRIORITY_COUNT];

    class Mutex {
        private:
            std::atomic<bool> ref;
            OJob::Fibre *heldby;
        public:
            std::vector<OJob::Fibre *> waitlist;
            pthread_spinlock_t waitlistlock;

            void lock(void) {
                // ASSERT(job_currentfibre != NULL, "Mutex lock outside of job system! Please use pthread spinlocks instead for synchronising current operations outside of Omicron's job system!\n");

                if (currentfibre == NULL) { // experimental outside-of-job-system mutex usage
                    while (this->ref.load()) { // busy wait while ref is locked
                        ;
                    }
                    this->ref.store(true); // claim the mutex
                    pthread_spin_lock(&this->waitlistlock);
                    this->heldby = NULL; // no one, but this doesn't matter
                    pthread_spin_unlock(&this->waitlistlock);
                    return;
                }


                if (this->ref.load()) {
                    pthread_spin_lock(&this->waitlistlock);
                    currentfibre->waitmutex = this;
                    pthread_spin_unlock(&this->waitlistlock);
                } else {
                    this->ref.store(true);
                    pthread_spin_lock(&this->waitlistlock);
                    this->heldby = currentfibre;
                    pthread_spin_unlock(&this->waitlistlock);
                }
            }
            void unlock(void) {
                // ASSERT(job_currentfibre != NULL, "Mutex unlock outside of job system! Please use pthread spinlocks instead for synchronising current operations outside of Omicron's job system!\n");
                if (this->ref.load()) {
                    pthread_spin_lock(&this->waitlistlock);
                    if (this->waitlist.size() != 0) {
                        OJob::Job *job = this->waitlist.back()->job;
                        this->heldby = job->fibre;
                        queues[job->priority].push(job);
                        this->waitlist.pop_back();
                    } else {
                        this->heldby = NULL;
                        this->ref.store(false);
                    }
                    pthread_spin_unlock(&this->waitlistlock);
                }

            }

            Mutex(void) {
                this->ref.store(false);
                this->waitlist.clear();
                pthread_spin_init(&this->waitlistlock, true);
            }

            ~Mutex(void) {
                this->waitlist.clear();
                pthread_spin_destroy(&this->waitlistlock);
            }
    };

    // mutex safe code block (but only if we're in the job system)
#define JOB_MUTEXSAFE(MUTEX, ...) \
        if (OJob::currentfibre) { \
            (MUTEX)->lock(); \
        } \
        __VA_ARGS__ \
        if (OJob::currentfibre) { \
            (MUTEX)->unlock(); \
        }

    // early return (but only if we're in the job system)
#define JOB_MUTEXSAFEEARLY(MUTEX) \
        if (OJob::currentfibre) { \
            (MUTEX)->unlock(); \
        }

    void kickjob(OJob::Job *job);
    // queue a batch of jobs
    void kickjobs(int count, OJob::Job *jobs[]);
    // call job_kickjob and wait for completion using job_waitcounter
    void kickjobwait(OJob::Job *job);
    // call job_kickjobs and wait for completion using job_waitcounter (expects all jobs to utilise the same counter)
    void kickjobswait(int count, OJob::Job *jobs[]);

    void init(void);

}

#endif
