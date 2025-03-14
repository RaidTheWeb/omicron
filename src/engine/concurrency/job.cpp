#include <common/TracySystem.hpp>
#include <engine/concurrency/job.hpp>
#include <errno.h>
#include <engine/utils/memory.hpp>
#include <unistd.h>

// XXX: This is not OS agnostic!!!!!
// XXX: Super high CPU usage! (Figure out how to prevent problems such as this)
// Try use pthread signals for worker queue notify

namespace OJob {



    // 15872 possible normal priority jobs, 512 high priority jobs (we expect there to be less of them).
    // consider increasing these queue sizes
    OUtils::MPMCQueue queues[OJob::Job::PRIORITY_COUNT] = { OUtils::MPMCQueue(16384), OUtils::MPMCQueue(512) };
    OUtils::MPMCQueue freefibres = OUtils::MPMCQueue(FIBRE_COUNT);

    pthread_spinlock_t spin;
    struct OJob::worker workers[JOB_MAXWORKERS];
    size_t numworkers = 2;

    OUtils::PoolAllocator allocator = OUtils::PoolAllocator(sizeof(class Job), 16896, MEMORY_POOLALLOCEXPAND, "Jobs"); // we have a memory pool equal to the queue size for jobs

    void *Job::operator new(size_t _) {
        (void)_;
        void *allocated = allocator.alloc();
        return allocated;
    }

    void Job::operator delete(void *ptr) {
        allocator.free((OJob::Job *)ptr);
    }

    static OJob::Job *findjob(void) {

        OJob::Job *job = (OJob::Job *)OJob::queues[OJob::Job::PRIORITY_HIGH].pop();
        if (job == NULL) {
            job = (OJob::Job *)OJob::queues[OJob::Job::PRIORITY_NORMAL].pop();
            if (job == NULL) {
                return NULL;
            }
        }

        return job;
    }

    static OJob::Job *getnextjob(void) {
        // step through job queues to find a job by order of priority
        OJob::Job *job = OJob::findjob();
        if (job == NULL) {
            ASSERT(false, "Job queue empty when job is requested!\n"); // this shouldn't occur since the condition will only work when there is a job available
        }
        return job;
    }

    // pthread mutexes and conditions to help reduce CPU usage on the worker threads
    pthread_cond_t availablecond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t availablemutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t fibrecond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t fibremutex = PTHREAD_MUTEX_INITIALIZER;

    static OJob::Fibre *getfibre(void) {
        // XXX Takes a bit too long sometimes.
        ZoneScopedN("Get Free Fibre");

        OJob::Fibre *fibre = NULL;
        pthread_mutex_lock(&OJob::fibremutex);
        while ((fibre = (OJob::Fibre *)OJob::freefibres.pop()) == NULL) {
            TracyMessageL("Hanging while waiting for free fibres");
            pthread_cond_wait(&OJob::fibrecond, &OJob::fibremutex);
        }
        pthread_mutex_unlock(&OJob::fibremutex);

        return fibre;
    }

    thread_local OJob::worker *currentworker = NULL;

    void yield(OJob::Fibre *fibre, enum OJob::Job::status status) {
        // set reason for yield and return to worker thread
        fibre->yieldstatus = status;

        COMPILER_BARRIER();
        ASSERT(OJob::currentfibre != NULL, "Yield called outside of job system or with invalid fibre.\n");
        ASSERT(OJob::currentworker != NULL, "Yield called outside of job system or with invalid worker.\n");
        ASSERT(OJob::currentworker->ctx.uc_mcontext.fpregs->rip != 0, "Invalid RIP.\n");
        swapcontext(&OJob::currentfibre->ctx, &OJob::currentworker->ctx); // Swap to worker context.
    }

    void kickjob(OJob::Job *job) {
        // ZoneScoped;
        ASSERT(job->priority < Job::PRIORITY_COUNT, "Invalid job priority %u.\n", job->priority);

        if (job->counter != NULL) {
            job->counter->reference();
            pthread_spin_trylock(&job->counter->lock);
        }

        pthread_mutex_lock(&OJob::availablemutex);
        OJob::queues[job->priority].push(job);
        pthread_cond_broadcast(&OJob::availablecond);
        pthread_mutex_unlock(&OJob::availablemutex);
    }

