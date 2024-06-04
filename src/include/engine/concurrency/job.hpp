#ifndef _ENGINE__CONCURRENCY__JOBS_HPP
#define _ENGINE__CONCURRENCY__JOBS_HPP

#include <atomic>
#include <engine/concurrency/coroutine.hpp>
#include <stddef.h>
#include <stdint.h>
#include <engine/utils.hpp>
#include <engine/utils/queue.hpp>
#include <tracy/Tracy.hpp>
#ifdef __linux__
#include <ucontext.h>
#endif
#include <vector>

namespace OJob {

#define COMPILER_BARRIER() asm volatile("" ::: "memory")

#define JOB_MAXWORKERS 512 // probably unlikely a CPU will ever need 512 cores to run this engine, but this is of course a wild number that has no logical sense in being needed

    class Fibre;
    class Job;

#define JOB_WAITLISTSIZE 16 // 16 jobs may wait on a single counter at any one time (seeing as in all but the most uncommon use cases we'd only need one job to wait on a counter, this works nicely)

    class Counter {
        public:
            std::atomic<ssize_t> ref;
            std::vector<OJob::Fibre *> waitlist;
            // std::vector<OJob::Job *> init;
            pthread_spinlock_t waitlistlock;
            pthread_spinlock_t lock;

            void reference(OJob::Job *job) {
                this->ref.fetch_add(1);
                // init.push_back(job);
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
                STATUS_DONE = 0,
                STATUS_YIELD,
                STATUS_WAIT
            };
            enum priority {
                PRIORITY_NORMAL, // do this at a normal pace please!
                PRIORITY_HIGH, // we need this done now! (this job will be placed into a priority queue and executed as soon as a spot is available)
                PRIORITY_COUNT
            };

            std::atomic<bool> allgood = false;

            OJob::Fibre *fibre = NULL;
            enum priority priority = PRIORITY_NORMAL;
            std::atomic<bool> running = false;
            OJob::Counter *counter = NULL;
            size_t id = 0;
            void (*entry)(OJob::Job *job) = NULL;
            uintptr_t param = 0;

            Job(void (*entry)(OJob::Job *job), uintptr_t param) {
                this->entry = entry;
                this->param = param;
                this->counter = NULL;
                this->fibre = NULL;
                this->id = OJob::id.fetch_add(1);
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
            char *name = NULL;
            size_t id = 0;
            enum OJob::Job::status yieldstatus;
            struct coroutine *co = NULL;
            OJob::Mutex *waitmutex = NULL;
            OJob::Counter *waitfor = NULL;
            std::atomic<OJob::Counter *> tounref = NULL;
            OJob::Job *job = NULL;

            void *stack = NULL;
#ifdef __linux__
            static constexpr ucontext_t INVALIDFIBRE = { };
            ucontext_t ctx = INVALIDFIBRE;
#endif

    };

    extern thread_local OJob::Fibre *currentfibre;

    struct worker {
        int id = 0;
        std::atomic<int> state; // current worker state
        OJob::Fibre *current = NULL; // current fibre
        OJob::Fibre *prev = NULL;
        pthread_t thread; // thread running the worker
        ucontext_t ctx;
    };

    extern thread_local OJob::worker *currentworker;

    extern OUtils::MPMCQueue freefibres;

    extern struct OJob::worker workers[JOB_MAXWORKERS];
    extern size_t numworkers;

    // Jobs are ordered by a queue of priorities
    extern OUtils::MPMCQueue queues[Job::PRIORITY_COUNT];

    // Concurrency synchronisation for when we expect to only wait for small amounts of time with heavy competition.
    class Spinlock {
        private:
            pthread_spinlock_t internal;
        public:
            Spinlock(void) {
                pthread_spin_init(&this->internal, true);
            }

            void lock(void) {
                ZoneScopedN("Spinlock Lock");
                pthread_spin_lock(&this->internal);
            }

            void unlock(void) {
                ZoneScopedN("Spinlock Unlock");
                pthread_spin_unlock(&this->internal);
            }
    };

    // Generally okay for any use case, yields to another job. Ideal for long waits where we'd like to be doing other work in the mean-time.
    class Mutex {
        private:
            std::atomic<bool> ref;
            OJob::Fibre *heldby;
            pthread_spinlock_t spin;
        public:
            std::vector<OJob::Fibre *> waitlist;
            pthread_spinlock_t waitlistlock;

