#ifndef _ENGINE__RESOURCES__RESOURCE_HPP
#define _ENGINE__RESOURCES__RESOURCE_HPP

#include <engine/assertion.hpp>
#include <engine/concurrency/job.hpp>
#include <map>
#include <engine/resources/rpak.hpp>
#include <engine/utils.hpp>
#include <engine/utils/pointers.hpp>

namespace OResource {
    class Resource;

#define RESOURCE_INVALIDHANDLE OUtils::Handle<OResource::Resource>(NULL, SIZE_MAX, SIZE_MAX)

    class ResourceManager {
        public:
            OJob::Mutex mutex; // We expect little contention but long waits on resources.

            // map hash of path to resources
            std::unordered_map<uint32_t, Resource *> resources;
            OUtils::ResolutionTable table = OUtils::ResolutionTable(8192);
            std::atomic<size_t> idcounter = 1;

            // load from RPak
            void loadrpak(RPak *rpak);
            void create(const char *path);
            void create(const char *path, void *src);

            OUtils::Handle<Resource> get(const char *path);
    };

    extern ResourceManager manager;

    // XXX: Rework resource system:
    // We actually care about *loading* stuff rather than just knowing where things are.
    class Resource {
        private:
            size_t handle = 0; // handle index for speedy creation of handles
        public:
            enum srctype {
                SOURCE_RPAK, // from an RPak
                SOURCE_VIRTUAL, // not actually a real file (possibly yet)
                SOURCE_OSFS // operating system's filesystem
            };

            OJob::Mutex mutex; // only one thread may "own" the resource at any one time. use mutex here instead of spinlock as we can expect long busy-waits if we were to use a spinlock instead.

            enum srctype type;
            RPak *rpak; // RPak
            struct RPak::tableentry rpakentry; // RPak entry header
            void *ptr; // virtual is a pointer to whatever source
            const char *path; // path to file (either os filesystem or RPak)
            size_t id; // unique resource ID.

            Resource() {
                this->handle = manager.table.bind(this);
                this->id = manager.idcounter.fetch_add(1);
            }
            Resource(RPak *rpak, const char *path) {
                this->rpak = rpak;
                this->path = path;
                this->type = SOURCE_RPAK;
                this->handle = manager.table.bind(this);
                this->id = manager.idcounter.fetch_add(1);
            }
            Resource(const char *path) {
                this->path = path;
                this->type = SOURCE_OSFS;
                this->handle = manager.table.bind(this);
                this->id = manager.idcounter.fetch_add(1);
            }
            Resource(const char *path, void *src) {
                this->path = path;
                this->ptr = src;
                this->type = SOURCE_VIRTUAL;
                this->handle = manager.table.bind(this);
                this->id = manager.idcounter.fetch_add(1);
            }

            // Claim exclusive access on active worker/thread.
            void claim(void) {
                this->mutex.lock();
            }

            // Release exclusive access on active worker/thread.
            void release(void) {
                this->mutex.unlock();
            }

            // Reinterpret resource as that of one pointing to a virtual resource of whatever type.
            template <typename T>
            T *as(void) {
                return (T *)this->ptr;
            }

            OUtils::Handle<Resource> gethandle(void) {
                return OUtils::Handle<Resource>(&manager.table, this->handle, this->id);
            }
    };

    // Guarantee ownership and access to a resource.
#define RESOURCE_GUARANTEE(RESOURCE, ...) \
    ASSERT((RESOURCE).isvalid(), "Invalid resource handle passed to guarantee macro.\n"); \
    (RESOURCE)->claim(); \
    __VA_ARGS__ \
    (RESOURCE)->release();

}