    void kickjobs(int count, OJob::Job *jobs[]) {
        ASSERT(count > 0, "Kicking zero jobs!\n");
        for (int i = 0; i < count; i++) {
            OJob::kickjob(jobs[i]);
        }
    }

    void Counter::unreference(void) {
        ZoneScopedN("Counter Unreference");
        pthread_spin_lock(&this->waitlistlock);
        this->ref.fetch_sub(1);


        COMPILER_BARRIER();
        if (this->ref.load() <= 0) {
            for (size_t i = 0; i < this->waitlist.size(); i++) {
                OJob::Fibre **it = &this->waitlist[i];
                ASSERT((*it) != NULL, "Invalid fibre on queue.\n");
                ASSERT((*it)->job != NULL, "Fibre with invalid job on queue.\n");
                ASSERT((*it)->job->priority < Job::PRIORITY_COUNT, "Fibre with invalid job priority.\n");
                OJob::queues[(*it)->job->priority].push((*it)->job);
            }

            pthread_spin_unlock(&this->lock); // only unlock the lock when we're done so in the tiny amount of time between unlocking starting the waitlist iteration we don't overwrite the counter memory (if of course it is deleted)
        }
        pthread_spin_unlock(&this->waitlistlock);
    }

    void Counter::wait(void) {
        OJob::Job *job = OJob::currentfibre != NULL ? OJob::currentfibre->job : NULL;
        pthread_spin_lock(&this->waitlistlock);
        if (this->ref.load() <= 0) {
            pthread_spin_unlock(&this->waitlistlock);
            return;
        }

        if (job != NULL) {
            ASSERT(job->fibre == OJob::currentfibre, "Fibre of active job does not match the current fibre.\n");
            job->fibre->waitfor = this;

            COMPILER_BARRIER();
            OJob::yield(job->fibre, Job::STATUS_WAIT);
        } else {
            pthread_spin_unlock(&this->waitlistlock);

            COMPILER_BARRIER();
            pthread_spin_lock(&this->lock);
            pthread_spin_unlock(&this->lock);
        }
    }

    void Semaphore::wait(void) {
        this->counter.wait(); // Just make us wait on a counter.
    }

    void Semaphore::trigger(void) {
        this->counter.unreference(); // Unreferencing here will trigger a wake up on all waiting jobs.
    }

    std::atomic<size_t> mutexid;

    void Mutex::lock(void) {
        // XXX: We can't use tracy scoped zoning on anything that yields mid way through or it'll mess up the fibre!.
        if (OJob::currentfibre == NULL) { // experimental outside-of-job-system mutex usage (doesn't even act as a mutex, just a simple spin lock)
            bool expected = false;
            // Ensure exclusive access, spurious failure lets us reduce power consumption during this.
            // NOTE: I was a little silly here earlier and forgot that this will return false whenever we fail, this meant I was letting through operations that should otherwise be mutex locked if attempting to acquire them on the main thread.
            while (!this->ref.compare_exchange_weak(expected, true)) { // busy wait until ref is unlocked, then immediately acquire it.
                expected = false;
                asm ("pause"); // Try to reduce load on the CPU during this (XXX: not non-x86 friendly!).
            }
            pthread_spin_lock(&this->waitlistlock);
            this->heldby = NULL; // no one, but this doesn't matter
            pthread_spin_unlock(&this->waitlistlock);
            return;
        }

        pthread_spin_lock(&this->waitlistlock);
        COMPILER_BARRIER();
        if (this->ref.load()) {
            currentfibre->waitmutex = this;

            COMPILER_BARRIER();
            OJob::yield(currentfibre, Job::STATUS_WAIT);
        } else {
            this->ref.store(true);
            this->heldby = currentfibre;
            pthread_spin_unlock(&this->waitlistlock);
        }
    }

    void kickjobwait(OJob::Job *job) {
        ASSERT(job->counter != NULL, "Attempting to kick and wait on a job with no counter.\n");
        OJob::Counter *counter = job->counter; // XXX: Store a reference to the counter before kicking the job! Knowing that we invalidate the job pointer memory when the job completes offers a possibility for job->counter to be invalidated if the calling thread yields for long enough for the worker thread to complete the job. Good practise is to write code to prevent this from happening.
        OJob::kickjob(job);
        counter->wait();
    }

