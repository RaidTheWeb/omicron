#ifndef _ENGINE__UTILS__POINTERS_HPP
#define _ENGINE__UTILS__POINTERS_HPP

#include <atomic>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <engine/concurrency/job.hpp>
#include <engine/utils.hpp>
#include <engine/utils/hash.hpp>
#include <unordered_map>

#define OMICRON_RESOLUTIONTABLEMAP

namespace OUtils {

#ifndef OMICRON_RESOLUTIONTABLEMAP // if we aren't using C++ stl maps for our table
    class ResolutionTable {
        public:
            size_t size = 0;
            void **table = NULL;

            ResolutionTable(size_t size) {
                table = (void **)malloc(sizeof(void *) * size);
                ASSERT(table != NULL, "Failed to allocate memory for handle resolution table.\n");
                this->size = size;
            }

            ~ResolutionTable(void) {
                free(table);
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
#else
    class ResolutionTable {
        public:
            OJob::Mutex mutex;
            size_t size = 0;

            std::unordered_map<size_t, void *> table;
            size_t handle;

            ResolutionTable(size_t size) {
                this->size = size;
                this->table.reserve(size); // initialise (these will be the fastest handles)
            }

            ~ResolutionTable(void) {
                this->table.clear();
            }

            size_t bind(void *ptr) {
                ASSERT(ptr != NULL, "Pointer is NULL.\n");
                this->mutex.lock();
                size_t index = this->handle++;
                this->table[index] = ptr;
                this->mutex.unlock();
                return index;
            }

            void release(size_t handle) {
                this->mutex.lock();
                this->table.erase(handle);
                this->mutex.unlock();
            }

            void *get(size_t handle) {
                return this->table[handle];
            }
    };
#endif

#define HANDLE_TEMPLATEINVALID OUtils::Handle<T>(NULL, SIZE_MAX, SIZE_MAX)

    template <typename T>
    class Handle {
        public:
            size_t objhandle = SIZE_MAX; // handle to object in resolution table
            size_t objid = SIZE_MAX; // object id to prevent stale handles
            ResolutionTable *table = NULL;
            Handle(void) { }
            Handle(ResolutionTable *table, size_t handle, size_t id) {
                this->table = table;
                this->objhandle = handle;
                this->objid = id;
            }

            template <typename U>
            Handle(Handle<U> &handle) {
                this->table = handle.table;
                this->objhandle = handle.objhandle;
                this->objid = handle.objid;
            }

            constexpr bool operator ==(const Handle &rhs) {
                return this->objhandle == rhs.objhandle && this->objid == rhs.objid;
            }

            constexpr bool operator !=(const Handle &rhs) {
                return this->objhandle != rhs.objhandle || this->objid != rhs.objid;
            }

            bool isvalid(void) {
                if (this->objhandle == SIZE_MAX) {
                    return false;
                }
                T *ptr = (T *)this->table->get(this->objhandle);
                if (ptr == NULL) {
                    return false;
                }

                if (ptr->id != this->objid) {
                    return false;
                }

                return true;
            }

            T& operator *(void) {
                ASSERT(this->objhandle != SIZE_MAX, "Attempted to dereference invalid handle.\n");
                T *ptr = (T *)this->table->get(this->objhandle);
                ASSERT(ptr != NULL && ptr->id == this->objid, "Attempted to dereference stale pointer.\n");
                return *ptr;
            }

            T *operator ->(void) {
                ASSERT(this->objhandle != SIZE_MAX, "Attempted to reference invalid handle.\n");
                T *ptr = (T *)this->table->get(this->objhandle);
                ASSERT(ptr != NULL && ptr->id == this->objid, "Attempted to dereference stale pointer.\n");
                return ptr;
            }
    };

}

#endif
