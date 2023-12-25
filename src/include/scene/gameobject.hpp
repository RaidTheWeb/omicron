#ifndef _SCENE__GAMEOBJECT_HPP
#define _SCENE__GAMEOBJECT_HPP

#include <engine/transform.hpp>
#include <unordered_set>
#include <utils/memory.hpp>

namespace OScene {
    // XXX: GRAHHHHHH, we might want a system that uses components to make it easier to program, but idk

    class GameObjectAllocator : public OUtils::SlabAllocator {
        public:
            OUtils::SlabAllocator::Slab slabs[8];

            GameObjectAllocator() {
                // This all consumes a lot of memory, so we'll need to restrict this a lot, and this only needs to be for dynamic objects (static objects can be allocated separately)
                this->slabs[0].init(128, 16384);
                this->slabs[1].init(256, 16384);
                this->slabs[2].init(512, 16384);
                this->slabs[3].init(1024, 16384);
                this->slabs[4].init(2048, 16384);
                this->slabs[5].init(4096, 8096);
                this->slabs[6].init(8096, 4096);
                this->slabs[7].init(16384, 4096);
            }
    };

    extern GameObjectAllocator objallocator;

    // This should be pooled, but we have to use the biggest sized object... (static max for everything?)
    // Small allocator (literally just a slab allocator)
    class GameObject {
        private:
            bool dynallocated; // allocated dynamically
        public:
            Transform transform;
            std::vector<GameObject *> children;

            // XXX: Potentially different versions of these destructors for handling dynamic objects vs. static scene loaded objects (these would be host malloc'd as speed isn't a major concern in level loading)
            void *operator new(size_t size) {
                GameObject *allocation = (GameObject *)objallocator.alloc(size);
                allocation->dynallocated = true;
                return allocation;
            }

            void operator delete(void *ptr) {
                if (((GameObject *)ptr)->dynallocated) {
                    objallocator.free(ptr);
                    return;
                }
            }
    };

    class PointLight : public GameObject {
        public:
            glm::vec3 colour = glm::vec3(1.0f);
            float intensity = 1.0f;
            float range = 1.0f;
    };

    class SpotLight : public GameObject {
        public:
            glm::vec3 colour = glm::vec3(1.0f);
            float intensity = 1.0f;
    };

    class MeshInstance : public GameObject {
        public:

    };


    // The idea behind the bucket system:
    // - inter-object updates may be dependant on each other, bucketed updates allow certain object types to be updated at optimal phases
    // - a collection of a certain type of object allows us to only iterate through objects that implement a certain function or something
    //      - while a physics object may want to be simulated or something every frame (eg. a physicsupdate() function or something), a generic game object that does nothing will not, this could introduce a problem where we are wasting cycles doing an unnessecary call to physicsupdate() (virtual function resolve, etc.) that will do absolutely nothing. a resolution to this problem is to separate certain objects into buckets so that we will only ever do certain things on certain objects (a problem ECS systems resolve similarly)
    class Bucket {
        public:
            enum {
                GENERIC,
                LIGHTS,
                POINTLIGHTS,
                SPOTLIGHTS,
                COUNT
            };

            size_t type;
            std::unordered_set<GameObject *> objs;

            Bucket(size_t type = Bucket::GENERIC) {
                this->type = type;
            }
    };

}

#endif
