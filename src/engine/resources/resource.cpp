#include <engine/concurrency/job.hpp>
#include <pthread.h>
#include <engine/resources/resource.hpp>
#include <engine/utils/hash.hpp>

// custom format for models (animations, etc.)
// KTX for textures

namespace OResource {
    ResourceManager manager;

    void ResourceManager::loadrpak(RPak *rpak) {
        ASSERT(rpak != NULL, "Invalid RPak given to resource manager load.\n");
        for (size_t i = 0; i < rpak->header.num; i++) {
            Resource *res = new Resource(rpak, rpak->entries[i].path);
            res->rpakentry = rpak->entries[i];
            this->resources[OUtils::fnv1a(rpak->entries[i].path, strlen(rpak->entries[i].path), FNV1A_SEED)] = res;
        }
    }

    OUtils::Handle<Resource> ResourceManager::create(const char *path) {
        ASSERT(path != NULL, "Invalid path given to resource manager create.\n");
        Resource *ret = new Resource(path);
        this->resources[OUtils::fnv1a(path, strlen(path), FNV1A_SEED)] = ret;
        return ret->gethandle();
    }

    OUtils::Handle<Resource> ResourceManager::create(const char *path, void *src) {
        ASSERT(path != NULL, "Invalid path given to resource manager virtual create.\n");
        ASSERT(src != NULL, "Invalid source pointer given to resource manager virtual create.\n");
        Resource *ret = new Resource(path, src);
        this->resources[OUtils::fnv1a(path, strlen(path), FNV1A_SEED)] = ret;
        return ret->gethandle();
    }

    OUtils::Handle<Resource> ResourceManager::get(const char *path) {
        ASSERT(path != NULL, "Invalid path given to resource manager search.\n");
        auto res = this->resources.find(OUtils::fnv1a(path, strlen(path), FNV1A_SEED));
        if (res == this->resources.end()) { // No such resource, return invalid handle.
            return RESOURCE_INVALIDHANDLE;
        } else {
            return res->second->gethandle();
        }
    }
}