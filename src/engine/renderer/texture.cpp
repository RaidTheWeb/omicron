#include <engine/renderer/bindless.hpp>
#include <engine/renderer/texture.hpp>
#include <engine/resources/texture.hpp>

namespace ORenderer {

    Texture::Texture(OUtils::Handle<OResource::Resource> resource) {
        ZoneScoped;
        // Grab headers
        this->headers = OResource::Texture::loadheaders(resource);
        this->resource = resource;

        size_t width = glm::max(1.0f, floor((float)this->headers.header.width / (1 << (this->headers.header.levelcount - 1))));
        size_t mw = width;
        size_t height = glm::max(1.0f, floor((float)this->headers.header.height / (1 << (this->headers.header.levelcount - 1))));
        size_t mh = height;

        size_t usage = 0;
        // Calculate the memory usage on GPU of this texture data to figure out the permanent residency textures.
        size_t levels = 0;
        for (size_t i = 0; i < this->headers.header.levelcount; i++) {
            size_t temp = width * height * OResource::Texture::strideformat(this->headers.header.format);
            if (usage + temp > TextureManager::PERMANENTRESIDENCY) { // We exceed the permanent residency budget.
                // reverse the prior step.
                width >>= 1;
                height >>= 1;
                break;
            }
            levels++;
            usage += temp;
            width <<= 1;
            height <<= 1;
        }

        printf("Maximum mip resolution within 64KiB budget: %lux%lu.\n", width, height);
        this->residentlevels = levels;

        struct texturedesc desc = { };
        desc.type = this->headers.header.type;
        // Derive width and height from the highest resolution we can work with.
        desc.width = width;
        desc.height = height;
        desc.depth = this->headers.header.depth;
        desc.mips = levels;
        desc.layers = this->headers.header.layercount;
        desc.samples = SAMPLE_X1;
        desc.format = this->headers.header.format;
        desc.memlayout = MEMLAYOUT_OPTIMAL;
        desc.usage = USAGE_SAMPLED | USAGE_DST | USAGE_SRC;
        // Creating a texture like this will also cause a memory allocation to be performed.
        context->createtexture(&desc, &this->texture);

        struct textureviewdesc viewdesc = { };
        viewdesc.texture = this->texture;
        viewdesc.format = this->headers.header.format;
        viewdesc.layercount = this->headers.header.layercount;
        viewdesc.aspect = ASPECT_COLOUR;
        // XXX: If we load in the smallest mip, doesn't that mean we start from the end?
        viewdesc.baselayer = 0;
        viewdesc.basemiplevel = 0; // Since textures will only ever represent the number of mips they HAVE we can safely assume the base mip level will be whatever we calculated earlier.
        viewdesc.mipcount = levels; // All the levels within permanent residency.
        viewdesc.type = this->headers.header.type;
        context->createtextureview(&viewdesc, &this->textureview);

        this->bindlessid = setmanager.registertexture(this->textureview);

        this->refcount.store(1);

        struct buffer staging = { };
        // We're lazy so we can just set the buffer size to the permanent residency buffer.
        // Use coherency so we know that every memory operation has completed.
        context->createbuffer(&staging, TextureManager::PERMANENTRESIDENCY, BUFFER_TRANSFERSRC, MEMPROP_CPUVISIBLE | MEMPROP_CPUCOHERENT | MEMPROP_CPUSEQUENTIALWRITE, 0);
        struct buffermap stagingmap = { };
        context->mapbuffer(&stagingmap, staging, 0, TextureManager::PERMANENTRESIDENCY);

        size_t offset = 0;
        for (size_t i = 0; i < levels; i++) {
            OResource::Texture::loadlevel(
                this->headers, i, this->resource,
                ((uint8_t *)stagingmap.mapped[0] + offset),
                this->headers.levels[i].size
            );
            offset += this->headers.levels[i].size;
        }

        // XXX: Use transfer queue instead.
        ORenderer::Stream *stream = ORenderer::context->getimmediate();
        stream->claim();
        stream->begin();

        offset = 0;
        size_t j = levels - 1;
        for (size_t i = 0; i < levels; i++) {
            // Use pipeline barriers during the loop so when running this command buffer on the GPU the GPU can make use of mip levels *as* they're being uploaded.
            stream->barrier(
                this->texture, this->headers.header.format,
                ORenderer::LAYOUT_UNDEFINED, ORenderer::LAYOUT_TRANSFERDST, // undefined -> transfer dst
                ORenderer::PIPELINE_STAGETOP, ORenderer::PIPELINE_STAGETRANSFER, // top -> transfer
                0, ORenderer::ACCESS_TRANSFERWRITE,
                j, 1 // from this mip to only this mip (this sort of system is useful so we can let existing pipelines continue to have access to other bits of memory while we're working on this)
            ); // Barrier only applied on this mip level so that all other mips remain available

            struct ORenderer::bufferimagecopy region = { };
            region.offset = offset;
            region.rowlen = mw;
            region.imgheight = mh;
            region.imgoff = { 0, 0, 0 };
            region.imgextent = { mw, mh, this->headers.header.depth };
            region.aspect = ORenderer::ASPECT_COLOUR;
            region.mip = j;
            region.baselayer = 0;
            region.layercount = this->headers.header.layercount;
            stream->copybufferimage(region, staging, this->texture);
            stream->barrier(
                this->texture, this->headers.header.format,
                ORenderer::LAYOUT_TRANSFERDST, ORenderer::LAYOUT_SHADERRO, // Convert to shader read only
                ORenderer::PIPELINE_STAGETRANSFER, ORenderer::PIPELINE_STAGEFRAGMENT,
                ORenderer::ACCESS_TRANSFERWRITE, ORenderer::ACCESS_SHADERREAD | ORenderer::ACCESS_INPUTREAD,
                j, 1
            );
            offset += this->headers.levels[i].size;
            mw <<= 1;
            mh <<= 1;
            j--;
        }
        stream->end();
        stream->release();

        this->updateinfo.timestamp = utils_getcounter();
        this->updateinfo.resolution = levels - 1;

    }