    void kickjobswait(int count, OJob::Job *jobs[]) {
        ASSERT(count > 0, "Kicking zero jobs!\n");
        OJob::Counter *initcounter = jobs[0]->counter;
        ASSERT(initcounter != NULL, "Attempting to kick a number of jobs and wait with no counter.\n");
        for (int i = 1; i < count; i++) { // check if jobs all use the same counter
            ASSERT(jobs[i]->counter == initcounter, "Expected jobs to all use the same counter!\n");
        }
        OJob::kickjobs(count, jobs);
        initcounter->wait();
    }

    [[noreturn]] static void fibre(OJob::Fibre *fibre) {
        for (;;) {
            OJob::Job *job = fibre->job;
            ASSERT(job != NULL, "Fibre %lu being run without a job!\n", fibre->id);
            ASSERT(job->entry != NULL, "Fibre %lu's job %lu references NULL function entry.\n", fibre->id, job->id);
            job->entry(job);

            fibre->tounref.store(job->counter); // Inform the worker we need to unreference this counter.

            job->allgood.store(true);
            // XXX: Stack smashing here?
            if (!job->returns) { // only delete the job if it returns, otherwise, we can return it later.
                delete job; // Early delete job before yielding.
            }

            OJob::yield(fibre, Job::STATUS_DONE); // yield to scheduler marking job as done
        }
        __builtin_unreachable();
    }

    thread_local Fibre *currentfibre = NULL;

    [[noreturn]] static void workerthread(int *id) {
        // ZoneScoped;
        char *name = (char *)malloc(32);
        snprintf(name, 32, "Worker Thread %u", *id);
        tracy::SetThreadName(name);

        struct OJob::worker *worker = &OJob::workers[*id];
        OJob::currentworker = worker;
        for (;;) {
            getcontext(&worker->ctx);
            // acquire lock for job schedule
            OJob::Job *decl = NULL;
            // we HAVE to use a pthread mutex here because polling otherwise causes major CPU busy usage
            // NEVER use a pthread mutex when using the job system as it'll only yield to another worker thread, not another job.
            pthread_mutex_lock(&OJob::availablemutex);
            while ((decl = OJob::findjob()) == NULL) {
                pthread_cond_wait(&OJob::availablecond, &OJob::availablemutex);
            }
            pthread_mutex_unlock(&OJob::availablemutex);
            ASSERT(decl != NULL, "Job queues are empty yet availability check signalled they weren't\n");
            ASSERT(!decl->allgood.load(), "Rescheduling an already completed job %lu.\n", decl->id);

            OJob::Fibre *fibre = NULL;
            if (decl->fibre != NULL) {
                fibre = decl->fibre; // Resume with existing encapsulating fibre.
            } else {
                fibre = OJob::getfibre();
            }
            ASSERT(fibre != NULL, "Attempted to use a NULL fibre.\n");
            ASSERT(decl->fibre != NULL ? true : fibre->job == NULL, "Fibre is being rescheduled (check code for potentially recursive job kick).\n");
            fibre->job = decl;
            decl->fibre = fibre;

            // Reset waiting stats.
            fibre->waitmutex = NULL;
            fibre->waitfor = NULL;

            OJob::currentfibre = fibre;
            TracyFiberEnter(fibre->name);

            // XXX: non-x86 hostile.
            ASSERT(fibre->ctx.uc_mcontext.fpregs->rip != 0, "Invalid RIP.\n");
            swapcontext(&worker->ctx, &fibre->ctx);
            TracyFiberLeave;

            worker->prev = fibre; // Establish what was our previous fibre.
            OJob::currentfibre = NULL; // thread-local. no contention.

            // all job and fibre cleanup code past this point (helps prevent resume-before-yield errors)

            if (fibre->yieldstatus != OJob::Job::STATUS_DONE) { // don't modify anything in here if the job is marked as done (we'll get a sigsegv as we already clean up all this stuff)
                decl->running.store(false); // Mark as no longer running (suspended)
            }

            if (fibre->yieldstatus == OJob::Job::STATUS_YIELD) {
                // this was pretty evil of me. this is being pushed back and yet isn't finished?
                OJob::queues[decl->priority].push(decl); // Push the job to the back of the queue, the idea being that it'll only be picked back up later.
            }

            if (fibre->yieldstatus == OJob::Job::STATUS_WAIT) {
                if (fibre->waitfor != NULL) { // waiting on counter
                    OJob::Counter *waiting = fibre->waitfor;
                    fibre->waitfor = NULL; // Declare fibre is no longer waiting on anything.

                    COMPILER_BARRIER();
                    waiting->waitlist.push_back(fibre);
                    pthread_spin_unlock(&waiting->waitlistlock);
                } else if (fibre->waitmutex != NULL) { // waiting on mutex
                    OJob::Mutex *mutex = fibre->waitmutex;
                    fibre->waitmutex = NULL; // Declare fibre is no longer waiting on anything.

                    COMPILER_BARRIER();
                    mutex->waitlist.push_back(fibre);
                    pthread_spin_unlock(&mutex->waitlistlock);
                }
            }

            if (fibre->yieldstatus == OJob::Job::STATUS_DONE) {
                if (fibre->tounref != NULL) {
                    fibre->tounref.load()->unreference(); // unreference when done.
                    fibre->tounref.store(NULL);
                }
                fibre->job = NULL;
                pthread_mutex_lock(&OJob::fibremutex);
                // Push fibre back to job list.
                COMPILER_BARRIER();
                OJob::freefibres.push(fibre); // only push after the coroutine exits, this way the fibre doesn't get acquired before the coroutine is properly yielded
                pthread_cond_broadcast(&OJob::fibrecond);
                pthread_mutex_unlock(&OJob::fibremutex);
            }
        }
        __builtin_unreachable();
    }

