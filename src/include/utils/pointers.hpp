#ifndef _UTILS__POINTERS_HPP
#define _UTILS__POINTERS_HPP

#include <atomic>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <utils.hpp>
#include <utils/hash.hpp>

namespace OUtils {

    class ResolutionTable {
        public:
            size_t size = 0;
            void **table = NULL;

            ResolutionTable(size_t size) {
                table = (void **)malloc(sizeof(void *) * size);
                this->size = size;
            }

            size_t bind(void *ptr) {
                ASSERT(ptr != NULL, "Pointer is NULL.\n");
                uint32_t off = OUtils::fnv1a(ptr); // we use a hash here to try and reduce the amount of time we'll spend searching for a free spot
                for (size_t i = 0; i < this->size; i++) {
                    size_t idx = (off + i) % this->size;

                    if (this->table[idx] != NULL) {
                        continue;
                    }

                    this->table[idx] = ptr;
                    return idx;
                }
                return SIZE_MAX;
            }

            void release(size_t handle) {
                this->table[handle] = NULL;
            }

            void *get(size_t handle) {
                return this->table[handle];
            }
    };

    template <typename T>
    class HandlePointer {
        public:
            size_t handle = SIZE_MAX;
            ResolutionTable *table = NULL;

            HandlePointer(void) { }
            HandlePointer(ResolutionTable *table, size_t handle) {
                this->table = table;
                this->handle = handle;
            }

            constexpr bool operator ==(const HandlePointer &rhs) {
                return this->handle == rhs.handle;
            }

            constexpr bool operator !=(const HandlePointer &rhs) {
                return this->handle != rhs.handle;
            }

            T& operator *(void) {
                ASSERT(this->handle != SIZE_MAX, "Attempted to dereference invalid handle.\n");
                return *((T *)this->table->get(this->handle));
            }

            T *operator ->(void) {
                ASSERT(this->handle != SIZE_MAX, "Attempted to reference invalid handle.\n");
                return (T *)this->table->get(this->handle);
            }
    };

}

#endif
