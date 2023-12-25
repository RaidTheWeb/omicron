#ifndef _UTILS__MEMORY_HPP
#define _UTILS__MEMORY_HPP

#include <concurrency/job.hpp>

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
            }

            ~LinearAllocator(void) {
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

    class PoolAllocator {
        private:
            size_t blocksize = 0;
            size_t size = 0;
            uint8_t *blocks = NULL;
            OJob::Mutex mutex;

        public:
            struct block {
                struct block *next;
            };

            struct block *allocblock = NULL;

            PoolAllocator(void) { }
            PoolAllocator(size_t blocksize, size_t size) {
                this->init(blocksize, size);
            }

            void init(size_t blocksize, size_t size) {
                ASSERT(size > 0, "Cannot initialise a pool allocator with block count of %lu\n", size);
                ASSERT(blocksize >= sizeof(struct block), "Block size %lu is not large enough to accomodate a pointer to the next free block.\n", blocksize);

                this->size = size;
                this->blocksize = blocksize;
                this->blocks = (uint8_t *)malloc(this->blocksize * size);
                ASSERT(this->blocks, "Failed to allocate memory blocks.\n");
                memset(this->blocks, 0, this->blocksize * size);

                struct block *block = (struct block *)this->blocks;
                for (size_t i = 0; i < size - 1; i++) {
                    block->next = (struct block *)(((uint8_t *)block) + blocksize);
                    block = block->next;
                }
                block->next = NULL;

                this->allocblock = (struct block *)this->blocks;
            }

            ~PoolAllocator(void) {
                std::free(blocks);
            }

            void *alloc(void) {
                this->mutex.lock();
                ASSERT(this->allocblock != NULL, "Allocation exceeds available blocks.\n");

                struct block *freeblock = this->allocblock;
                this->allocblock = this->allocblock->next;
                this->mutex.unlock();
                return (void *)freeblock;
            }

            void free(void *ptr) {
                this->mutex.lock();
                ASSERT(ptr != NULL, "Expected allocation, not NULL.\n");

                ((struct block *)ptr)->next = this->allocblock;
                this->allocblock = ((struct block *)ptr);
                this->mutex.unlock();
            }
    };

    class SlabAllocator {
        private:
            struct metadata {
                size_t size;
            };

        public:
            class Slab {
                public:
                    PoolAllocator allocator;
                    size_t entsize;
                    Slab(void) { }
                    void init(size_t entsize, size_t blocks) {
                        this->entsize = entsize;
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

            SlabAllocator() {
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
                    return slab->alloc(size);
                }

                // Fallback to host memory allocator (yikers!)
                struct metadata *allocation = (struct metadata *)malloc(size + sizeof(struct metadata));
                allocation->size = size;
                return allocation + 1;
            }

            void free(void *ptr) {
                struct metadata *allocation = (struct metadata *)((uint8_t *)ptr - sizeof(struct metadata));
                Slab *slab = this->optimalslab(allocation->size);
                if (slab != NULL) {
                    slab->free(allocation);
                } else {
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
            OJob::Mutex mutex;

        public:
            StackAllocator(void) { }
            StackAllocator(size_t size) {
                this->init(size);
            }

            void init(size_t size) {
                ASSERT(size > 0, "Cannot initialise stack allocator with memory size of %lu\n", size);

                this->size = size;
                this->mem = (uint8_t *)malloc(size);
                ASSERT(this->mem != NULL, "Failed to allocate memory.\n");
                memset(this->mem, 0, size);
            }
            ~StackAllocator(void) {
                free(this->mem);
            }

            // Allocate a `size` memory from the stack allocator block.
            void *alloc(size_t size) {
                this->mutex.lock();
                ASSERT(this->marker + size <= this->size, "Allocation of size %lu exceeds available memory block.\n", size);
                void *allocation = mem + this->marker;
                this->marker += size;
                this->mutex.unlock();
                return allocation;
            }

            // Return the current marker position of the stack allocator.
            size_t getmarker(void) {
                return this->marker;
            }

            // Free back to a marker position.
            void freeto(size_t marker) {
                this->mutex.lock();
                this->marker = marker;
                this->mutex.unlock();
            }

            // Free all allocated memory.
            void clear(void) {
                this->mutex.lock();
                marker = 0;
                this->mutex.unlock();
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
