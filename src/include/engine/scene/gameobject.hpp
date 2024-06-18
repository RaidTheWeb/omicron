#ifndef _ENGINE__SCENE__GAMEOBJECT_HPP
#define _ENGINE__SCENE__GAMEOBJECT_HPP

#include <engine/renderer/mesh.hpp>
#include <engine/resources/serialise.hpp>
#include <engine/math/math.hpp>
#include <engine/utils/memory.hpp>
#include <engine/utils/pointers.hpp>
#include <engine/utils/reflection.hpp>
#include <unordered_set>
#include <vector>

namespace OScene {
    // XXX: GRAHHHHHH, we might want a system that uses components to make it easier to program, but idk

    class GameObjectAllocator {
        public:
            OUtils::SlabAllocator::Slab slabs[8];

            GameObjectAllocator(const char *name = NULL) {
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

            OUtils::SlabAllocator::Slab *optimalslab(size_t size) {
                for (size_t i = 0; i < sizeof(this->slabs) / sizeof(this->slabs[0]); i++) {
                    OUtils::SlabAllocator::Slab *slab = &this->slabs[i];
                    // Slab allocator
                    if (slab->entsize >= size && slab->allocator.allocblock != NULL) {
                        return slab;
                    }
                }
                return NULL;
            }

            void *alloc(size_t size) {
                OUtils::SlabAllocator::Slab *slab = this->optimalslab(size);
                if (slab != NULL) {
                    void *allocation = slab->alloc(size);
                    TracySecureAllocN(allocation, sizeof(struct OUtils::SlabAllocator::metadata) + size, "GameObjects and Components");
                    return allocation;
                }

                // Fallback to host memory allocator (yikers!)
                struct OUtils::SlabAllocator::metadata *allocation = (struct OUtils::SlabAllocator::metadata *)malloc(size + sizeof(struct OUtils::SlabAllocator::metadata));
                ASSERT(allocation != NULL, "Failed to allocate memory from fallback host memory allocator.\n");
                allocation->size = size;
                TracySecureAllocN(allocation, sizeof(struct OUtils::SlabAllocator::metadata) + size, "GameObjects and Components Host Fallback");
                TracySecureAllocN(allocation, sizeof(struct OUtils::SlabAllocator::metadata) + size, "GameObjects and Components");
                return allocation + 1;
            }

            virtual void free(void *ptr) {
                struct OUtils::SlabAllocator::metadata *allocation = (struct OUtils::SlabAllocator::metadata *)((uint8_t *)ptr - sizeof(struct OUtils::SlabAllocator::metadata));
                OUtils::SlabAllocator::Slab *slab = this->optimalslab(allocation->size);
                if (slab != NULL) {
                    TracySecureFreeN(allocation, "GameObjects and Components");
                    slab->free(allocation);
                } else {
                    TracySecureFreeN(allocation, "GameObjects and Components Host Fallback");
                    std::free(allocation);
                }
            }

    };

    // Shared between both objects and components (as it is just a generic slab allocator)
    extern GameObjectAllocator objallocator;
    extern std::atomic<size_t> objidcounter;

    extern OUtils::ResolutionTable table;
#define SCENE_INVALIDHANDLE OUtils::Handle<OScene::GameObject>(NULL, SIZE_MAX, SIZE_MAX)

    // Early exit on invalid objects/components
#define SCENE_INVALIDATION() \
        if (this->invalid) { \
            return; \
        }

#define COMPONENT_GETRESOLVER(...) \
        switch (type) { \
            __VA_ARGS__ \
            default: \
                ASSERT(false, "An invalid component was requested of object.\n"); \
        };

#define COMPONENT_GETRESOLUTION(TYPE, COMPONENT) \
        case (TYPE): return (COMPONENT);

#define COMPONENT_HASRESOLVER(...) \
        switch (type) { \
            __VA_ARGS__ \
            default: \
                return false; \
        };

#define COMPONENT_HASRESOLUTION(TYPE) \
        case (TYPE): return true;

    class Scene;
    class Component;

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
                IS_CULLABLE = (1 << 2), // possesses bounds
                IS_INVISIBLE = (1 << 3), // refuse to render
            };
            uint32_t flags = 0; // So that we can cast between types at runtime

            uint64_t components = 0;

