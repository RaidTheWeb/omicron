#ifndef _ENGINE__UTILS__REFLECTION_HPP
#define _ENGINE__UTILS__REFLECTION_HPP

#include <stdlib.h>
#include <stdint.h>
#include <unordered_map>

namespace OUtils {

    class ReflectedType {
        public:
            // Call `new` and return allocation.
            virtual void *instantiate(void) = 0;
            // Explicitly malloc to bypass any custom constructor (pool allocation, etc.).
            virtual void *alloc(void) = 0;
            virtual void *create(bool pool = true) = 0;
    };

    template <typename T>
    class ReflectedImplementation : public ReflectedType {
        public:
            virtual void *instantiate(void) {
                return new T;
            }

            virtual void *alloc(void) {
                // XXX: Probably very unsafe
                T *allocation = (T *)malloc(sizeof(T));
                ASSERT(allocation != NULL, "Failed to allocate memory for reflected type implementation.\n");
                T t = T();
                memcpy(allocation, &t, sizeof(T));
                return allocation;
            }

            virtual void *create(bool pool = true) {
                return T::template create<T>(pool); 
            }
    };

    extern std::unordered_map<uint32_t, ReflectedType *> reflectiontable;
}

#endif
