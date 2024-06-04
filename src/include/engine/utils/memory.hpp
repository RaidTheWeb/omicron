#ifndef _ENGINE__UTILS__MEMORY_HPP
#define _ENGINE__UTILS__MEMORY_HPP

#include <engine/concurrency/job.hpp>
#include <tracy/Tracy.hpp>

namespace OUtils {

    class LinearAllocator {
        private:
            size_t blocksize = 0;
            size_t size = 0;
            size_t marker = 0;
            uint8_t *blocks = NULL;
            OJob::Mutex mutex;

        public:
            LinearAllocator(void) { }
            LinearAllocator(size_t blocksize, size_t size) {
                this->init(blocksize, size);
            }

            void init(size_t blocksize, size_t size) {
                ASSERT(size > 0, "Cannot initialise linear allocator with block count of %lu\n", size);

                this->blocksize = blocksize;
                this->size = size;
                this->blocks = (uint8_t *)malloc(this->blocksize * size);
                ASSERT(this->blocks != NULL, "Failed to allocate memory blocks.\n");
                memset(this->blocks, 0, this->blocksize * size);
                TracySecureAllocN(this->blocks, this->blocksize * this->size, "Linear Allocator");
            }

            ~LinearAllocator(void) {
                TracySecureFreeN(this->blocks, "Linear Allocator");
                free(this->blocks);
            }

            void *alloc(size_t size) {
                this->mutex.lock();
                ASSERT(this->marker + size <= this->size, "Allocation of size %lu exceeds available memory block.\n", size);
                void *allocation = blocks + this->marker;
                this->marker += this->blocksize * size;
                this->mutex.unlock();
                return allocation;
            }

            void clear(void) {
                this->mutex.lock();
                this->marker = 0;
                this->mutex.unlock();
            }
    };

#define MEMORY_POOLALLOCEXPAND 16

    class PoolAllocator {
        private:
            size_t blocksize = 0;
            size_t size = 0;
            size_t allocated = 0;
            size_t expandsize = 0;
            uint8_t *blocks = NULL;
            uint64_t previous = 0;
            OJob::Spinlock spin;
            std::vector<void *> additionalmem; // additional memory allocations (upon expansion)

        public:
            struct block {
                struct block *next;
            };

            struct block *allocblock = NULL;
            const char *name = NULL;

            PoolAllocator(void) { }
            PoolAllocator(size_t blocksize, size_t size, size_t expand = MEMORY_POOLALLOCEXPAND, const char *name = NULL) {
                this->expandsize = expand;
                this->init(blocksize, size, name);
            }

            void init(size_t blocksize, size_t size, const char *name = NULL) {
                ASSERT(size > 0, "Cannot initialise a pool allocator with block count of %lu\n", size);
                ASSERT(blocksize >= sizeof(struct block), "Block size %lu is not large enough to accomodate a pointer to the next free block.\n", blocksize);

                this->name = name;
                this->size = size;
                this->blocksize = blocksize;
                this->blocks = (uint8_t *)malloc(this->blocksize * size);
                ASSERT(this->blocks, "Failed to allocate memory blocks.\n");
                memset(this->blocks, 0, this->blocksize * size);
                TracySecureAllocN(this->blocks, this->blocksize * this->size, "PoolAllocator");

                struct block *block = (struct block *)this->blocks;
                for (size_t i = 0; i < size - 1; i++) {
                    block->next = (struct block *)(((uint8_t *)block) + blocksize);
                    block = block->next;
                }
                block->next = NULL;

                this->allocblock = (struct block *)this->blocks;
            }

            ~PoolAllocator(void) {
                TracySecureFreeN(this->blocks, "PoolAllocator");
                std::free(this->blocks);
                for (auto it = this->additionalmem.begin(); it != this->additionalmem.end(); it++) {
                    TracySecureFreeN(*it, "PoolAllocator Additional Memory");
                    std::free(*it); // free additional memory allocations
                }
            }

