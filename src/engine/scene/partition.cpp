#include <engine/scene/partition.hpp>

namespace OScene {

    void ParitionManager::addtocell(Cell *cell, OUtils::Handle<GameObject> obj) {
        Cell *head = cell->header.prev != NULL ? this->map[cell->header.cellpos] : cell; // Head of our cell list

        if (cell->header.count < cell->COUNT - 1) { // This header has a free spot somewhere
            size_t slot = this->getfreespot(cell);
            ASSERT(slot != SIZE_MAX, "Cell reported having a free slot but none was found.\n");

            cell->objects[slot] = obj;
            obj->culldata = (struct GameObject::culldata) {
                .cellpos = head->header.cellpos, // head of the cell list
                .objid = slot, // slot in object list
                .pageref = cell, // cell page in cell list
                .pageid = cell->header.id // ID of page in cell list
            };
            cell->header.count++;
            return;
        }

        // O(N) time complexity, consider using a "free list" (this will leave less space for objects per page as we'll need to keep information about the next free cell data, but it will turn time complexity to O(1) to search for the next free spot)
        cell = cell->header.next;
        while (cell != NULL) {
            if (cell->header.count < cell->COUNT - 1) { // Check for a free spot
                size_t slot = this->getfreespot(cell);
                ASSERT(slot != SIZE_MAX, "Cell reported having a free slot but none was found.\n");

                cell->objects[slot] = obj;
                obj->culldata = (struct GameObject::culldata) {
                    .cellpos = head->header.cellpos, // head of the cell list
                    .objid = slot, // slot in object list
                    .pageref = cell, // cell page in cell list
                    .pageid = cell->header.id // ID of page in cell list
                };
                cell->header.count++;
                return;
            }

            cell = cell->header.next;
        }
       
        Cell *newcell = (Cell *)this->cellallocator.alloc();
        memset(newcell, 0, sizeof(Cell));
        for (size_t i = 0; i < newcell->COUNT; i++) {
            newcell->objects[i] = SCENE_INVALIDHANDLE;
        }
        newcell->header.origin = head->header.origin;
        newcell->header.cellpos = head->header.cellpos;
        // Place this new cell just after the head cell (so future placements need not reposition everything)
        if (head->header.next != NULL) {
            newcell->header.next = head->header.next;
        }
        newcell->header.prev = head;
        newcell->header.id = this->idcounter.fetch_add(1);

        if (head->header.next != NULL) {
            newcell->header.next->header.prev = newcell; // make sure we're just before the next cell
        }
        head->header.next = newcell;
        
        newcell->header.count = 1;
        newcell->objects[0] = obj;
        obj->culldata = (struct GameObject::culldata) {
            .cellpos = head->header.cellpos, // head of the cell list
            .objid = 0, // slot in object list
            .pageref = newcell, // cell page in cell list
            .pageid = newcell->header.id // ID of page in cell list
        };
    }

    void ParitionManager::add(OUtils::Handle<GameObject> obj) {
        ASSERT(obj.isvalid(), "Attempted to register invalid object to partition system.\n");
        Cell *icell = NULL;
        const glm::ivec3 cellpos = obj->getglobalposition() * (1 / 300.0f);
        if (this->map.find(cellpos) != this->map.end()) { // XXX: Rework this so we only have one call to do
            glm::vec3 pos = obj->getglobalposition();
            icell = this->map[cellpos]; // Retrieve head of cell page list
        } else {
            glm::vec3 pos = obj->getglobalposition();
            Cell *cell = (Cell *)this->cellallocator.alloc();
            memset(cell, 0, sizeof(Cell));
            for (size_t i = 0; i < cell->COUNT; i++) {
                cell->objects[i] = SCENE_INVALIDHANDLE;
            }
            cell->header.origin = (pos * (1 / 300.0f)) * 300.0f; // Convert to cell coordinates and back to lose precision
            cell->header.cellpos = cellpos;
            cell->header.next = NULL;
            cell->header.prev = NULL;
            cell->header.count = 0;
            cell->header.id = this->idcounter.fetch_add(1);
            this->map[cell->header.cellpos] = cell;
            icell = this->map[cell->header.cellpos];
        }

        this->addtocell(icell, obj);
    }
    void ParitionManager::remove(OUtils::Handle<GameObject> obj) {
        ASSERT(obj.isvalid(), "Attempted to remove invalid object from partition.\n");

        struct GameObject::culldata &cdata = obj->culldata;
        ASSERT(cdata.pageid != 0, "Object is not registered in the partition system.\n");
        ASSERT(cdata.objid < Cell::COUNT, "Object index in cell exceeds maximum number of objects [in a cell].\n");

        ASSERT(this->map.find(cdata.cellpos) != this->map.end(), "Object culling data refers to a cell that is not mapped.\n");

        ASSERT(cdata.pageref != NULL, "Cell referenced is located at NULL, therefore the object is not registered in the partition system.\n");
        Cell *cell = (Cell *)cdata.pageref;
        ASSERT(cell->header.id != 0, "Reference to stale cell (does not exist in the partition system).\n");

        if (cell->header.count == 1) { // Removing this object will make the cell contain nothing
            if (!cell->header.prev) { // Cell is the head of a list
                if (!cell->header.next) { // Only cell in list, invalidate the entire map reference
                    this->map.erase(cell->header.cellpos);
                } else {
                    this->map[cell->header.cellpos] = cell->header.next; // Make this next cell the new head of the list
                } 
            }

            if (cell->header.prev) { // Normal cell in the list
                cell->header.prev->header.next = cell->header.next; // Make the previous cell forget we exist
            }
            if (cell->header.next) { // Normal cell somewhere inside the list
                cell->header.next->header.prev = cell->header.prev; // Make the next cell forget we exist 
            }

            cell->header.id = 0; // Invalidate cell (it will now be picked up as a stale reference as a cell's id will NEVER be 0)
            this->cellallocator.free(cell); // Free from the allocator so that this may be used once again
        } else {
            cell->objects[cdata.objid] = SCENE_INVALIDHANDLE; // Invalidate the reference
            cell->header.count--; // Decrement the number of claimed spots
        }

        // Invalidate culling data
        obj->culldata = (struct GameObject::culldata) {
            .cellpos = glm::ivec3(0, 0, 0),
            .objid = SIZE_MAX,
            .pageref = NULL,
            .pageid = 0
        };
    }

    void ParitionManager::updatepos(OUtils::Handle<GameObject> obj) {
        ASSERT(obj.isvalid(), "Attempted to remove invalid object from partition.\n");
        
        if (obj->culldata.objid == SIZE_MAX) { // Not already in the system, add it.
            this->add(obj);
            return;
        }

        glm::ivec3 current = obj->getglobalposition() * (1 / 300.0f);
        if (current == obj->culldata.cellpos) {
            return; // Literally no need to update the position of the object in our system as nothing has changed (and it would be a waste of cycles to go through the whole remove+add routine).
        }

        this->remove(obj); // remove from old cell
        this->add(obj); // get us a new cell
    }

}
