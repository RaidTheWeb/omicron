#ifndef _ENGINE__RENDERER__TEXTURE_HPP
#define _ENGINE__RENDERER__TEXTURE_HPP

#include <engine/renderer/renderer.hpp>
#include <engine/resources/asyncio.hpp>
#include <engine/resources/texture.hpp>

namespace ORenderer {
    // GPU abstraction for texture, this only ever exists when a texture actually exists (aka. is used). This should probably be allocated through a pool allocator with assigned handles so we can easily indirecly reference a texture as well as forcing its resolution. It should be noted however that an additional layer of indirection should probably be used, materials will of course reference textures which would include the information needed to stream their data in and out of GPU memory (file paths, resource handles, etc.) but those textures will probably indirecly reference this data here, or perhaps just null initialise it until it is actually needed. This is problematic to think about because we both need to be able to immediately know all information about the texture from the get go but also have it not exist until its absolutely necessary.
    // What could be done is that when a material is loaded (and becomes used by something) we initialise this texture struct using the information the material knows about the texture. When the material is refcounted to be 0 and texture stops being used, we should get rid of the texture. We can't risk data duplication so textures should be indirect and refcounted too.
    // Simply just sitting in the resource system, nothing more complex than that!
    // Refcounted as well so we can get rid of the resources when we no longer need it.
    // Textures in the VFS also means we can avoid memory duplication as loading a texture that is already loaded will fail as the resource path for the texture is already taken.
    class Texture {
        public:
            // Parent resource on VFS.
            OUtils::Handle<OResource::Resource> resource;
            OUtils::Handle<OResource::Resource> self; // This current resource.
            size_t residentlevels = 0; // How many levels should be kept permanently.

            struct OResource::Texture::loaded headers;

            std::atomic<size_t> refcount = 0; // Reference count (atomic to avoid locking costs) used to count the number of engine internals that hold a reference to the texture (aka. material load will reference, material unload will dereference).

            // on GPU texture data.
            struct texture texture; // GPU texture handle.
            struct textureview textureview; // GPU texture view (changed during streaming).
            uint32_t bindlessid; // ID registered in bindless system.

            struct updateinfo {
                uint64_t timestamp; // Timestamp of last update (so we can exclude out of order updates).
                uint32_t resolution; // Current resolution (derived from the maximum possible value for texture resolution)
            };
            // Mutex locks :D
            // We can use mutexes here because texture streaming is actually a really low priority operation and contention isn't problematic.
            // XXX: There is probably a better way of doing this, look into it.
            OJob::Mutex mutex;
            struct updateinfo updateinfo; // Information of last upload.

            Texture(OUtils::Handle<OResource::Resource> resource);
            // Meet the required resolution.
            void meetresolution(struct updateinfo info);
    };

    class TextureManager {
        public:
            // Texture streaming as described in 28/05/24

            // XXX: Deadline in frames instead? Using a deadline of x frames is a lot friendlier.
            static const uint64_t EVICTDEADLINE = 4; // Deadline for frames that needs to elapse after the last reference to a texture before it is unloaded.
            static const uint64_t PERMANENTRESIDENCY = 64 * 1024; // 64KiB per-texture level permanent residency budget. NOTE: Permanent does not mean it stays forever, it'll just stay for as long as it is actually used (ie. loaded models have a reference to it).

            // create texture handle empty and let it persist, only load up the data when needed.

            // Memory used of budget on texture streaming allocations.
            std::atomic<size_t> memoryused = 0;
            // Memory budget available to texture streaming. 0 is debug for DO NOT TRACK.
            size_t memorybudget = 0;

            OJob::Mutex operationsmutex;
            std::vector<Stream *> activeoperations;

            // Create managed texture from path (loads first mip level to GPU).
            OUtils::Handle<OResource::Resource> create(const char *path);
            OUtils::Handle<OResource::Resource> create(OUtils::Handle<OResource::Resource> resource);


    };

    extern TextureManager texturemanager;
}

#endif