            void lock(void);
            void unlock(void) {
                ZoneScopedN("Mutex Unlock");

                // XXX: Memory issues seem to stem from here.

                // ASSERT(job_currentfibre != NULL, "Mutex unlock outside of job system! Please use pthread spinlocks instead for synchronising current operations outside of Omicron's job system!\n");

                // pthread_spin_unlock(&this->spin);
                // this->ref.store(false);
                // return;
                pthread_spin_lock(&this->waitlistlock);
                if (this->ref.load()) {
                    if (this->waitlist.size() > 0) {
                        // XXX: Note: what if the fibre ends and detaches itself from any jobs before this is called, code is now invalidated.
                        // However, this should never happen as a job inside a waitlist should continue waiting forever or until the mutex is unlocked like it is here.

                        OJob::Fibre *fibre = this->waitlist.back();
                        this->waitlist.pop_back();


                        ASSERT(fibre != NULL, "Invalid fibre on waitlist.\n");
                        ASSERT(fibre->id < FIBRE_COUNT, "Invalid fibre ID %lu. Must be corrupt!.\n", fibre->id);
                        OJob::Job *job = fibre->job;
                        ASSERT(job != NULL, "Invalid job from waitlist for fibre %lu.\n", fibre->id); // A job this fibre encapsulated exited before a waiting fibre was released.
                        ASSERT(job->id < id.load(), "Invalid job ID %lu. Must be corrupt!.\n", fibre->id);

                        // if this is triggered a few things must hold true:
                        // - The fibre is valid.
                        // - The job is valid.
                        // - The fibre pointed to by the job is invalid.
                        ASSERT(job->fibre != NULL, "Invalid fibre for job from waitlist.\n");
                        this->heldby = fibre;
                        // printf("[%ld] rescheduling a job %lu.\n", utils_getcounter(), job->id);

                        COMPILER_BARRIER(); // Keep flow coherent.
                        queues[job->priority].push(job); // Essentially act like a single file line, leading jobs back into the queue one at a time to prevent wasting time rescheduling everything only for one to actually do anything with that opportunity
                    } else {
                        this->heldby = NULL;
                        this->ref.store(false);
                    }
                }
                pthread_spin_unlock(&this->waitlistlock);

            }

            Mutex(void) {
                this->ref.store(false);
                this->waitlist.clear();
                pthread_spin_init(&this->spin, true);
                pthread_spin_init(&this->waitlistlock, true);
            }

            ~Mutex(void) {
                this->waitlist.clear();
                pthread_spin_destroy(&this->waitlistlock);
                // free(this->lockname);
                // free(this->unlockname);
            }
    };

    class ScopedMutex {
        public:
            OJob::Mutex *mutex;

            ScopedMutex(OJob::Mutex *mutex) {
                ASSERT(mutex != NULL, "Invalid reference to mutex.\n");
                this->mutex = mutex;
                this->mutex->lock();
            }

            ~ScopedMutex(void) {
                this->mutex->unlock();
            }
    };

    class ScopedSpinlock {
        public:
            OJob::Spinlock *spin;

        ScopedSpinlock(OJob::Spinlock *spin) {
            ASSERT(spin != NULL, "Invalid reference to spinlock.\n");
            this->spin = spin;
            this->spin->lock();
        }

        ~ScopedSpinlock(void) {
            this->spin->unlock();
        }
    };

    // mutex safe code block (but only if we're in the job system)
#define JOB_MUTEXSAFE(MUTEX, ...) \
        (MUTEX)->lock(); \
        __VA_ARGS__ \
        (MUTEX)->unlock(); \

    // early return (but only if we're in the job system)
#define JOB_MUTEXSAFEEARLY(MUTEX) \
        (MUTEX)->unlock(); \

#define JOB_MUTEXSAFERETURN(MUTEX, VALUE) \
        (MUTEX)->lock(); \
        typeof(VALUE) TEMP = (VALUE); \
        (MUTEX)->unlock(); \
        return TEMP; \

    void kickjob(OJob::Job *job);
    // queue a batch of jobs
    void kickjobs(int count, OJob::Job *jobs[]);
    // call job_kickjob and wait for completion using job_waitcounter
    void kickjobwait(OJob::Job *job);
    // call job_kickjobs and wait for completion using job_waitcounter (expects all jobs to utilise the same counter)
    void kickjobswait(int count, OJob::Job *jobs[]);

    void init(void);
    void destroy(void);

}

#endif
