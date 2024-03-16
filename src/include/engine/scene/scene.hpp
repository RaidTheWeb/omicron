#ifndef _ENGINE__SCENE__SCENE_HPP
#define _ENGINE__SCENE__SCENE_HPP

#include <engine/scene/gameobject.hpp>
#include <engine/scene/partition.hpp>
#include <vector>

namespace OScene {

    class Scene {
        public:
            struct gameobjecthdr {
                size_t id; // object ID
                uint32_t type; // object type FNV1a hash (for RTTI)
                uint32_t flags; // object flags
                // transform
                glm::vec3 position;
                glm::quat orientation;
                glm::vec3 scale; 
                size_t parent; // parent object ID
                uint32_t numchildren; // number of children objects in children[]
                uint32_t datasize; // serialised data size for data[]
                // size_t children[]
                // uint8_t data[]
            };

            // XXX: Consider terrain as a static game object instead
            struct terrainhdr {

            } __attribute__((packed));

            struct scenehdr {
                char magic[5]; // OSCE
                size_t numterrain; // terrains are described before objects
                size_t numobjects; // objects take up the rest of the file
            } __attribute__((packed));

            Scene(const char *path);
            void load(const char *path);
            void save(const char *path);

            // Header
            // Terrains...
            // Objects...
            //      Header
            //      Children
            //      Data




            // description of the actively loaded scene

            // XXX: Collection of high level objects only with children considered below? That approach would resolve inter-object dependancy issues where children rely on the parent to have updated before it does
            std::vector<OUtils::Handle<GameObject>> objects; // should this be shoved into the culling system? object updates should be handled through the culling system (so we only update objects that are within simulation distance, distinction between this and static should be handled too)
            // should static objects be handled outside the culling system? this is inadvisable as it doesn't allow us to frustum cull.
            // how should the scene tree be worked out?
            ParitionManager partitionmanager; // Handles the partitions used for culling and other world-space operations that would benefit from optimising the number of objects worked on

            Scene(void) {

            }
    };

}

#endif
