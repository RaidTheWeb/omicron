#ifndef _ENGINE__UTILS__QUEUE_HPP
#define _ENGINE__UTILS__QUEUE_HPP

#include <atomic>
#include <engine/assertion.hpp>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace OUtils {

    class MPMCQueue {
        private:
            struct cell {
                std::atomic<size_t> seq;
                void *data;
            };

            struct work {
                struct cell *cell;
                size_t pos;
            };

            // Cacheline friendly (64-bit aligned) for optimal access speed
            uint8_t pad0[64];
            struct cell *buf;
            size_t mask;
            uint8_t pad1[64];
            std::atomic<size_t> enqueuepos;
            uint8_t pad2[64];
            std::atomic<size_t> dequeuepos;
            uint8_t pad3[64];

            struct work work(std::atomic<size_t> *atomic, uint32_t delta) {
                size_t pos = atomic->load(std::memory_order_relaxed);

                for (;;) {
                    struct cell *cell = &this->buf[pos & this->mask];
                    size_t seq = cell->seq.load(std::memory_order_acquire);

                    intptr_t diff = (intptr_t)seq - (intptr_t)(pos + delta);

                    if (!diff) {
                        if (atomic->compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
                            return (struct work) { .cell = cell, .pos = pos };
                        }
                    } else {
                        if (diff < 0) {
                            return (struct work) { .cell = NULL, .pos = 0 };
                        } else {
                            pos = atomic->load(std::memory_order_relaxed);
                        }
                    }
                }
            }
        public:
            MPMCQueue(size_t size = 16384) {
                ASSERT((size >= 2) && ((size & (size - 1)) == 0), "Invalid queue size %lu, must be a power of 2.\n", size);

                this->buf = (struct cell *)malloc(size * sizeof(struct cell));
                ASSERT(buf != NULL, "Failed to allocate cells for MPMC queue.\n");
                memset(this->buf, 0, size * sizeof(struct cell));

                this->mask = size - 1;

                for (size_t i = 0; i != size; i++) {
                    this->buf[i].seq.store(i, std::memory_order_relaxed);
                }

                this->enqueuepos.store(0, std::memory_order_relaxed);
                this->dequeuepos.store(0, std::memory_order_relaxed);
            }

            ~MPMCQueue(void) {
                free(this->buf);
            }

            void push(void *data) {
                struct work get = this->work(&this->enqueuepos, 0);

                ASSERT(get.cell != NULL, "No space available in queue for element.\n");
                get.cell->data = data;
                get.cell->seq.store(get.pos + 1, std::memory_order_release);
            }

            void *pop(void) {
                struct work get = this->work(&this->dequeuepos, 1);

                if (get.cell) {
                    void *data = get.cell->data;
                    get.cell->seq.store(get.pos + this->mask + 1, std::memory_order_release);
                    return data;
                }

                return NULL; // queue is empty
            }
    };

    // struct mpmc_queue {
    //     // Everything here is aligned 64-bits for optimal cache
    //     uint8_t pad0[64];
    //     struct mpmc_cell *buf;
    //     size_t mask;
    //     uint8_t pad1[64];
    //     std::atomic<size_t> enqueuepos;
    //     uint8_t pad2[64];
    //     std::atomic<size_t> dequeuepos;
    //     uint8_t pad3[64];
    // };

    // bool mpmc_initqueue(struct mpmc_queue *queue, size_t size);
    // void mpmc_destroyqueue(struct mpmc_queue *queue);
    // bool mpmc_queuepush(struct mpmc_queue *queue, void *data);
    // void *mpmc_queuepop(struct mpmc_queue *queue);

}

#endif
