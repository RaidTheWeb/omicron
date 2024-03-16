#ifndef _ENGINE__RESOURCES__SERIALISE_HPP
#define _ENGINE__RESOURCES__SERIALISE_HPP

#include <engine/resources/resource.hpp>
#include <engine/resources/rpak.hpp>

namespace OResource {
    // Serialisation is in binary form, every class is able to provide information on its fields and how to serialise/deserialise them

    struct objheader {
        uint32_t key;
        uint32_t version;
    };

    class Serialiser {
        public:
            OJob::Mutex mutex;
            uint8_t *data = NULL;
            size_t writeoffset = 0;
            size_t readoffset = 0;
            bool ro = false;

            Serialiser(void) {
                this->data = (uint8_t *)malloc(1);
            }
            
            Serialiser(uint8_t *data, size_t size) {
                this->data = data;
                this->ro = true; // we can't write to buffers outside the serialiser
                this->writeoffset = size;
            }

            ~Serialiser(void) {
                if (!this->ro) {
                    free(this->data);
                }
            }

            template <typename T>
            void write(T *data) {
                this->mutex.lock();
                // XXX: Consider mutex safe ASSERTs
                ASSERT(!this->ro, "Tried to write to read only buffer.\n");
                this->data = (uint8_t *)realloc(this->data, this->writeoffset + sizeof(T));
                memcpy(this->data + this->writeoffset, data, sizeof(T));
                this->writeoffset += sizeof(T);
                this->mutex.unlock();
            }

            template <typename T>
            void read(T *data) {
                this->mutex.lock();
                ASSERT(this->readoffset + sizeof(T) <= this->writeoffset, "Attempting to read more than is written (writeoffset: %lu, readoffset: %lu).\n", this->writeoffset, this->readoffset);
                memcpy(data, this->data + this->readoffset, sizeof(T));
                this->readoffset += sizeof(T);
                this->mutex.unlock();
            }
    };

}


#endif
