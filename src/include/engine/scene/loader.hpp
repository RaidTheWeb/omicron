#ifndef _ENGINE__SCENE__LOADER_HPP
#define _ENGINE__SCENE__LOADER_HPP

#include <engine/scene/gameobject.hpp>
#include <engine/scene/scene.hpp>

namespace OScene {

    struct gameobjecthdr {
        uint32_t type;
        glm::vec3 position;
        glm::quat orientation;
        glm::vec3 scale;
        uint32_t parent;
        uint32_t numchildren;
        uint32_t datasize;
        // uint32_t children[]
        // uint8_t data[]
    } __attribute__((packed));

    struct terrainhdr {

    } __attribute__((packed));

    struct scenehdr {
        char magic[5]; // OSCE
        size_t numterrain; // terrains are described before objects
        size_t numobjects; // objects take up the rest of the file
    } __attribute__((packed));

    // Header
    // Terrains...
    // Objects...
    //      Header
    //      Children
    //      Data

    void savescene(const char *path, Scene *scene);
    Scene *loadscene(const char *path);
}

#endif