// // TODO: Occlusion query and other GPU resources we haven't quite yet tied up here
// // type of GPU resource
// enum {
//     GPU_DYNAMIC_INDEX,
//     GPU_DYNAMIC_VERTEX,
//     GPU_FRAMEBUFFER,
//     GPU_INDEX,
//     GPU_PROGRAM,
//     GPU_SHADER,
//     GPU_TEXTURE,
//     GPU_UNIFORM,
//     GPU_VERTEX
// };
//
// struct resource_ring {
//     void *head;
//     void *tail;
//     std::atomic<size_t> size;
//     size_t capacity;
//     OJob::Mutex mutex;
//     pthread_spinlock_t lock; // TODO: Use mutexes instead of spinlocks? spinlocks at the moment are perfectly fine as these spinlocks aren't used for waiting for long times (so the CPU isn't sitting in busy-wait for ages).
// };
//
// #define RESOURCE_RINGNODE(s, t) t { \
//     s data; \
//     t *next; \
// }
//
// #define RESOURCE_RINGPUSHDEF(n, s) s *n(struct resource_ring *ring, s data)
// #define RESOURCE_RINGPUSH(n, s, t) s *n(struct resource_ring *ring, s data) { \
//     pthread_spin_lock(&ring->lock); \
//     ASSERT(ring->size.load() < ring->capacity, "Resource ringbuffer ran out of space.\n"); \
//     t *node = (t *)malloc(sizeof(t)); \
//     node->data = data; \
//     node->next = NULL; \
//     if (ring->tail) { \
//         ((t *)ring->tail)->next = node; \
//     } \
//     ring->tail = node; \
//     if (!ring->head) { \
//         ring->head = ring->tail; \
//     } \
//     ring->size.fetch_add(1); \
//     pthread_spin_unlock(&ring->lock); \
//     return &node->data; \
// }
//
// #define RESOURCE_RINGPOPDEF(n, s) s n(struct resource_ring *ring)
// #define RESOURCE_RINGPOP(n, s, t) s n(struct resource_ring *ring) { \
//     pthread_spin_lock(&ring->lock); \
//     if (ring->size.load() == 0) { \
//         pthread_spin_unlock(&ring->lock); \
//         return (s) { 0 }; \
//     } \
//     t *old = (t *)ring->head; \
//     s data = old->data; \
//     ring->head = ((t *)(ring->head))->next; \
//     free(old); \
//     ring->size.fetch_sub(1); \
//     if (!ring->size) { \
//         ring->tail = NULL; \
//     } \
//     pthread_spin_unlock(&ring->lock); \
//     return data; \
// }
//
// #define RESOURCE_RINGITERDEF(n) void n(struct resource_ring *ring, void (*func)(void *))
// #define RESOURCE_RINGITER(n, t) void n(struct resource_ring *ring, void (*func)(void *)) { \
//     t *node = (t *)ring->head; \
//     while (node) { \
//         func(&node->data); \
//         node = node->next; \
//     } \
// }
//
// #define RESOURCE_RINGDELETEDEF(n, s) void n(struct resource_ring *ring, s *data)
// #define RESOURCE_RINGDELETE(n, s, t) void n(struct resource_ring *ring, s *data) { \
//     pthread_spin_lock(&ring->lock); \
//     t *prev = NULL; \
//     t *node = (t *)ring->head; \
//     while (node) { \
//         if (!memcmp(&node->data, data, sizeof(s))) { \
//             if (!prev) { \
//                 ring->head = node->next; \
//                 if (!ring->head) { \
//                     ring->tail = NULL; \
//                 } \
//             } else { \
//                 prev->next = node->next; \
//                 if (!prev->next) { \
//                     ring->tail = prev; \
//                 } \
//             } \
//             free(node); \
//             ring->size.fetch_sub(1); \
//             pthread_spin_unlock(&ring->lock); \
//             return; \
//         } \
//         prev = node; \
//         node = node->next; \
//     } \
//     pthread_spin_unlock(&ring->lock); \
// }
//
// #define RESOURCE_RINGDESTROYDEF(n) void n(struct resource_ring *ring)
// #define RESOURCE_RINGDESTROY(n, t) void n(struct resource_ring *ring) { \
//     t *node = (t *)ring->head; \
//     while (node) { \
//         t *next = (t *)node->next; \
//         free(node); \
//         node = next; \
//     } \
//     ring->head = NULL; \
//     ring->tail = NULL; \
//     ring->size.store(0); \
// }
//
// #define RESOURCE_RINGFINDDEF(n, s, ...) s *n(struct resource_ring *ring, __VA_ARGS__)
// #define RESOURCE_RINGFIND(n, s, t, cmp, ...) s *n(struct resource_ring *ring, __VA_ARGS__) { \
//     t *node = (t *)ring->head; \
//     while (node) { \
//         if (cmp) { \
//             return &node->data; \
//         } \
//         node = node->next; \
//     } \
//     ASSERT(false, "Could not find resource in resource ring.\n"); \
//     return NULL; \
// }
//
// /**
//  * Reference lock, when marked the resource manager thread is not allowed to free this memory
//  *
//  */
// struct resource_reflock {
//     bool ref; // marked for reference
//     // TODO: GPU Specific reference locks (post-frame release, frame based release)
//     size_t life;
// };
//
// #define RESOURCE_REF(r) ({ (r)->lock.ref = true; r; })
// // free in 6 frames
// #define RESOURCE_MARKFREE(r) ({ (r)->lock.ref = false; (r)->lock.life = 6; r; });
// // free in the next frame
// #define RESOURCE_MARKFREEIMMEDIATE(r) ({ (r)->lock.ref = false; (r)->lock.life = 1; r; })
//
// /**
//  * Resource union for all GPU resource handles
//  */
// struct resource_gpu {
//     union {
//         bgfx_dynamic_index_buffer_handle_t dindex;
//         bgfx_dynamic_vertex_buffer_handle_t dvertex;
//         bgfx_frame_buffer_handle_t framebuffer;
//         bgfx_index_buffer_handle_t index;
//         bgfx_program_handle_t program;
//         bgfx_shader_handle_t shader;
//         bgfx_texture_handle_t texture;
//         bgfx_uniform_handle_t uniform;
//         bgfx_vertex_buffer_handle_t vertex;
//     };
//     bool freed;
//     size_t type;
//     struct resource_reflock lock;
// };
//
// RESOURCE_RINGNODE(struct resource_gpu, struct resource_gpunode);
//
// struct resource_ring *resource_ringinit(size_t capacity);
// // struct resource_gpu *resource_gpuringpush(struct resource_ring *ring, struct resource_gpu data);
// RESOURCE_RINGPUSHDEF(resource_gpuringpush, struct resource_gpu);
// // struct resource_gpu resource_gpuringpop(struct resource_ring *ring);
// RESOURCE_RINGPOPDEF(resource_gpuringpop, struct resource_gpu);
// // void resource_gpuringiter(struct resource_ring *ring, void (*func)(void *));
// RESOURCE_RINGITERDEF(resource_gpuringiter);
// // void resource_gpuringdelete(struct resource_ring *ring, struct resource_gpu *data);
// RESOURCE_RINGDELETEDEF(resource_gpuringdelete, struct resource_gpu);
// // struct resource_gpu *resource_gpuringfind(struct resource_ring *ring, uint16_t idx, size_t type);
// RESOURCE_RINGFINDDEF(resource_gpuringfind, struct resource_gpu, uint16_t idx, size_t type);
// // void resource_gpuringdestroy(struct resource_ring *ring);
// RESOURCE_RINGDESTROYDEF(resource_gpuringdestroy);
//
// // extern VECTOR_TYPE(struct resource_gpu) resource_gpu resources;
// // extern struct resource_gpu *resource_gpuresources;
// extern struct resource_ring *resource_gpuresources;
//
// #define ASSERT_NOTFREED(r) ({ ASSERT(!(r)->freed, "Attempted to free already freed resource.\n"); })
//
// static inline struct resource_gpu *resource_gpuquickpush(uint16_t idx, size_t type) {
//     struct resource_gpu rgpu = { 0 };
//     rgpu.shader.idx = idx;
//     rgpu.type = type;
//     rgpu.lock.ref = true;
//     return resource_gpuringpush(resource_gpuresources, rgpu);
// }
//
// #define RESOURCE_INIT(s, t) ({ resource_gpuquickpush((s).idx, (t)); })
//
// // #define RESOURCE_INIT(s, t) ({ \
//     struct resource_gpu rgpu = { 0 }; \
//     rgpu.shader.idx = (s).idx; \
//     rgpu.type = (t); \
//     rgpu.lock.ref = true; \
//     resource_gpuringpush(resource_gpuresources, rgpu); \
//     (s); \
// })
//
// // #define RESOURCE_DINDEX(s) ({ \
// //     struct resource_gpu rgpu = { .dindex = s }; \
// //     rgpu.type = GPU_DYNAMIC_INDEX; \
// //     rgpu.lock.ref = true; \
// //     rgpu.destroy = bgfx_destroy_dynamic_index_buffer; \
// //     stbds_arrput(resource_gpuresources, rgpu); \
// //     s; \
// // })
// //
// // #define RESOURCE_DVERTEX(s) ({ \
// //     struct resource_gpu rgpu = { .dvertex = s }; \
// //     rgpu.type = GPU_DYNAMIC_VERTEX; \
// //     rgpu.lock.ref = true; \
// //     rgpu.destroy = bgfx_destroy_dynamic_vertex_buffer; \
// //     stbds_arrput(resource_gpuresources, rgpu); \
// //     s; \
// // })
// //
// // #define RESOURCE_FRAMEBUFFER(s) ({ \
// //     struct resource_gpu rgpu = { .framebuffer = s }; \
// //     rgpu.type = GPU_FRAMEBUFFER; \
// //     rgpu.lock.ref = true; \
// //     rgpu.destroy = bgfx_destroy_frame_buffer; \
// //     stbds_arrput(resource_gpuresources, rgpu); \
// //     s; \
// // })
// //
// // #define RESOURCE_INDEX(s) ({ \
// //     struct resource_gpu rgpu = { .index = s }; \
// //     rgpu.type = GPU_INDEX \
// //     rgpu.lock.ref = true; \
// //     rgpu.destroy = bgfx_destroy_index_buffer; \
// //     stbds_arrput(resource_gpuresources, rgpu); \
// //     s; \
// // })
// //
// // #define RESOURCE_PROGRAM(s) ({ \
// //     struct resource_gpu rgpu = { .program = s }; \
// //     rgpu.type = GPU_PROGRAM; \
// //     printf("new program resource with idx %u on %u:%s\n", s.idx, __LINE__, __FILE__); \
// //     rgpu.lock.ref = true; \
// //     rgpu.destroy = bgfx_destroy_program; \
// //     stbds_arrput(resource_gpuresources, rgpu); \
// //     s; \
// // })
// //
// // #define RESOURCE_SHADER(s) ({ \
// //     struct resource_gpu rgpu = { .shader = s }; \
// //     rgpu.type = GPU_SHADER; \
// //     printf("new shader resource with idx %u on %u:%s\n", s.idx, __LINE__, __FILE__); \
// //     rgpu.lock.ref = true; \
// //     rgpu.destroy = bgfx_destroy_shader; \
// //     stbds_arrput(resource_gpuresources, rgpu); \
// //     s; \
// // })
// //
// // #define RESOURCE_TEXTURE(s) ({ \
// //     struct resource_gpu rgpu = { .texture = s }; \
// //     rgpu.type = GPU_TEXTURE; \
// //     rgpu.lock.ref = true; \
// //     printf("new texture resource with idx %u on %u:%s\n", s.idx, __LINE__, __FILE__); \
// //     rgpu.destroy = bgfx_destroy_texture; \
// //     stbds_arrput(resource_gpuresources, rgpu); \
// //     s; \
// // })
// //
// // #define RESOURCE_UNIFORM(s) ({ \
// //     struct resource_gpu rgpu = { .uniform = s }; \
// //     rgpu.type = GPU_UNIFORM; \
// //     rgpu.lock.ref = true; \
// //     rgpu.destroy = bgfx_destroy_uniform; \
// //     stbds_arrput(resource_gpuresources, rgpu); \
// //     s; \
// // })
// //
// // #define RESOURCE_VERTEX(s) ({ \
// //     struct resource_gpu rgpu = { .vertex = s }; \
// //     rgpu.type = GPU_VERTEX; \
// //     rgpu.lock.ref = true; \
// //     rgpu.destroy = bgfx_destroy_vertex_buffer; \
// //     stbds_arrput(resource_gpuresources, rgpu); \
// //     s; \
// // })
//
// // #define RESOURCE_FINDDINDEX(s) ({ resource_findresource((s).idx, GPU_DYNAMIC_INDEX); })
// // #define RESOURCE_FINDDVERTEX(s) ({ resource_findresource((s).idx, GPU_DYNAMIC_VERTEX); })
// // #define RESOURCE_FINDFRAMEBUFFER(s) ({ printf("framebuffer free\n"); resource_findresource((s).idx, GPU_FRAMEBUFFER); })
// // #define RESOURCE_FINDINDEX(s) ({ resource_findresource((s).idx, GPU_INDEX); })
// // #define RESOURCE_FINDPROGRAM(s) ({ resource_findresource((s).idx, GPU_PROGRAM); })
// // #define RESOURCE_FINDSHADER(s) ({ resource_findresource((s).idx, GPU_SHADER); })
// // #define RESOURCE_FINDTEXTURE(s) ({ resource_findresource((s).idx, GPU_TEXTURE); })
// // #define RESOURCE_FINDUNIFORM(s) ({ resource_findresource((s).idx, GPU_UNIFORM); })
// // #define RESOURCE_FINDVERTEX(s) ({ resource_findresource((s).idx, GPU_VERTEX); })
// #define RESOURCE_FIND(s, t) ({ resource_gpuringfind(resource_gpuresources, (s).idx, (t)); })
//
// struct resource_gpu *resource_findresource(uint16_t idx, size_t type);
// void resource_gpufree(struct resource_gpu *rgpu);
// void resource_gpufreeall(void);
//
// struct resource_cpu {
//     void *memory;
//     size_t length;
//     bool freed;
//     struct resource_reflock lock;
// };
//
// // extern VECTOR_TYPE(struct resource_cpu) resource_cpuresources;
//
// void resource_cpufree(struct resource_cpu rcpu);
// void resource_cpufreeall(void);
//
// void resource_init(void);
// void resource_kick(void);

#endif
