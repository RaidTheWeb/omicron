#include <engine/math/bounds.hpp>
#include <engine/scene/partition.hpp>
#include <tracy/Tracy.hpp>

namespace OScene {
    #define PARTITION_CELLSIZE 150.0f

    void ParitionManager::addtocell(Cell *cell, OUtils::Handle<GameObject> obj) {
        ZoneScoped;
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

        Cell *newcell = (Cell *)this->allocator.alloc();
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
        ZoneScoped;
        ASSERT(obj.isvalid(), "Attempted to register invalid object to partition system.\n");
        Cell *icell = NULL;
        const glm::ivec3 cellpos = obj->getglobalposition() * (1 / PARTITION_CELLSIZE);
        if (this->map.find(cellpos) != this->map.end()) { // XXX: Rework this so we only have one call to do
            icell = this->map[cellpos]; // Retrieve head of cell page list
        } else {
            Cell *cell = (Cell *)this->allocator.alloc();
            memset(cell, 0, sizeof(Cell));
            for (size_t i = 0; i < cell->COUNT; i++) {
                cell->objects[i] = SCENE_INVALIDHANDLE;
            }
            cell->header.origin = glm::vec3(cellpos.x, cellpos.y, cellpos.z) * PARTITION_CELLSIZE; // Convert to cell coordinates and back to lose precision
            cell->header.cellpos = cellpos;
            cell->header.next = NULL;
            cell->header.prev = NULL;
            cell->header.count = 0;
            cell->header.idx = this->cells.size();
            cell->header.id = this->idcounter.fetch_add(1);
            this->cells.push_back(cell);
            this->map[cell->header.cellpos] = cell;
            icell = this->map[cell->header.cellpos];
        }

        this->addtocell(icell, obj);
    }
    void ParitionManager::remove(OUtils::Handle<GameObject> obj) {
        ZoneScoped;
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
                    this->cells.erase(this->cells.begin() + cell->header.idx);
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
            this->allocator.free(cell); // Free from the allocator so that this may be used once again
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
        ZoneScoped;
        ASSERT(obj.isvalid(), "Attempted to remove invalid object from partition.\n");

        if (obj->culldata.objid == SIZE_MAX) { // Not already in the system, add it.
            this->add(obj);
            return;
        }

        glm::ivec3 current = obj->getglobalposition() * (1 / PARTITION_CELLSIZE);
        if (current == obj->culldata.cellpos) {
            return; // Literally no need to update the position of the object in our system as nothing has changed (and it would be a waste of cycles to go through the whole remove+add routine).
        }

        this->remove(obj); // remove from old cell
        this->add(obj); // get us a new cell
    }

    void ParitionManager::docull(Cell *cell, OMath::Frustum *frustum, CullResult **ret, CullResultList *list) {
        ZoneScoped;
        CullResult *res = *ret;
        size_t cursor = res->header.count;

        while (cell != NULL) { // Iterate over all cells in the chain.
            for (size_t i = 0; i < cell->header.count; i++) {
                if (!(cell->objects[i]->flags & GameObject::typeflags::IS_CULLABLE)) {
                    continue;
                }

                OUtils::Handle<GameObject> obj = cell->objects[i]->gethandle<GameObject>();
                OMath::AABB bounds = obj->bounds;
                // bounds = bounds.transformed(glm::translate(obj->getglobalposition()) * glm::toMat4(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)) * glm::scale(obj->getglobalscale()));
                bounds = bounds.transformed(obj->getglobalstaticmatrix());
                // bounds = bounds.transformed(obj->getglobalmatrix());
                // bounds.translate(obj->getglobalposition());
                OMath::Sphere sphere = OMath::Sphere(bounds.centre, bounds.radius());

                // printf("checking bounds %f %f %f to %f %f %f\n", bounds.min.x, bounds.min.y, bounds.min.z, bounds.max.x, bounds.max.y, bounds.max.z);
                // printf("checking sphere %f %f %f %f\n", sphere.pos.x, sphere.pos.y, sphere.pos.z, sphere.radius);
                if (frustum->testsphere(sphere) == OMath::Frustum::OUTSIDE) {
                // if (camera.getfrustum().testobb(OMath::OBB(bounds, obj->getglobalorientation())) == OMath::Frustum::OUTSIDE) {
                // if (camera.getfrustum().testaabb(bounds) == OMath::Frustum::OUTSIDE) {
                // if (!camera.getfrustum().sphereinfrustum(sphere)) {
                    continue;
                }

                if (cursor == CullResult::COUNT) {
                    res->header.count = cursor;
                    res = list->acquire();
                    cursor = 0;
                }

                res->objects[cursor++] = cell->objects[i];
            }
            res->header.count = cursor;

            cell = cell->header.next;
        }
        *ret = res;
    }