    void destroy(void) {
#ifdef __unix__
        for (size_t i = 0; i < numworkers; i++) {
            pthread_cancel(OJob::workers[i].thread);
            // TODO: Core affinity
        }
#endif
        pthread_mutex_destroy(&availablemutex);
        pthread_mutex_destroy(&fibremutex);

        OJob::Fibre *fibre = NULL;
        while ((fibre = (OJob::Fibre *)OJob::freefibres.pop()) != NULL) {
            // coroutine_destroy(fibre->co);
            free(fibre->stack);
            free(fibre->name);
            delete fibre;
        }
    }

    std::atomic<size_t> id;

    // TODO: Jobify the engine!!!!!
    // as much as possible should be shoved into jobs so we can be concurrent
    // work asynchronously whenever possible
    void init(void) {
        OJob::id.store(0);

        pthread_spin_init(&OJob::spin, true);

        for (size_t i = 0; i < FIBRE_COUNT; i++) {
            OJob::Fibre *fibre = new OJob::Fibre(); // we can afford to use the heap here as we only ever allocate this once
            fibre->id = i;
            fibre->name = (char *)malloc(16);
            ASSERT(fibre->name != NULL, "Failed to allocate memory for fibre debug name.\n");
            snprintf(fibre->name, 32, "Fibre %lu", fibre->id);

            getcontext(&fibre->ctx);

            fibre->stack = (uint8_t *)malloc(64 * 1024 * 1024); // 64KB
            ASSERT(fibre->stack != NULL, "Failed to allocate stack for fibre context.\n");
            fibre->ctx.uc_stack.ss_sp = fibre->stack;
            fibre->ctx.uc_stack.ss_size = 64 * 1024 * 1024;
            fibre->ctx.uc_link = NULL; // No return is ever expected.
            makecontext(&fibre->ctx, (void (*)())OJob::fibre, 1, fibre);
            OJob::freefibres.push(fibre);
        }

        for (size_t i = 0; i < JOB_MAXWORKERS; i++) {
            OJob::workers[i].id = i;
        }

#ifdef __linux__
        numworkers = sysconf(_SC_NPROCESSORS_ONLN);
#endif

        printf("Job system initialised with %lu worker thread(s)\n", numworkers);

#ifdef __unix__
        for (size_t i = 0; i < numworkers; i++) {
            pthread_create(&OJob::workers[i].thread, NULL, (void *(*)(void *))OJob::workerthread, &OJob::workers[i].id);
            // TODO: Core affinity
        }
#endif
    }

}
