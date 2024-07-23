#ifndef _ENGINE__SCENE__PARTITION_HPP
#define _ENGINE__SCENE__PARTITION_HPP

#include <engine/math/math.hpp>
#include <engine/renderer/camera.hpp>
#include <engine/scene/gameobject.hpp>

namespace OScene {

    // https://www.beosil.com/download/CollisionDetectionHashing_VMV03.pdf
    // Proposes a grid cell system for collision detection, however, we can use this grid cell system for culling and apply it to many use cases

    class CellDescriptor {
        public:
            glm::ivec3 pos;

            CellDescriptor(void) { };
            CellDescriptor(glm::vec3 pos, float size) {
                this->pos = pos * (1 / size);
            }

            constexpr bool operator==(const CellDescriptor &rhs) {
                return this->pos == rhs.pos;
            }
    };

    // Custom hash function for std::unordered_map
    class CellDescHasher {
        public:
            // Hash the position of a cell
            size_t operator()(const glm::ivec3 &cell) const {
                // Large prime numbers let us hash these values properly
                return (uint32_t)cell.x * 73856093 + (uint32_t)cell.y * 19349663 + (uint32_t)cell.z * 83492791;
            }
    };

    // 4096 byte aligned cells (so they may fit into a page allocator)
    // NOTE: Multiple of these can be assigned to a single cell position (hence the prev and next members) so that we can exceed the arbitary limit imposed by 4096 byte alignment
    class Cell {
        public:
            struct header {
                // Doubly linked to other cells
                Cell *next = NULL;
                Cell *prev = NULL;
                glm::vec3 origin; // cell position in the world (for making world-local translations for objects)
                glm::ivec3 cellpos; // cell position in grid (for searching)
                uint32_t count; // number of objects in this cell
                size_t id; // unique ID given on cell creation
                size_t idx; // index in cell list
                bool big; // Contains an object whose bounds exceeds the cell
            };

            struct header header;

            static const uint32_t COUNT = (4096 - sizeof(header)) / sizeof(OUtils::Handle<GameObject>);

            OUtils::Handle<GameObject> objects[COUNT];
    } __attribute__((aligned(4096)));

    class CullResult {
        public:
            struct header {
                // Single linked (we have no need for runtime element coherency, and additionally it's all a bunch of linear operations anyway)
                CullResult *next = NULL;
                // XXX: To be considered: Any code that works on culled objects could use the job system to set a job on every page worth of objects
                uint32_t count = 0;
            };

            struct header header;
            static const uint32_t COUNT = (4096 - sizeof(header)) / (sizeof(OUtils::Handle<GameObject>));
            OUtils::Handle<GameObject> objects[COUNT];
    } __attribute__((aligned(4096)));

    class CullResultList {
        public:
            OUtils::PoolAllocator *allocator = NULL;
            CullResult *begin = NULL;
            CullResult *end = NULL;
            // OJob::Mutex mutex;
            OJob::Spinlock spin;
            uint64_t previous = 0;

            CullResultList(OUtils::PoolAllocator *allocator) {
                this->allocator = allocator;
            }

            ~CullResultList(void) {
                ASSERT(this->allocator != NULL, "Invalid allocator, cannot free.\n");
                CullResult *i = this->begin;
                while (i != NULL) {
                    CullResult *tmp = i;
                    i = i->header.next;
                    this->allocator->free(tmp);
                }
            }

            // Detach results list from this handler.
            CullResult *detach(void) {
                CullResult *tmp = this->begin;
                this->previous = 0;
                this->begin = NULL;
                this->end = NULL;
                return tmp;
            }

            // Acquire a new element for the list.
            CullResult *acquire(void) {
                // this->mutex.lock();
                OJob::ScopedSpinlock spin(&this->spin);

                CullResult *res = (CullResult *)this->allocator->alloc();
                memset(res, 0, sizeof(CullResult));
                if (this->begin == NULL) { // List is empty, initialise it.
                    this->begin = res;
                    this->end = res;
                } else {
                    this->end->header.next = res; // Add to end of list
                    res->header.next = NULL;
                    this->end = res; // Set this allocation as the new end of the list
                }
                ASSERT((uint64_t)res != previous, "Reallocating the same pointer with %p.\n", res);
                previous = (uint64_t)res;
                // this->mutex.unlock();
                return res;
            }
    };

    // Cells are created as needed rather than on level load or anything like that.
    // Cells are not 2D, they are 3D (allowing for better culling density)

    // The cell position hash function in CellDescHasher will represent the start the head of the doubly linked cell list, when attempting to place an object in a cell one must loop over all the "same-cells" to find a free slot
    // Problematic: That's an O(N) time complexity function. Potential solutions could include moving the referenced head to the next free slot. However, this does mean objects moved out of a cell will leave free slots in cells no longer referenced.

    // Keep a list of the next free cells? (Linked, so simple enough) would solve the speed issue of O(N) time complexity for search as we always have O(1) search time for a slot with free cells.

    class ParitionManager {
        public:
            struct work {
                std::atomic<size_t> *idx; // Reference to the index atomic.
                CullResultList *list; // Reference to the result list.
                OMath::Frustum *frustum; // Reference to the camera frustum.
                ParitionManager *manager; // Reference to the partition manager this work is for.
            };

            std::unordered_map<glm::ivec3, Cell *, CellDescHasher> map;
            std::vector<Cell *> cells;
            OUtils::PoolAllocator allocator = OUtils::PoolAllocator(4096, 4096, 256, "Dynamic World Partition Culling"); // Generic page allocator (16MB)
            CellDescHasher hasher;
            std::atomic<size_t> idcounter = 1; // Start at one so a zeroed out cell can never be valid

            // Create this only once as a constant so we can have fast runtime comparisons
            const OUtils::Handle<GameObject> INVALIDHANDLE = OUtils::Handle<GameObject>(NULL, SIZE_MAX, SIZE_MAX);

            size_t getfreespot(Cell *cell) {
                // O(N) time complexity, unfortunate but there is no immediate solution I can think of to produce the same result. (Perhaps do what we do for the old resolution table and hash the game object to get a potential spot quicker?)
                for (size_t i = 0; i < cell->COUNT; i++) {
                    if (cell->objects[i] == INVALIDHANDLE) { // We don't use isvalid() here because it's actually faster to do a comparison when we know that this slot will be invalidated automatically when a game object is deleted (and thus, isvalid() would only really waste extra cycles on checking the handle)
                        return i; // Free slot here
                    }
                }
                return SIZE_MAX; // Failed to find a free cell
            }

            // Add an object to a specific cell.
            void addtocell(Cell *cell, OUtils::Handle<GameObject> obj);
            // Add an object to the partition manager system.
            void add(OUtils::Handle<GameObject> obj);
            // Remove an object from the cell it is registered in.
            void remove(OUtils::Handle<GameObject> obj);
            // Update the position of an object in the partition manager system (will also add the object if it's not already there).
            void updatepos(OUtils::Handle<GameObject> obj);

            void freeresults(CullResult *results) {
                while (results != NULL) {
                    CullResult *tmp = results; // Save this temporarily because after the page is freed we cannot guarantee it'll not be overwritten.
                    results = results->header.next;
                    this->allocator.free(tmp);
                }
            }

            void docull(Cell *cell, OMath::Frustum *frustum, CullResult **ret, CullResultList *list);
            CullResult *cull(ORenderer::PerspectiveCamera &camera);
    };
}

#endif