            void expand(void) {
                // we implictly assume this is already locked as it's only ever called inside a mutex locked call
                size_t increment = expandsize;
                void *memory = malloc(increment * this->blocksize);
                ASSERT(memory != NULL, "Failed to allocate memory for pool expansion.\n");
                memset(memory, 0, increment * this->blocksize);
                TracySecureAllocN(memory, increment * this->blocksize, "PoolAllocator Additional Memory");
                struct block *block = (struct block *)memory;
                for (size_t i = 0; i < increment - 1; i++) {
                    block->next = (struct block *)(((uint8_t *)block) + blocksize);
                    block = block->next;
                }
                block->next = NULL;

                this->allocblock = (struct block *)memory;
                this->additionalmem.push_back(memory);
                size += increment;
            }

            void *alloc(void) {
                // this->mutex.lock();
                this->spin.lock();
                if (OJob::currentfibre != NULL) {
                    TracyMessageL("lock acquired");
                    // printf("lock acquired on fibre %lu\n", OJob::currentfibre->id);
                }
                // ASSERT(this->allocblock != NULL, "Allocation exceeds available blocks.\n");

                if (this->allocblock == NULL) {
                    this->expand();
                    // this->mutex.unlock();
                    this->spin.unlock();
                    if (OJob::currentfibre != NULL) {
                        TracyMessageL("lock released following expansion");
                        // printf("lock released on fibre before expansion %lu\n", OJob::currentfibre->id);
                    }
                    return this->alloc();
                }

                struct block *freeblock = this->allocblock;
                if (this->name != NULL) {
                    TracySecureAllocN(freeblock, this->blocksize, this->name);
                }
                this->allocblock = this->allocblock->next;
                this->allocated++;
                ASSERT(previous != (uint64_t)freeblock, "Current allocation matches previous %lx == %lx.\n", previous, (uint64_t)previous);
                this->previous = 0;
                // this->mutex.unlock();
                this->spin.unlock();
                if (OJob::currentfibre != NULL) {
                    TracyMessageL("lock released");
                    // printf("lock released on fibre %lu\n", OJob::currentfibre->id);
                }

                return (void *)freeblock;
            }

            void free(void *ptr) {
                // this->mutex.lock();
                this->spin.lock();
                ASSERT(ptr != NULL, "Expected allocation, not NULL.\n");

                TracyMessageL("Freeing!");
                if (this->name != NULL) {
                    TracySecureFreeN(ptr, this->name);
                }

                ((struct block *)ptr)->next = this->allocblock;
                this->allocblock = ((struct block *)ptr);
                this->allocated--;
                this->spin.unlock();
                // this->mutex.unlock();
            }

            size_t getfree(void) {
                return this->size - this->allocated;
            }
    };

    class SlabAllocator {
        public:
            struct metadata {
                size_t size;
            };
            const char *name = NULL;
            class Slab {
                public:
                    PoolAllocator allocator;
                    size_t entsize;
                    Slab(void) { }
                    void init(size_t entsize, size_t blocks) {
                        this->entsize = entsize;
                        // Ideally we wouldn't expand the pool allocators
                        allocator.init(entsize + sizeof(struct metadata), blocks);
                    }

                    void *alloc(size_t size) {
                        struct metadata *allocation = (struct metadata *)this->allocator.alloc();
                        allocation->size = size;
                        return allocation + 1;
                    }

                    void free(void *ptr) {
                        this->allocator.free(ptr);
                    }
            };

            // 7MB~ of preallocated slab memory
            Slab slabs[10];

            SlabAllocator(const char *name = NULL) {
                this->name = name;
                // 3MB
                this->slabs[0].init(8, 16384);
                this->slabs[1].init(16, 16384);
                this->slabs[2].init(24, 16384);
                this->slabs[3].init(32, 16384);
                this->slabs[4].init(48, 16384);
                this->slabs[5].init(64, 16384);

                // 1MB
                this->slabs[6].init(128, 8096);
                // 1MB
                this->slabs[7].init(256, 4096);
                // 1MB
                this->slabs[8].init(512, 2048);
                // 1MB
                this->slabs[9].init(1024, 1024);
            }