            // Transform data
            glm::vec3 position = glm::vec3(0.0f);
            glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            glm::vec3 scale = glm::vec3(1.0f);
            glm::mat4 matrix;
            glm::mat4 staticmatrix;
            enum dirtyflags {
                DIRTY_MATRIX = (1 << 0),
                DIRTY_STATIC = (1 << 1),
                DIRTY_ALL = DIRTY_MATRIX | DIRTY_STATIC
            };
            std::atomic<uint8_t> dirty = dirtyflags::DIRTY_ALL;

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
            OMath::AABB bounds;

            virtual void construct(void) { }
            virtual void deconstruct(void) { }

            template <typename T>
            static T *create(bool poolalloc = true) {
                GameObject *allocation = NULL;
                if (poolalloc) {
                    allocation = (GameObject *)objallocator.alloc(sizeof(T));
                    T t = T();
                    memcpy(allocation, &t, sizeof(T));
                    // ((GameObject *)&t)->invalid = true; // invalidate object
                    allocation->poolallocated = true;
                } else {
                    allocation = (GameObject *)malloc(sizeof(T));
                    ASSERT(allocation != NULL, "Failed to allocate memory for game object from host allocator.\n");
                    T t = T();
                    memcpy(allocation, &t, sizeof(T));
                    // ((GameObject *)&t)->invalid = true; // invalidate object
                    allocation->poolallocated = false;
                }
                allocation->handle = table.bind(allocation);
                allocation->id = objidcounter.fetch_add(1);
                allocation->construct();
                return (T *)allocation;
            }

            static void destroy(OUtils::Handle<GameObject> obj) {
                obj->deconstruct();
                size_t handle = obj->handle;
                if (obj->poolallocated) {
                    objallocator.free(obj.resolve());
                    table.release(obj->handle);
                    return;
                } else {
                    free(obj.resolve());
                    table.release(obj->handle);
                }
            }

            // Get a handle to our object
            OUtils::Handle<GameObject> gethandle(void) {
                return OUtils::Handle<GameObject>(&table, this->handle, this->id);
            }

            // Get a handle to our object as another object type
            template <typename T>
            OUtils::Handle<T> gethandle(void) {
                return OUtils::Handle<T>(&table, this->handle, this->id);
            }

            virtual void serialise(OResource::Serialiser *serialiser) { }
            virtual void deserialise(OResource::Serialiser *serialiser) { }

            virtual OUtils::Handle<Component> getcomponent(uint32_t type) { return OUtils::Handle<Component>(NULL, SIZE_MAX, SIZE_MAX); }
            virtual bool hascomponent(uint32_t type) { return false; }

            // Returns the static matrix (not orientated) of the object.
            glm::mat4 getstaticmatrix(void);
            glm::mat4 getglobalstaticmatrix(void);
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

    class Component {
        private:
            size_t handle = 0;
            bool poolallocated;
            OUtils::Handle<GameObject> owner;
        public:
            size_t id = 0;
            bool invalid;

            virtual void construct(void) { }
            virtual void deconstruct(void) { }

            template <typename T>
            static T *create(OUtils::Handle<GameObject> owner, bool poolalloc = true) {
                Component *allocation = NULL;
                if (poolalloc) {
                    allocation = (Component *)objallocator.alloc(sizeof(T));
                    T t = T();
                    memcpy(allocation, &t, sizeof(T));
                    // ((Component *)&t)->invalid = true; // invalidate object
                    allocation->poolallocated = true;
                } else {
                    allocation = (Component *)malloc(sizeof(T));
                    ASSERT(allocation != NULL, "Failed to allocate memory for game object from host allocator.\n");
                    T t = T();
                    memcpy(allocation, &t, sizeof(T));
                    // ((Component *)&t)->invalid = true; // invalidate object
                    allocation->poolallocated = false;
                }
                allocation->handle = table.bind(allocation);
                allocation->id = objidcounter.fetch_add(1);
                allocation->owner = owner;
                allocation->construct();
                return (T *)allocation;
            }

            static void destroy(OUtils::Handle<Component> component) {
                component->deconstruct();
                size_t handle = component->handle;
                if (component->poolallocated) {
                    objallocator.free(component.resolve());
                    table.release(handle);
                    return;
                } else {
                    free(component.resolve());
                    table.release(handle);
                }
            }

            virtual void serialise(OResource::Serialiser *serialiser) { }

            virtual void deserialise(OResource::Serialiser *serialier) { }

            // Get a handle to our component
            OUtils::Handle<Component> gethandle(void) {
                return OUtils::Handle<Component>(&table, this->handle, this->id);
            }

