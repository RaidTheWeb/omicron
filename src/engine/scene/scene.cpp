#include <engine/scene/scene.hpp>

namespace OScene {
#define SCENE_OBJECTSPERLOADCHUNK 256
    void Scene::load(const char *path) {
        ASSERT(path != NULL, "NULL path.\n");
        FILE *f = fopen(path, "r");
        ASSERT(f != NULL, "Failed to open scene file `%s` for loading.\n", path);

        struct scenehdr header = { };
        ASSERT(fread(&header, sizeof(struct scenehdr), 1, f), "Failed to read scene header.\n");
        ASSERT(!strncmp(header.magic, "OSCE", sizeof(header.magic)), "Failed to verify scene header magic.\n");
        printf("Loading scene `%s`.\n", path);
        printf("Number of terrains: %lu\n", header.numterrain);
        printf("Number of objects: %lu\n", header.numobjects);

        std::unordered_map<size_t, GameObject *> idptrmap; // Resolution between serialised ID and pointer

        for (size_t i = 0; i < header.numobjects; i++) {
            struct gameobjecthdr gobj = { };
            ASSERT(fread(&gobj, sizeof(struct gameobjecthdr), 1, f), "Failed to read game object header.\n");

            ASSERT(OUtils::reflectiontable.find(gobj.type) != OUtils::reflectiontable.end(), "Invalid object type (not registered in reflection table).\n");

            // Instantiate without using the pool allocator instead of
            GameObject *obj = (GameObject *)OUtils::reflectiontable[gobj.type]->create(false);
            obj->scene = this;
            obj->position = gobj.position;
            obj->orientation = gobj.orientation;
            obj->scale = gobj.scale;
            obj->parent = SCENE_INVALIDHANDLE;
            obj->sparent = gobj.parent;
            obj->bounds = OMath::AABB(gobj.min, gobj.max);
            if (gobj.numchildren) {
                obj->schildren.reserve(gobj.numchildren);
                ASSERT(fread(obj->schildren.data(), sizeof(size_t) * gobj.numchildren, 1, f), "Failed to read game object children.\n");
            }
            obj->flags = gobj.flags;
            obj->type = gobj.type;
            obj->id = gobj.id;
            idptrmap[gobj.id] = obj;
            if (gobj.datasize) {
                uint8_t *data = (uint8_t *)malloc(gobj.datasize);
                ASSERT(data != NULL, "Failed to allocate memory for object serialised data.\n");
                ASSERT(fread(data, gobj.datasize, 1, f), "Failed to read serialised game object data.\n");
                OResource::Serialiser serialiser = OResource::Serialiser(data, gobj.datasize);
                obj->deserialise(&serialiser);
                free(data);
            }
            this->objects.push_back(obj->gethandle());
        }

        fclose(f);

        // TODO: Concurrent (split into chunks and shove work onto each job)
        size_t numobjects = this->objects.size();
        // Split objects into chunks
        size_t numjobs = numobjects > SCENE_OBJECTSPERLOADCHUNK ? numobjects / SCENE_OBJECTSPERLOADCHUNK : 1;

        for (size_t i = 0; i < numobjects; i++) {
            OUtils::Handle<GameObject> obj = this->objects[i];
            if (obj->sparent) {
                ASSERT(idptrmap.find(obj->sparent) != idptrmap.end(), "Unresolved parent ID (%lu) for object.\n", obj->sparent);
                obj->parent = idptrmap[obj->sparent]->gethandle();
                obj->sparent = 0;
            }

            if (obj->schildren.size()) {
                for (auto it = obj->schildren.begin(); it != obj->schildren.end(); it++) {
                    ASSERT(idptrmap.find(*it) != idptrmap.end(), "Unresolved child ID (%lu) for object.\n", *it);
                    obj->children.push_back(idptrmap[*it]->gethandle());
                }
                obj->schildren.clear();
            }
            this->partitionmanager.add(obj);
        }
    }

    Scene::Scene(const char *path) {
        this->load(path);
    }

    static void saveobj(FILE *f, OUtils::Handle<GameObject> obj) {
        struct Scene::gameobjecthdr gobj = { };
        gobj.position = obj->position;
        gobj.orientation = obj->orientation;
        gobj.scale = obj->scale;
        gobj.flags = obj->flags;
        gobj.type = obj->type;
        gobj.id = obj->id;
        gobj.min = obj->bounds.min;
        gobj.max = obj->bounds.max;
        gobj.parent = obj->parent.isvalid() ? obj->parent->id : 0;
        gobj.numchildren = obj->children.size();

        OResource::Serialiser serialiser = OResource::Serialiser();
        obj->serialise(&serialiser);
        gobj.datasize = serialiser.writeoffset;
        ASSERT(fwrite(&gobj, sizeof(struct Scene::gameobjecthdr), 1, f), "Failed to write game object header.\n");
        for (auto it = obj->children.begin(); it != obj->children.end(); it++) {
            ASSERT(fwrite(&(*it)->id, sizeof(size_t), 1, f), "Failed to write game object children.\n");
        }
        if (gobj.datasize) {
            ASSERT(fwrite(serialiser.data, serialiser.writeoffset, 1, f), "Failed to write serialised game object data.\n");
        }
    }

    void Scene::save(const char *path) {
        ASSERT(path != NULL, "NULL path.\n");
        FILE *f = fopen(path, "w");
        ASSERT(f != NULL, "Failed to open scene file `%s` for scene serialisation.\n", path);

        struct scenehdr header = { };
        strcpy(header.magic, "OSCE");
        header.numterrain = 0; // XXX: TODO
        header.numobjects = this->objects.size();

        ASSERT(fwrite(&header, sizeof(struct scenehdr), 1, f), "Failed to write scene header.\n");

        for (auto it = this->objects.begin(); it != this->objects.end(); it++) {
            saveobj(f, *it);
        }

        fclose(f);
    }
}
