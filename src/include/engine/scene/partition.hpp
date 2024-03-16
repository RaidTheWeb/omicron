#ifndef _ENGINE__SCENE__PARTITION_HPP
#define _ENGINE__SCENE__PARTITION_HPP

#include <engine/math.hpp>
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

    // 4096 byte aligned cells
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
            };

            struct header header;
    
            static const uint32_t COUNT = (4096 - sizeof(header)) / (sizeof(OMath::Sphere) + sizeof(OUtils::Handle<GameObject>));
    
            OUtils::Handle<GameObject> objects[COUNT];
            OMath::Sphere spheres[COUNT];
    } __attribute__((aligned(4096)));

    // Cells are created as needed rather than on level load or anything like that.
    // Cells are not 2D, they are 3D (allowing for better culling density)

    // The cell position hash function in CellDescHasher will represent the start the head of the doubly linked cell list, when attempting to place an object in a cell one must loop over all the "same-cells" to find a free slot
    // Problematic: That's an O(N) time complexity function. Potential solutions could include moving the referenced head to the next free slot. However, this does mean objects moved out of a cell will leave free slots in cells no longer referenced.

    // Keep a list of the next free cells? (Linked, so simple enough) would solve the speed issue of O(N) time complexity for search as we always have O(1) search time for a slot with free cells.

    class ParitionManager {
        public:
            std::unordered_map<glm::ivec3, Cell *, CellDescHasher> map;
            OUtils::PoolAllocator cellallocator = OUtils::PoolAllocator(sizeof(Cell), 1024, 128);
            CellDescHasher hasher;
            std::atomic<size_t> idcounter = 1; // Start at one so a zeroed out cell can never be valid

            size_t getfreespot(Cell *cell) {
                // O(N) time complexity, unfortunate but there is no immediate solution I can think of to produce the same result. (Perhaps do what we do for the old resolution table and hash the game object to get a potential spot quicker?)  
                for (size_t i = 0; i < cell->COUNT; i++) {
                    if (cell->objects[i] == SCENE_INVALIDHANDLE) { // We don't use isvalid() here because it's actually faster to do a comparison when we know that this slot will be invalidated automatically when a game object is deleted (and thusly isvalid() would only really waste extra cycles on checking the handle)
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
    };
}

#endif
