#ifndef _ENGINE__RESOURCES__RPAK_HPP
#define _ENGINE__RESOURCES__RPAK_HPP

#include <engine/assertion.hpp>
#include <engine/concurrency/job.hpp>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

namespace OResource {

#define RPAK_FORMATVERSION 1
#define RPAK_COMPRESSBIAS (16 * 1024 * 1024) // above 16MB we compress the file
#define RPAK_COMPRESSIONFAULTTOLERANCE (32) // fault tolerance for compression buffers

    class RPak {
        private:
            OJob::Mutex lock;
            int fd;
        public:
            struct header {
                char magic[5]; // RPAK\0
                uint8_t version; // version of RPak
                uint32_t contentversion; // version of this particular RPak archive
                char name[64];
                uint32_t num; // number of package entries
            } __attribute__((packed));

            struct tableentry {
                char path[512]; // full pathname for the file in RPak
                bool compressed; // is this file compressed?
                size_t uncompressedsize; // original file size
                size_t compressedsize; // file size when compressed
                size_t offset; // offset of file data in blob
            } __attribute__((packed));

            struct stat {
                size_t realsize; // size in the RPAK file
                size_t decompressedsize; // size counting for decompression
                bool compressed;
            };

            struct data {
                uint8_t *data;
                size_t size;
            };

            struct header header;
            struct tableentry *entries;

            RPak(const char *path);
            ~RPak(void) {
                this->lock.lock();
                close(this->fd);
                free(entries);
                this->lock.unlock();
            }

            struct stat stat(const char *path);
            size_t read(const char *path, void *buf, const size_t size, const size_t off);

            static void create(const char *path, const char *output);
    };
}

#endif
