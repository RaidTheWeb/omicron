#include <concurrency/job.hpp>
#include <pthread.h>
#include <resources/resource.hpp>







// struct resource_ring *resource_gpuresources = NULL;
//
// struct resource_ring *resource_ringinit(size_t capacity) {
//     struct resource_ring *ring = (struct resource_ring *)malloc(sizeof(struct resource_ring));
//     ring->head = NULL;
//     ring->tail = NULL;
//     ring->size.store(0);
//     ring->capacity = capacity;
//     pthread_spin_init(&ring->lock, true);
//     return ring;
// }
//
// RESOURCE_RINGPUSH(resource_gpuringpush, struct resource_gpu, struct resource_gpunode);
// RESOURCE_RINGPOP(resource_gpuringpop, struct resource_gpu, struct resource_gpunode);
// RESOURCE_RINGITER(resource_gpuringiter, struct resource_gpunode);
// RESOURCE_RINGDELETE(resource_gpuringdelete, struct resource_gpu, struct resource_gpunode);
// RESOURCE_RINGDESTROY(resource_gpuringdestroy, struct resource_gpunode);
// RESOURCE_RINGFIND(resource_gpuringfind, struct resource_gpu, struct resource_gpunode, node->data.shader.idx == idx && node->data.type == type, uint16_t idx, size_t type);
//
// void resource_gpufree(struct resource_gpu *rgpu) {
//     ASSERT_NOTFREED(rgpu);
//
//     // ((void (*)(bgfx_shader_handle_t))resource_destroy[rgpu->type])(rgpu->shader);
//     rgpu->freed = true;
//     resource_gpuringdelete(resource_gpuresources, rgpu);
// }
//
// void resource_gpufreeall(void) {
//     printf("Releasing GPU resources...\n");
//     // resource_gpuringiter(resource_gpuresources, (void *)resource_gpufree);
//     resource_gpuringdestroy(resource_gpuresources);
// }
//
// // typeof(resource_cpuresources) resource_cpuresources = VECTOR_INIT;
//
// void resource_cpufree(struct resource_cpu rcpu) {
//     ASSERT_NOTFREED(&rcpu);
//     rcpu.freed = true;
//     free(rcpu.memory);
// }
//
// void resource_cpufreeall(void) {
//     // VECTOR_FOR_EACH(&resource_cpuresources, it,
//         // struct resource_cpu rcpu = *it;
//         // resource_cpufree(rcpu);
//     // );
// }
//
// static OJob::Mutex resource_kickmutex = { };
//
// static void resource_gpufreecheck(struct resource_gpu *data) {
//     if (!data->lock.ref && data->lock.life == 0) {
//         resource_gpufree(data);
//         return;
//     }
//
//     if (!data->lock.ref && data->lock.life > 0) {
//         data->lock.life--; // frame lifetime
//     }
// }
//
// static void resource_freemarkedjob(struct job_jobdecl *job) {
//     // funnily enough, mutexes work perfectly and exactly as I hoped they would.
//     resource_kickmutex.lock();
//     // resource_gpuringiter(resource_gpuresources, (void *)resource_gpufreecheck);
//     resource_kickmutex.unlock();
// }
//
// void resource_kick(void) {
//     // struct job_jobdecl *decl = job_initjob(resource_freemarkedjob, 0);
//     // decl->priority = JOB_PRIORITYLOW; // low priority, we don't really care how often our resources get freed, they're declared as useless anyway (and we don't want to hog the job queue from more important operations)
//     // decl->counter = NULL;
//     // job_kickjob(decl); // don't bother with waiting for job completion (asynchronous job)
// }
//
// void resource_init(void) {
//     resource_gpuresources = resource_ringinit(16384);
// }
