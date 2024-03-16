#ifndef _ENGINE__SCENE__GAMEOBJECT_HPP
#define _ENGINE__SCENE__GAMEOBJECT_HPP

#include <engine/renderer/mesh.hpp>
#include <engine/resources/serialise.hpp>
#include <unordered_set>
#include <engine/utils/memory.hpp>
#include <engine/utils/pointers.hpp>
#include <engine/utils/reflection.hpp>
#include <vector>

namespace OScene {
    // XXX: GRAHHHHHH, we might want a system that uses components to make it easier to program, but idk

    class GameObjectAllocator : public OUtils::SlabAllocator {
        public:
            OUtils::SlabAllocator::Slab slabs[8];

            GameObjectAllocator() {
                // This all consumes a lot of memory, so we'll need to restrict this a lot, and this only needs to be for dynamic objects (static objects can be allocated separately)
                // XXX: Figure out static distinction
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
    extern std::atomic<size_t> objidcounter;

    extern OUtils::ResolutionTable table;
#define SCENE_INVALIDHANDLE OUtils::Handle<OScene::GameObject>(NULL, SIZE_MAX, SIZE_MAX)
    

    class Scene;
    // This should be pooled, but we have to use the biggest sized object... (static max for everything?)
    // Small allocator (literally just a slab allocator)
    class GameObject {
        private:
            bool poolallocated; // allocated dynamically
            size_t handle = 0; // handle index for speedy creation of handles
        public:
            size_t id = 0; // unique game object id
            Scene *scene; // "Parent" scene

            bool invalid; // is invalid (ignore certain deconstructor stuff)
            enum typeflags {
                IS_DYNAMIC = (1 << 0), // should be updated on every game loop cycles
                IS_MODEL = (1 << 1), // possesses a renderable model
                IS_INVISIBLE = (1 << 2), // refuse to render
            };
            uint32_t flags = 0; // So that we can cast between types at runtime

            // Transform data
            glm::vec3 position = glm::vec3(0.0f);
            glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            glm::vec3 scale = glm::vec3(1.0f);
            glm::mat4 matrix;
            std::atomic<bool> dirty = true;

            uint32_t type = OUtils::fnv1a("GameObject");

            OUtils::Handle<GameObject> parent; // This is guaranteed to have been updated before this current object to solve the object dependency problem.
            std::vector<OUtils::Handle<GameObject>> children;
            // Temporary values, aids scene loading
            size_t sparent; // serialised parent
            std::vector<size_t> schildren; // serialised children (freed after load)

            // Culling
            // On object deletion we must remove from culling to prevent occupying spots (also prevents any need for validity checks when iterating over the cell objects)
            struct culldata {
                glm::ivec3 cellpos; // Position of the cell this game object occupies (used to validate a cell page tree exists)
                size_t objid = SIZE_MAX; // ID in cell data
                void *pageref = NULL; // Pointer reference to the current cell page (Used to index directly into linked list of cells) 
                size_t pageid; // Backup ID to prevent stale references to old cells (cells are pool allocated, although this shouldn't be much of a problem as we memset new allocations anyway)
            } culldata;

            template <typename T>
            static T *create(bool poolalloc = true) {
                GameObject *allocation = NULL;
                if (poolalloc) {
                    allocation = (GameObject *)objallocator.alloc(sizeof(T));
                    T t = T();
                    memcpy(allocation, &t, sizeof(T));
                    ((GameObject *)&t)->invalid = true; // invalidate object
                    allocation->poolallocated = true;
                } else {
                    allocation = (GameObject *)malloc(sizeof(T));
                    ASSERT(allocation != NULL, "Failed to allocate memory for game object from host allocator.\n");
                    T t = T();
                    memcpy(allocation, &t, sizeof(T));
                    ((GameObject *)&t)->invalid = true; // invalidate object
                    allocation->poolallocated = false;
                }
                allocation->handle = table.bind(allocation);
                allocation->id = objidcounter.fetch_add(1);
                return (T *)allocation;
            }

            void operator delete(void *ptr) {
                if (((GameObject *)ptr)->poolallocated) {
                    objallocator.free(ptr);
                    return;
                } else {
                    free(ptr);
                }
            }

            // Get a handle to our object
            OUtils::Handle<GameObject> gethandle(void) {
                return OUtils::Handle<GameObject>(&table, this->handle, this->id);
            }

            virtual void serialise(OResource::Serialiser *serialiser) {
            }

            virtual void deserialise(OResource::Serialiser *serialiser) {
            }

            glm::mat4 getmatrix(void);
            glm::mat4 getglobalmatrix(void);
            glm::vec3 getglobalposition(void);
            glm::vec3 getposition(void);
            glm::quat getglobalorientation(void);
            glm::quat getorientation(void);
            glm::vec3 getglobalscale(void);
            glm::vec3 getscale(void);

            // XXX: Figure out how to set global positions (transform based on global position?)
            void setglobalposition(glm::vec3 pos);
            void setposition(glm::vec3 pos);
            void setorientation(glm::quat q);
            void setrotation(glm::vec3 euler);
            void setscale(glm::vec3 scale);
            void translate(glm::vec3 v);
            void orientate(glm::quat q);
            void lookat(glm::vec4 target);
            void scaleby(glm::vec3 s);
    };

    // Omnidirectional light
    class PointLight : public GameObject {
        public:
            glm::vec3 colour = glm::vec3(1.0f);
            float intensity = 1.0f;
            float range = 1.0f;

            PointLight(void) {
                this->type = OUtils::fnv1a("PointLight");
            }

            void serialise(OResource::Serialiser *serialiser) {
                serialiser->write<glm::vec3>(&this->colour);
                serialiser->write<float>(&this->intensity);
                serialiser->write<float>(&this->range);
            }

            void deserialise(OResource::Serialiser *serialiser) {
                serialiser->read<glm::vec3>(&this->colour);
                serialiser->read<float>(&this->intensity);
                serialiser->read<float>(&this->range);
            }
    };

    // Light with specific cone of influence
    class SpotLight : public GameObject {
        public:
            glm::vec3 colour = glm::vec3(1.0f);
            float intensity = 1.0f;

            SpotLight(void) {
                this->type = OUtils::fnv1a("SpotLight");
            }
    };

    // Game object tied to one or more meshes
    class ModelInstance : public GameObject {
        public:
            ORenderer::Model model;
            char *modelpath;

            ModelInstance(void) {
                flags |= GameObject::IS_MODEL;
                this->type = OUtils::fnv1a("ModelInstance");
            }

            void serialise(OResource::Serialiser *serialiser) {
                uint32_t len = 0;
                if (this->modelpath != NULL) {
                    len = strlen(this->modelpath);
                    serialiser->write<uint32_t>(&len);
                    for (size_t i = 0; i < len; i++) {
                        serialiser->write<char>(&this->modelpath[i]);
                    }
                } else {
                    serialiser->write<uint32_t>(&len);
                }
            }

            void deserialise(OResource::Serialiser *serialiser) {
                uint32_t len = 0;
                serialiser->read<uint32_t>(&len);
                if (len > 0) {
                    this->modelpath = (char *)malloc(len + 1);
                    ASSERT(this->modelpath, "Failed to allocate memory for deserialising the model path.\n");
                    for (size_t i = 0; i < len; i++) {
                        serialiser->read<char>(&this->modelpath[i]);
                    }
                    this->modelpath[len] = '\0';
                    this->model = ORenderer::Model(this->modelpath);
                }
            }
    };

    // Handle adding the game object types to the reflection table.
    void setupreflection(void);

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