    static void cullworker(OJob::Job *job) {
        ZoneScoped;
        struct ParitionManager::work *work = (struct ParitionManager::work *)job->param;
        ParitionManager *manager = work->manager;
        CullResultList list = CullResultList(&manager->allocator); // create a new result list
        OMath::Frustum *frustum = work->frustum;

        size_t start = work->idx->fetch_add(work->cellsperjob); // get the current working index, while adding the number of cells to work through for the next worker.
        size_t end = MIN(start + work->cellsperjob, manager->cells.size()); // either do all the cells that a worker is supposed to do, or work until the end of the list of cells.

        CullResult *res = NULL;
        size_t total = 0;
        for (size_t index = start; index < end; index++) {
            total++;

            Cell *cell = manager->cells[index];

            if (res == NULL) {
                res = list.acquire();
            }

            if (frustum->testaabb(OMath::AABB(cell->header.origin, cell->header.origin + PARTITION_CELLSIZE)) == OMath::Frustum::INSIDE) {
                size_t remaining = cell->header.count;
                size_t srcoff = 0;

                // Directly copy everything inside the cell into the result implicitly (as we can assume total containment == all are visible).
                while (remaining > 0) {
                    if (res->header.count == CullResult::COUNT) {
                        res = list.acquire();
                    }
                    size_t remspace = CullResult::COUNT - res->header.count;
                    size_t step = glm::min(remaining, remspace);

                    memcpy(res->objects + res->header.count, cell->objects + srcoff, step * sizeof(cell->objects[0]));
                    srcoff += step;
                    res->header.count += step;
                    remaining -= step;
                }
            } else if (frustum->testaabb(OMath::AABB(cell->header.origin - PARTITION_CELLSIZE, cell->header.origin + PARTITION_CELLSIZE)) == OMath::Frustum::INTERSECT) {
                manager->docull(cell, frustum, &res, &list);
            }
        }

        job->returnvalue = list.detach(); // our job's return value will be a detached list of culling nodes, that can be merged in later with the main result list.
    }

    CullResult *ParitionManager::cull(ORenderer::PerspectiveCamera &camera) {
        ZoneScoped;
        if (!this->map.size()) {
            return NULL;
        }

        CullResultList list = CullResultList(&this->allocator);
        std::atomic<size_t> workeridx = 0; // Index into cell map list, atomic so only one worker is working on a cell at any one time.

        size_t cellsperjob = MAX(1, this->cells.size() / (OJob::numworkers * 2)); // even spread, but ensure at least one cell per job if the number of worker threads exceeds the number of cells. additionally, pessimistically assume that the job system will be in heavy use.
        size_t numjobs = (this->cells.size() + cellsperjob - 1) / cellsperjob; // balance the number of jobs based on the number of cells and the number of cells per job.

        printf("Dispatching culling work on %lu cells at a saturation of %lu jobs, with %lu cells per job.\n", this->cells.size(), numjobs, cellsperjob);
        struct work work = { .idx = &workeridx, .frustum = &camera.getfrustum(), .manager = this, .cellsperjob = cellsperjob };
        OJob::Counter *counter = new OJob::Counter();
        OJob::Job **jobs = (OJob::Job **)malloc(sizeof(OJob::Job *) * numjobs);
        ASSERT(jobs != NULL, "Failed to allocate memory for job list.\n");
        for (size_t i = 0; i < numjobs; i++) { // Create a series of jobs to distribute the work over the worker threads.
            jobs[i] = new OJob::Job(cullworker, (uintptr_t)&work);
            jobs[i]->returns = true; // mark that this job returns a value.
            jobs[i]->counter = counter;
            OJob::kickjob(jobs[i]);
        }

        counter->wait();
        delete counter;

        for (size_t i = 0; i < numjobs; i++) {
            void *ret = jobs[i]->returnvalue;
            ASSERT(ret != NULL, "Job returned null value, it should be returning a result node.\n");
            list.merge((CullResult *)ret); // merge results into main list.
            delete jobs[i]; // clean up jobs.
        }
        free(jobs);

        return list.detach(); // Detach list from the handler.
    }

}