            // Get a handle to our component as a different component type
            template <typename T>
            OUtils::Handle<T> gethandle(void) {
                return OUtils::Handle<T>(&table, this->handle, this->id);
            }
    };

    // Omnidirectional light
    class PointLight : public Component {
        public:
            glm::vec3 colour = glm::vec3(1.0f);
            float intensity = 1.0f;
            float range = 1.0f;

            PointLight(void) {
                // this->type = OUtils::fnv1a("PointLight");
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
    class SpotLight : public Component {
        public:
            glm::vec3 colour = glm::vec3(1.0f);
            float intensity = 1.0f;

            SpotLight(void) {
                // this->type = OUtils::fnv1a("SpotLight");
            }

            void serialise(OResource::Serialiser *serialiser) {
                serialiser->write<glm::vec3>(&this->colour);
                serialiser->write<float>(&this->intensity);
            }

            void deserialise(OResource::Serialiser *serialiser) {
                serialiser->read<glm::vec3>(&this->colour);
                serialiser->read<float>(&this->intensity);
            }
    };

    // Game object tied to one or more meshes
    class ModelInstance : public Component {
        public:
            OUtils::Handle<OResource::Resource> model;
            char *modelpath;

            ModelInstance(void) {
                // flags |= GameObject::IS_MODEL;
                // this->type = OUtils::fnv1a("ModelInstance");
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
                if (len > 0) { // XXX: TODO: Resolve resource.
                    this->modelpath = (char *)malloc(len + 1);
                    ASSERT(this->modelpath, "Failed to allocate memory for deserialising the model path.\n");
                    for (size_t i = 0; i < len; i++) {
                        serialiser->read<char>(&this->modelpath[i]); // Read in the model path from the serialiser incrementally.
                    }
                    this->modelpath[len] = '\0';
                    char *loaded = (char *)malloc(len + 2);
                    ASSERT(loaded != NULL, "Failed to allocate memory for marking a loader version of the model path.\n");
                    snprintf(loaded, len + 2, "%s*", this->modelpath); // Represent the virtual resource as the same path but with an asterisk to mark it "loaded". If the model path remains the same for multiple objects, we'll end up just grabbing one that's already been loaded instead of loading it again and consuming more memory.
                    loaded[len + 1] = '\0';

                    // Formatted like this so we'll end up setting the model either way if it *does* exist without having to do it twice
                    // This code here will basically let us load an already loaded copy of the model whenever we need it to be loaded.
                    if ((this->model = OResource::manager.get(loaded)) == RESOURCE_INVALIDHANDLE) {
                        printf("creating new model.\n");
                        OResource::manager.create(loaded, new ORenderer::Model(this->modelpath));
                        this->model = OResource::manager.get(loaded);
                    } else {
                        free(loaded); // Not needed to be persistent for the resource manager, free it.
                    }
                }
            }
    };

    class Test : public GameObject {
        public:
            OUtils::Handle<ModelInstance> model;
            OUtils::Handle<SpotLight> light;

            void construct(void) {
                this->type = OUtils::STRINGID("Test");
                this->flags |= IS_CULLABLE;
                this->model = Component::create<ModelInstance>(this->gethandle())->gethandle<ModelInstance>();
                this->light = Component::create<SpotLight>(this->gethandle())->gethandle<SpotLight>();
            }

            void deconstruct(void) {
                Component::destroy(this->model);
                Component::destroy(this->light);
            }

            bool hascomponent(uint32_t type) {
                ZoneScoped;
                COMPONENT_HASRESOLVER(
                    COMPONENT_HASRESOLUTION(OUtils::STRINGID("ModelInstance"));
                    COMPONENT_HASRESOLUTION(OUtils::STRINGID("SpotLight"));
                );
            }

            OUtils::Handle<Component> getcomponent(uint32_t type) {
                ZoneScoped;
                COMPONENT_GETRESOLVER(
                    COMPONENT_GETRESOLUTION(OUtils::STRINGID("ModelInstance"), this->model);
                    COMPONENT_GETRESOLUTION(OUtils::STRINGID("SpotLight"), this->light);
                );
            }

            void serialise(OResource::Serialiser *serialiser) {
                this->model->serialise(serialiser);
                this->light->serialise(serialiser);
            }

            void deserialise(OResource::Serialiser *serialiser) {
                this->model->deserialise(serialiser);
                this->light->deserialise(serialiser);
            }

            void silly(void) {
                // printf("%f\n", this->light->intensity);
                // printf("%s\n", this->model->modelpath);
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
