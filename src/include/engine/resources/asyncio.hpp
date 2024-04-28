#ifndef _ENGINE__RESOURCES__ASYNCIO_HPP
#define _ENGINE__RESOURCES__ASYNCIO_HPP

#include <engine/concurrency/job.hpp>
#include <engine/resources/resource.hpp>
#include <engine/resources/rpak.hpp>
#include <stdio.h>

namespace OResource {
    class AsyncIO {
        public:
            enum error {
                SUCCESS,
                SUSPENDED,
                FAILED
            };

            struct work {
                const Resource *resource;
                void *buffer = NULL;
                bool hascallback;
                size_t offset;
                size_t size;
                uint8_t *error;
                void (*callback)(struct work *work);
            };

            static void loadwrapper(OJob::Job *job) {
                ZoneScopedN("AsyncIO File Load");
                struct work *work = (struct work *)job->param;
                if (work->size != SIZE_MAX) {
                    work->buffer = malloc(work->size);
                    ASSERT(work->buffer != NULL, "Failed to allocate buffer memory for async file load of %s.\n", work->resource->path);
                }
                if (work->resource->type == Resource::SOURCE_OSFS) {
                    FILE *f = fopen(work->resource->path, "r");
                    ASSERT(f != NULL, "Failed to load file %s.\n", work->resource->path);
                    ASSERT(!fseek(f, 0, SEEK_END), "Failed to seek end of file to figure out its size of %s.\n", work->resource->path);
                    size_t size = ftell(f);
                    if (work->size != SIZE_MAX) {
                        ASSERT(size - work->offset != 0, "Specified offset is at the end of the file for %s.\n", work->resource->path);
                    }
                    ASSERT(!fseek(f, work->offset, SEEK_SET), "Failed to seek to specified offset for %s.\n", work->resource->path);
                    if (work->size != SIZE_MAX) {
                        ASSERT(fread(work->buffer, work->size, 1, f), "Failed to read data from file %s.\n", work->resource->path);
                    } else {
                        work->buffer = malloc(size - work->offset);
                        ASSERT(work->buffer != NULL, "Failed to allocate buffer memory for async file load of %s.\n", work->resource->path);
                        work->size = size - work->offset; // update the size to now include the actual buffer size
                        ASSERT(fread(work->buffer, size - work->offset, 1, f), "Failed to read data from file %s.\n", work->resource->path);
                    }
                    fclose(f);
                    if (work->error != NULL) {
                        *work->error = SUCCESS;
                    }
                }
                if (work->hascallback) {
                    ASSERT(work->callback != NULL, "Callback async file load for %s was requested but no callback provided!\n", work->resource->path);
                    work->callback(work); // Run callback
                }
            }

            // Request a file to be loaded and sleep the calling job until the file has been loaded (Blocking, but will wake the caller when done).
            static void loadwait(Resource *resource, void **buffer, size_t offset, size_t size, uint8_t *error = NULL) {
                ZoneScopedN("AsynIO File Load Wait");
                ASSERT(buffer != NULL, "No buffer for output data provided.\n");
                ASSERT(size > 0, "Buffer cannot have the size 0.\n");

                struct work work; // Stack memory isn't invalidated until this function returns.
                work.resource = resource;
                work.size = size;
                work.hascallback = false;
                work.offset = offset;
                work.error = error;

                OJob::Job *job = new OJob::Job(loadwrapper, (uintptr_t)&work);
                OJob::Counter counter = OJob::Counter(); // We can keep this in stack memory because the memory here isn't invalidated until the function returns.
                job->counter = &counter;
                OJob::kickjobwait(job);
                *buffer = work.buffer;
            }

            // Request a file to be loaded and calls a callback when done, buffer for data is allocated automatically for the callback (Non-blocking).
            static void loadcall(Resource *resource, size_t offset, size_t size, void (*callback)(struct work *)) {
                struct work *work = (struct work *)malloc(sizeof(struct work)); // We have to allocate a pointer for our work here as it would otherwise result in nothing being done.
                ASSERT(work != NULL, "Failed to allocate work object for async file load.\n");
                work->resource = resource;
                work->buffer = NULL;
                work->hascallback = true;
                work->callback = callback;
                work->size = size;
                work->error = NULL;
                work->offset = offset;

                OJob::Job *job = new OJob::Job(loadwrapper, (uintptr_t)work);
                OJob::kickjob(job);
            }

            static void loadwait(const char *path, void **buffer, size_t offset, size_t size, uint8_t *error) {
                loadwait(manager.get(path), buffer, offset, size, error);
            }

            static void loadcall(const char *path, size_t offset, size_t size, void (*callback)(struct work *)) {
                loadcall(manager.get(path), offset, size, callback);
            }
    };
};

#endif