            Slab *optimalslab(size_t size) {
                for (size_t i = 0; i < sizeof(this->slabs) / sizeof(this->slabs[0]); i++) {
                    Slab *slab = &this->slabs[i];
                    // Slab allocator
                    if (slab->entsize >= size && slab->allocator.allocblock != NULL) {
                        return slab;
                    }
                }
                return NULL;
            }

            void *alloc(size_t size) {
                Slab *slab = this->optimalslab(size);
                if (slab != NULL) {
                    void *allocation = slab->alloc(size);
                    if (this->name != NULL) {
                        TracySecureAllocN(allocation, sizeof(struct metadata) + size, this->name);
                    }
                    return allocation;
                }

                // Fallback to host memory allocator (yikers!)
                struct metadata *allocation = (struct metadata *)malloc(size + sizeof(struct metadata));
                ASSERT(allocation != NULL, "Failed to allocate memory from fallback host memory allocator.\n");
                allocation->size = size;
                TracySecureAllocN(allocation, sizeof(struct metadata) + size, "SlabAllocator Host Fallback");
                if (this->name != NULL) {
                    TracySecureAllocN(allocation, sizeof(struct metadata) + size, this->name);
                }
                return allocation + 1;
            }

            virtual void free(void *ptr) {
                struct metadata *allocation = (struct metadata *)((uint8_t *)ptr - sizeof(struct metadata));
                Slab *slab = this->optimalslab(allocation->size);
                if (slab != NULL) {
                    if (this->name != NULL) {
                        TracySecureFreeN(allocation, this->name);
                    }
                    slab->free(allocation);
                } else {
                    TracySecureFreeN(allocation, "SlabAllocator Host Fallback");
                    std::free(allocation);
                }
            }
    };

    // Stack based allocator, blazing fast alternative compared to malloc/free for many temporary allocations.
    class StackAllocator {
        private:
            size_t size = 0;
            size_t marker = 0;
            uint8_t *mem = NULL;
            const char *name = NULL;
            OJob::Mutex mutex;

        public:
            StackAllocator(void) { }
            StackAllocator(size_t size) {
                this->init(size);
            }

            void claim(void) {
                this->mutex.lock();
            }

            void release(void) {
                this->mutex.unlock();
            }

            void init(size_t size) {
                ASSERT(size > 0, "Cannot initialise stack allocator with memory size of %lu\n", size);

                this->size = size;
                this->mem = (uint8_t *)malloc(size);
                ASSERT(this->mem != NULL, "Failed to allocate memory.\n");
                memset(this->mem, 0, size);
                TracySecureAllocN(this->mem, this->size, "StackAllocator");
            }
            ~StackAllocator(void) {
                TracySecureFreeN(this->mem, "StackAllocator");
                free(this->mem);
            }

            // Allocate a `size` memory from the stack allocator block.
            void *alloc(size_t size) {
                ASSERT(this->marker + size <= this->size, "Allocation of size %lu exceeds available memory block.\n", size);
                void *allocation = mem + this->marker;
                this->marker += size;
                return allocation;
            }

            // Return the current marker position of the stack allocator.
            size_t getmarker(void) {
                return this->marker;
            }

            // Free back to a marker position.
            void freeto(size_t marker) {
                if (marker == this->marker) {
                    return;
                }
                this->marker = marker;
            }

            // Free all allocated memory.
            void clear(void) {
                marker = 0;
            }
    };

    // Dirty double buffer allocator combining two stack allocators, useful for amortised memory allocations across two frames.
    class DoubleBufferedAllocator {
        private:
            uint8_t stack : 1;

            StackAllocator stacks[2];

        public:
            DoubleBufferedAllocator(void) {
                this->stacks[0].init(16384);
                this->stacks[1].init(16384);
            }

            // Swap between stack allocators.
            void swap(void) {
                this->stack = !this->stack;
            }

            // Clear current stack allocator.
            void clear(void) {
                this->stacks[this->stack].clear();
            }

            // Allocate from current stack allocator.
            void *alloc(size_t size) {
                return this->stacks[this->stack].alloc(size);
            }
    };

}

#endif