    void Texture::meetresolution(struct updateinfo info) {
        ZoneScoped;
        OJob::ScopedMutex(&this->mutex);

        if (info.timestamp < this->updateinfo.timestamp) { // Early exit (aka. discard this work as this is an out of order resolution request)
            return;
        }

        if (info.resolution > this->updateinfo.resolution) {
            printf("Accepting request to upgrade resolution to %u.\n", info.resolution);
            // Create and map a buffer for chunked uploading.
            struct buffer staging = { };
            // Allocate staging buffer the size of the highest mip level to be loaded in.
            // XXX: Consider mapping a large staging buffer like this constantly for the purpose of transfers.
            size_t usage = 0;
            for (size_t i = this->updateinfo.resolution; i < info.resolution + 1; i++) {
                usage += this->headers.levels[i].size;
            }
            printf("final usage metric: %lu.\n", usage);
            // Allocate a staging buffer big enough to accomodate all of this.
            context->createbuffer(&staging, usage, BUFFER_TRANSFERSRC, MEMPROP_CPUVISIBLE | MEMPROP_CPUCOHERENT | MEMPROP_CPUSEQUENTIALWRITE, 0);
            struct buffermap stagingmap = { };
            context->mapbuffer(&stagingmap, staging, 0, this->headers.levels[info.resolution].size);

            // XXX: Wrong queue!
            ORenderer::Stream *stream = ORenderer::context->getimmediate();
            stream->claim();
            stream->begin();

            struct texture texture = { };
            struct texturedesc desc = { };
            desc.type = this->headers.header.type;
            // Backtrack from our version of mip levels all the way to the original width/height of this mip level.
            const size_t width = glm::max(1.0f,
                floor((float)this->headers.header.width /
                    (1 << ((0 - info.resolution) + this->headers.header.levelcount - 1))
                )
            );
            const size_t height = glm::max(1.0f,
                floor((float)this->headers.header.height /
                    (1 << ((0 - info.resolution) + this->headers.header.levelcount - 1))
                )
            );
            printf("updating to new resolution %lux%lu with level count of %u.\n", width, height, info.resolution + 1);
            desc.width = width;
            desc.height = height;
            desc.depth = this->headers.header.depth;
            desc.mips = info.resolution + 1;
            desc.layers = this->headers.header.layercount;
            desc.samples = SAMPLE_X1;
            desc.format = this->headers.header.format;
            desc.memlayout = MEMLAYOUT_OPTIMAL;
            desc.usage = USAGE_SAMPLED | USAGE_DST | USAGE_SRC;
            context->createtexture(&desc, &texture); // And allocate! Also: How do we destroy the old version without affecting existing in flight work? Perhaps dispatch a destroy on this resource after some frames of not being used?

            struct textureview textureview = { };
            struct textureviewdesc viewdesc = { };
            viewdesc.texture = texture;
            viewdesc.format = this->headers.header.format;
            viewdesc.layercount = this->headers.header.layercount;
            viewdesc.aspect = ASPECT_COLOUR;
            viewdesc.baselayer = 0;
            viewdesc.basemiplevel = 0;
            viewdesc.mipcount = info.resolution + 1;
            viewdesc.type = this->headers.header.type;
            context->createtextureview(&viewdesc, &textureview); // This will replace our old texture view in due course.

            // Copy over old data to new data. On-GPU memory operation, doesn't incur bus bandwidth costs!
            for (size_t i = 0; i < this->updateinfo.resolution + 1; i++) { // For all existing resolutions.
                struct imagecopy region = { };
                size_t level = ((0 - i) + info.resolution);
                size_t oldlevel = ((0 - i) + this->updateinfo.resolution);
                size_t mw = glm::max(1.0f, floor((float)width / (1 << level)));
                size_t mh = glm::max(1.0f, floor((float)height / (1 << level)));

                printf("copying image from level %lu to new texture at res %lux%lu.\n", i, mw, mh);

                stream->barrier(
                    texture, this->headers.header.format,
                    ORenderer::LAYOUT_UNDEFINED, ORenderer::LAYOUT_TRANSFERDST,
                    ORenderer::PIPELINE_STAGETOP, ORenderer::PIPELINE_STAGETRANSFER,
                    0, ORenderer::ACCESS_TRANSFERWRITE,
                    level, 1
                );
                stream->barrier(
                    this->texture, this->headers.header.format,
                    ORenderer::LAYOUT_SHADERRO, ORenderer::LAYOUT_TRANSFERSRC,
                    ORenderer::PIPELINE_STAGEFRAGMENT, ORenderer::PIPELINE_STAGETRANSFER,
                    ORenderer::ACCESS_SHADERREAD | ORenderer::ACCESS_INPUTREAD,
                    ORenderer::ACCESS_TRANSFERREAD,
                    oldlevel, 1
                );

                region.srcmip = oldlevel;
                region.dstmip = level;
                region.srcbaselayer = 0;
                region.dstbaselayer = 0;
                region.srclayercount = this->headers.header.layercount;
                region.dstlayercount = this->headers.header.layercount;
                region.srcaspect = ASPECT_COLOUR;
                region.dstaspect = ASPECT_COLOUR;
                region.srcoff = { 0, 0, 0 };
                region.dstoff = { 0, 0, 0 };
                region.extent = { mw, mh, this->headers.header.depth };
                stream->copyimage(region, this->texture, texture, LAYOUT_TRANSFERSRC, LAYOUT_TRANSFERDST);
                stream->barrier(
                    this->texture, this->headers.header.format,
                    ORenderer::LAYOUT_TRANSFERSRC, ORenderer::LAYOUT_SHADERRO,
                    ORenderer::PIPELINE_STAGETRANSFER, ORenderer::PIPELINE_STAGEFRAGMENT,
                    ORenderer::ACCESS_TRANSFERREAD,
                    ORenderer::ACCESS_SHADERREAD | ORenderer::ACCESS_INPUTREAD,
                    oldlevel, 1
                );
                stream->barrier(
                    texture, this->headers.header.format,
                    ORenderer::LAYOUT_TRANSFERDST, ORenderer::LAYOUT_SHADERRO,
                    ORenderer::PIPELINE_STAGETRANSFER, ORenderer::PIPELINE_STAGEFRAGMENT,
                    ORenderer::ACCESS_TRANSFERWRITE,
                    ORenderer::ACCESS_SHADERREAD | ORenderer::ACCESS_INPUTREAD,
                    level, 1
                );
            }

            // XXX: Somehow, between these points something takes ages!

            uint32_t iterator = this->updateinfo.resolution + 1;
            size_t dataoffset = 0;
            do {
                size_t reading = this->headers.levels[iterator].size;
                OResource::Texture::loadlevel(this->headers, iterator, this->resource, ((uint8_t *)stagingmap.mapped[0] + dataoffset), reading, 0); // Read in full chunk (or up until the mip level size)
                size_t level = ((0 - iterator) + info.resolution);
                size_t mw = glm::max(1.0f, floor((float)width / (1 << level)));
                size_t mh = glm::max(1.0f, floor((float)height / (1 << level)));
                printf("working on level %lu with size %lux%lu.\n", level, mw, mh);

                stream->barrier(
                    texture, this->headers.header.format,
                    ORenderer::LAYOUT_UNDEFINED, ORenderer::LAYOUT_TRANSFERDST,
                    ORenderer::PIPELINE_STAGETOP, ORenderer::PIPELINE_STAGETRANSFER,
                    0, ORenderer::ACCESS_TRANSFERWRITE,
                    level, 1
                );

                struct ORenderer::bufferimagecopy region = { };
                region.offset = dataoffset;
                dataoffset += reading; // Increment data offset so we know how much we've written so far and how much is left.
                region.rowlen = mw;
                region.imgheight = mh;
                region.imgoff = { 0, 0, 0 };
                region.imgextent = { mw, mh, this->headers.header.depth };
                region.aspect = ORenderer::ASPECT_COLOUR;
                region.mip = level;
                region.baselayer = 0;
                region.layercount = this->headers.header.layercount;
                stream->copybufferimage(region, staging, texture);

                // if (dataoffset == this->headers.levels[iterator].size) { // we're done!
                    stream->barrier(
                        texture, this->headers.header.format,
                        ORenderer::LAYOUT_TRANSFERDST, ORenderer::LAYOUT_SHADERRO,
                        ORenderer::PIPELINE_STAGETRANSFER, ORenderer::PIPELINE_STAGEFRAGMENT,
                        ORenderer::ACCESS_TRANSFERWRITE, ORenderer::ACCESS_SHADERREAD | ORenderer::ACCESS_INPUTREAD,
                        level, 1
                    );
                    // XXX: Transition the layout to its final Shader RO form and move to the next bit of work.
                    iterator++;
                // }
            } while (iterator < info.resolution + 1); // Use a do-while here so it runs at least once, we do requested + 1 here because otherwise it is NOT going to do all the work required of it and will early exit before the current working resolution is equal to the requested.

            stream->end();
            stream->release();

            // update everything!
            setmanager.updatetexture(this->bindlessid, textureview);
            // XXX: Destroy old one!
            // Just dispatch a "deferred resource destroy" low priority job that'll loop and yield waiting for the frames in flight deadline to expire.
            this->texture = texture;
            this->textureview = textureview;

            // We're done. Get rid of this allocation.
            context->unmapbuffer(stagingmap);
            context->destroybuffer(&staging);
        } else if (info.resolution < this->updateinfo.resolution && info.resolution >= residentlevels - 1) {
            printf("Accepting request to downgrade resolution to %u.\n", info.resolution);

            ORenderer::Stream *stream = ORenderer::context->getimmediate();
            stream->claim();
            stream->begin();

            struct texture texture = { };
            struct texturedesc desc = { };
            desc.type = this->headers.header.type;
            const size_t width = glm::max(1.0f,
                floor((float)this->headers.header.width /
                    (1 << ((0 - info.resolution) + this->headers.header.levelcount - 1))
                )
            );
            const size_t height = glm::max(1.0f,
                floor((float)this->headers.header.height /
                    (1 << ((0 - info.resolution) + this->headers.header.levelcount - 1))
                )
            );

            desc.width = width;
            desc.height = height;
            desc.depth = this->headers.header.depth;
            desc.mips = info.resolution + 1;
            desc.layers = this->headers.header.layercount;
            desc.samples = SAMPLE_X1;
            desc.format = this->headers.header.format;
            desc.memlayout = MEMLAYOUT_OPTIMAL;
            desc.usage = USAGE_SAMPLED | USAGE_DST | USAGE_SRC;
            context->createtexture(&desc, &texture);

            struct textureview textureview = { };
            struct textureviewdesc viewdesc = { };
            viewdesc.texture = texture;
            viewdesc.format = this->headers.header.format;
            viewdesc.layercount = this->headers.header.layercount;
            viewdesc.aspect = ASPECT_COLOUR;
            viewdesc.baselayer = 0;
            viewdesc.basemiplevel = 0;
            viewdesc.mipcount = info.resolution + 1;
            viewdesc.type = this->headers.header.type;
            context->createtextureview(&viewdesc, &textureview);

            // Copy over old data to new data. On-GPU memory operation, doesn't incur bus bandwidth costs!
            // Do these calculations out here and just work on them to get the required size.
            size_t mw = glm::max(1.0f, floor((float)width / (1 << info.resolution)));
            size_t mh = glm::max(1.0f, floor((float)height / (1 << info.resolution)));
            for (size_t i = 0; i < info.resolution + 1; i++) { // For all existing resolutions.
                struct imagecopy region = { };
                size_t level = ((0 - i) + info.resolution);
                size_t oldlevel = ((0 - i) + this->updateinfo.resolution);

                printf("copying image from level %lu to new texture at res %lux%lu.\n", i, mw, mh);

                stream->barrier(
                    texture, this->headers.header.format,
                    ORenderer::LAYOUT_UNDEFINED, ORenderer::LAYOUT_TRANSFERDST,
                    ORenderer::PIPELINE_STAGETOP, ORenderer::PIPELINE_STAGETRANSFER,
                    0, ORenderer::ACCESS_TRANSFERWRITE,
                    level, 1
                );
                stream->barrier(
                    this->texture, this->headers.header.format,
                    ORenderer::LAYOUT_SHADERRO, ORenderer::LAYOUT_TRANSFERSRC,
                    ORenderer::PIPELINE_STAGEFRAGMENT, ORenderer::PIPELINE_STAGETRANSFER,
                    ORenderer::ACCESS_SHADERREAD | ORenderer::ACCESS_INPUTREAD,
                    ORenderer::ACCESS_TRANSFERREAD,
                    oldlevel, 1
                );

                region.srcmip = oldlevel;
                region.dstmip = level;
                region.srcbaselayer = 0;
                region.dstbaselayer = 0;
                region.srclayercount = this->headers.header.layercount;
                region.dstlayercount = this->headers.header.layercount;
                region.srcaspect = ASPECT_COLOUR;
                region.dstaspect = ASPECT_COLOUR;
                region.srcoff = { 0, 0, 0 };
                region.dstoff = { 0, 0, 0 };
                region.extent = { mw, mh, this->headers.header.depth };
                stream->copyimage(region, this->texture, texture, LAYOUT_TRANSFERSRC, LAYOUT_TRANSFERDST);
                stream->barrier(
                    this->texture, this->headers.header.format,
                    ORenderer::LAYOUT_TRANSFERSRC, ORenderer::LAYOUT_SHADERRO,
                    ORenderer::PIPELINE_STAGETRANSFER, ORenderer::PIPELINE_STAGEFRAGMENT,
                    ORenderer::ACCESS_TRANSFERREAD,
                    ORenderer::ACCESS_SHADERREAD | ORenderer::ACCESS_INPUTREAD,
                    oldlevel, 1
                );
                stream->barrier(
                    texture, this->headers.header.format,
                    ORenderer::LAYOUT_TRANSFERDST, ORenderer::LAYOUT_SHADERRO,
                    ORenderer::PIPELINE_STAGETRANSFER, ORenderer::PIPELINE_STAGEFRAGMENT,
                    ORenderer::ACCESS_TRANSFERWRITE,
                    ORenderer::ACCESS_SHADERREAD | ORenderer::ACCESS_INPUTREAD,
                    level, 1
                );

                // Simple power of two to get the width and height for the next level. Significantly faster than calculating the width and height every single time by deriving it from the mip level.
                mw <<= 1;
                mh <<= 1;
            }
            stream->end();
            stream->release();
            setmanager.updatetexture(this->bindlessid, textureview);
            this->texture = texture;
            this->textureview = textureview;
        } else {
            return;
        }

        this->updateinfo = info; // update info to represent the request.
    }

    OUtils::Handle<OResource::Resource> TextureManager::create(OUtils::Handle<OResource::Resource> resource) {
        resource->claim();

        size_t len = strnlen(resource->path, 128);
        char *newpath = (char *)malloc(len + 3);
        ASSERT(newpath != NULL, "Failed to allocate memory for VFS path.\n");
        snprintf(newpath, len + 2, "%s*", resource->path);
        newpath[len + 2] = '\0';
        printf("%s.\n", newpath);

        OUtils::Handle<OResource::Resource> ret = RESOURCE_INVALIDHANDLE;
        if ((ret = OResource::manager.get(newpath)) == RESOURCE_INVALIDHANDLE) {
            ret = OResource::manager.create(newpath, new Texture(resource));
            ret->as<Texture>()->self = ret;
            resource->release();
            return ret;
        } else {
            free(newpath);
            resource->release();
            return ret; // Texture already exists, silently continue on as always.
        }

        resource->release();
        return RESOURCE_INVALIDHANDLE;
    }

    OUtils::Handle<OResource::Resource> TextureManager::create(const char *path) {
        return this->create(OResource::manager.get(path));
    }

}
