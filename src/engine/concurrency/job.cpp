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

    OUtils::PoolAllocator allocator = OUtils::PoolAllocator(sizeof(class Job), 16896, MEMORY_POOLALLOCEXPAND, "Jobs"); // we have a memory pool equal to the queue size for jobs

    void *Job::operator new(size_t _) {
        (void)_;
        return allocator.alloc();
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
        OJob::Fibre *fibre = NULL;
        pthread_mutex_lock(&OJob::fibremutex);
        while ((fibre = (OJob::Fibre *)OJob::freefibres.pop()) == NULL) {
            pthread_cond_wait(&OJob::fibrecond, &OJob::fibremutex);
        }
        pthread_mutex_unlock(&OJob::fibremutex);

        return fibre;
    }

    void yield(OJob::Fibre *fibre, enum OJob::Job::status status) {
        // set reason for yield and return to worker thread
        fibre->yieldstatus = status;
        coroutine_yield(fibre->co);
    }

    void kickjob(OJob::Job *job) {
        // ZoneScoped;
        ASSERT(job->priority < Job::PRIORITY_COUNT, "Invalid job priority %u.\n", job->priority);

        if (job->counter) {
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
        for (size_t i = 0; i < count; i++) {
            OJob::kickjob(jobs[i]);
        }
    }

    void Counter::unreference(void) {
        this->ref.fetch_sub(1);

        if (this->ref.load() <= 0) {
            pthread_spin_unlock(&this->lock);

            pthread_spin_lock(&this->waitlistlock);
            for (size_t i = 0; i < this->waitlist.size(); i++) {
                OJob::queues[this->waitlist[i]->job->priority].push(this->waitlist[i]->job);
            }
            pthread_spin_unlock(&this->waitlistlock);
        }
    }

    void Counter::wait(void) {
        OJob::Job *job = OJob::currentfibre != NULL ? OJob::currentfibre->job : NULL;
        if (this->ref.load() == 0) {
            return;
        }

        if (job != NULL) {
            pthread_spin_lock(&this->waitlistlock);
            job->fibre->waitfor = this;
            OJob::yield(job->fibre, Job::STATUS_WAIT);
        } else {
            pthread_spin_lock(&this->lock);
            pthread_spin_unlock(&this->lock);
        }
    }

    void kickjobwait(OJob::Job *job) {
        OJob::kickjob(job);
        job->counter->wait();
    }

    void kickjobswait(int count, OJob::Job *jobs[]) {
        ASSERT(count > 0, "Kicking zero jobs!\n");
        OJob::Counter *initcounter = jobs[0]->counter;
        for (size_t i = 1; i < count; i++) { // check if jobs all use the same counter
            ASSERT(jobs[i]->counter == initcounter, "Expected jobs to all use the same counter!\n");
        }
        OJob::kickjobs(count, jobs);
        initcounter->wait();
    }

    [[noreturn]] static void fibre(struct coroutine *co) {
        OJob::Fibre *fibre = (OJob::Fibre *)co->userdata;
        for (;;) {
            OJob::Job *job = fibre->job;
            ASSERT(job != NULL, "Fibre %lu being run without a job!\n", fibre->id);
            ASSERT(job->entry != NULL, "Fibre %lu's job %lu references NULL function entry.\n", fibre->id, job->id);
            job->entry(job);
            delete job;
            OJob::yield(fibre, Job::STATUS_DONE); // yield to scheduler marking job as done
        }
        __builtin_unreachable();
    }

    thread_local Fibre *currentfibre;

    [[noreturn]] static void workerthread(int *id) {
        // ZoneScoped;
        char *name = (char *)malloc(32);
        snprintf(name, 32, "Worker Thread %u", *id);
        tracy::SetThreadName(name);

        struct OJob::worker *worker = &OJob::workers[*id];
        for (;;) {
            // acquire lock for job schedule
            OJob::Job *decl;
            // we HAVE to use a pthread mutex here because polling otherwise causes major CPU busy usage
            // NEVER use a pthread mutex when using the job system as it'll only yield to another worker thread, not another job.
            pthread_mutex_lock(&OJob::availablemutex);
            while ((decl = OJob::findjob()) == NULL) {
                pthread_cond_wait(&OJob::availablecond, &OJob::availablemutex);
            }
            pthread_mutex_unlock(&OJob::availablemutex);
            ASSERT(decl != NULL, "Job queues are empty yet availability check signalled they weren't\n");

            OJob::Fibre *fibre = decl->fibre != NULL ? decl->fibre : OJob::getfibre();
            ASSERT(decl->fibre != NULL ? true : fibre->job == NULL, "Fibre is being rescheduled (check code for potentially recursive job kick).\n");
            fibre->job = decl;
            decl->fibre = fibre;

            OJob::currentfibre = fibre;
            TracyFiberEnter(fibre->name);
            coroutine_resume(fibre->co);
            TracyFiberLeave;
            OJob::currentfibre = NULL;

            // all job and fibre cleanup code past this point (helps prevent resume-before-yield errors)

            if (fibre->yieldstatus != OJob::Job::STATUS_DONE) { // don't modify anything in here if the job is marked as done (we'll get a sigsegv as we already clean up all this stuff)
                decl->running.store(false);
            }

            if (fibre->yieldstatus == OJob::Job::STATUS_YIELD) {
                OJob::queues[decl->priority].push(decl);
            }

            if (fibre->yieldstatus == OJob::Job::STATUS_WAIT) {
                if (fibre->waitfor) { // cleanup, queue fibre for counter requeue
                    OJob::Counter *waiting = fibre->waitfor;
                    waiting->waitlist.push_back(fibre);
                    fibre->waitfor = NULL;
                    pthread_spin_unlock(&waiting->waitlistlock);
                } else if (fibre->yieldstatus == OJob::Job::STATUS_WAIT && fibre->waitmutex) {
                    OJob::Mutex *mutex = fibre->waitmutex;
                    mutex->waitlist.push_back(fibre);
                    fibre->waitmutex = NULL;
                    pthread_spin_unlock(&mutex->waitlistlock);
                }
            }

            if (fibre->yieldstatus == OJob::Job::STATUS_DONE) {
                if (fibre->job->counter != NULL) {
                    fibre->job->counter->unreference(); // unreference when done.
                }
                fibre->job = NULL;
                pthread_mutex_lock(&OJob::fibremutex);
                OJob::freefibres.push(fibre); // only push after the coroutine exits, this way the fibre doesn't get acquired before the coroutine is properly yielded
                pthread_cond_broadcast(&OJob::fibrecond);
                pthread_mutex_unlock(&OJob::fibremutex);
            }
        }
        __builtin_unreachable();
    }

    void destroy(void) {
        int numcores = 1;
#ifdef __linux__
        numcores = sysconf(_SC_NPROCESSORS_ONLN);
#endif

#ifdef __unix__
        for (size_t i = 0; i < numcores; i++) {
            pthread_cancel(OJob::workers[i].thread);
            // TODO: Core affinity
        }
#endif
        pthread_mutex_destroy(&availablemutex);
        pthread_mutex_destroy(&fibremutex);

        OJob::Fibre *fibre = NULL;
        while ((fibre = (OJob::Fibre *)OJob::freefibres.pop()) != NULL) {
            coroutine_destroy(fibre->co);
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
            OJob::Fibre *fibre = new OJob::Fibre(); // we can afford to use the heap here as we only ever allocate this
            struct coroutine_desc desc = coroutine_initdesc(OJob::fibre, 0);
            desc.userdata = fibre;
            fibre->id = i;
            fibre->name = (char *)malloc(32);
            snprintf(fibre->name, 32, "Fibre %lu\n", fibre->id);
            struct coroutine *co;
            int res = coroutine_create(&co, &desc);
            ASSERT(!res, "Coroutine could not be created.\n");
            fibre->co = co;
            OJob::freefibres.push(fibre);
        }

        for (size_t i = 0; i < JOB_MAXWORKERS; i++) {
            OJob::workers[i].id = i;
        }

        int numcores = 1;
#ifdef __linux__
        numcores = sysconf(_SC_NPROCESSORS_ONLN);
#endif

        printf("Job system initialised with %u worker thread(s)\n", numcores);

#ifdef __unix__
        for (size_t i = 0; i < numcores; i++) {
            pthread_create(&OJob::workers[i].thread, NULL, (void *(*)(void *))OJob::workerthread, &OJob::workers[i].id);
            // TODO: Core affinity
        }
#endif
    }

}
