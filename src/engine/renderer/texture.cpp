#include <engine/renderer/bindless.hpp>
#include <engine/renderer/texture.hpp>
#include <engine/resources/texture.hpp>
#include <engine/utils/print.hpp>

namespace ORenderer {

    TextureManager texturemanager;

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

        OUtils::print("Maximum resident resolution within 64KiB budget: %lux%lu.\n", width, height);

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
        context->setdebugname(this->texture, "Streamable Texture");

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
        ORenderer::Stream *stream = ORenderer::context->requeststream(ORenderer::STREAM_IMMEDIATE);
        stream->claim();
        stream->begin();

        // Use pipeline barriers during the loop so when running this command buffer on the GPU the GPU can make use of mip levels *as* they're being uploaded.
        stream->barrier(
            this->texture, this->headers.header.format,
            ORenderer::LAYOUT_UNDEFINED, ORenderer::LAYOUT_TRANSFERDST, // undefined -> transfer dst
            ORenderer::PIPELINE_STAGETOP, ORenderer::PIPELINE_STAGETRANSFER, // top -> transfer
            0, ORenderer::ACCESS_TRANSFERWRITE,
            ORenderer::STREAM_IMMEDIATE, ORenderer::STREAM_IMMEDIATE//,
            // j, 1 // from this mip to only this mip (this sort of system is useful so we can let existing pipelines continue to have access to other bits of memory while we're working on this)
        ); // Barrier only applied on this mip level so that all other mips remain available

        offset = 0;
        size_t j = levels - 1;
        for (size_t i = 0; i < levels; i++) {
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
            offset += this->headers.levels[i].size;
            mw <<= 1;
            mh <<= 1;
            j--;
        }

        stream->barrier(
            this->texture, this->headers.header.format,
            ORenderer::LAYOUT_TRANSFERDST, ORenderer::LAYOUT_SHADERRO, // Convert to shader read only
            ORenderer::PIPELINE_STAGETRANSFER, ORenderer::PIPELINE_STAGEFRAGMENT,
            ORenderer::ACCESS_TRANSFERWRITE, ORenderer::ACCESS_SHADERREAD | ORenderer::ACCESS_INPUTREAD,
            ORenderer::STREAM_IMMEDIATE, ORenderer::STREAM_IMMEDIATE//,
            // j, 1
        );
        stream->end();
        stream->release();
        context->submitstream(stream, true);

        this->updateinfo.timestamp = utils_getcounter();
        this->updateinfo.resolution = levels - 1;
        this->stagingbuffers.reserve(levels); // reserve as many levels as there are levels in the texture.
    }

    // Deferred destruction work, passed to job.
    struct work {
        size_t deadline;
        struct buffer staging;
        struct texture texture;
        struct textureview textureview;
        bool upgrade;

        ORenderer::Stream *stream; // transfer stream
        ORenderer::Stream *tomain; // transfer->graphics stream
        ORenderer::Stream *totransfer; // graphics->transfer stream
    };

    static void deferreddestroy(OJob::Job *job) {
        struct work *work = (struct work *)job->param;
        while (context->frameid < work->deadline) { // So long as we haven't met the deadline, keep working.
            OJob::yield(OJob::currentfibre, OJob::Job::STATUS_YIELD); // This fucking sucks, actually (thinking cap on)
        }

        if (work->upgrade) {
            // Destroy all deferred work.
            context->destroybuffer(&work->staging);
            context->destroytexture(&work->texture);
            context->destroytextureview(&work->textureview);
            // Free the stream, this should actually be done differently!
            // Could probably just use a fence and wait on it. I should imagine it'd be easier to get the fence status and act like a critical section rather than a spinlock, because that'd take too long.
            // Problems could arise where we already go ahead and destroy that work before we update, i mean, if we deployed a number of update jobs within the span of a few frames that *could* happen, it's unlikely, but not impossible. Shouldn't ever be a concern though :D (i'll implement some checks just in case so i know it's what is causing a problem should it ever arise)
            context->freestream(work->stream);
            context->freestream(work->tomain);
            context->freestream(work->totransfer);

            // XXX: Handle this allocation better.
            free(work);
        } else {
            context->destroytexture(&work->texture);
            context->destroytextureview(&work->textureview);
            context->freestream(work->stream);
            free(work);
        }
    }

    static void updateworker(OJob::Job *job) {
        ZoneScopedN("Asynchronous Level Loader");

        struct Texture::updatework *work = (struct Texture::updatework *)job->param;
        Texture *texture = work->texture; // What texture are we working on?
        struct Texture::updateinfo info = work->info; // Information about the upgrade request.
        printf("update info %u %lu.\n", info.resolution, info.timestamp);

        texture->mutex.lock(); // Gain a lock on the texture.

        if (texture->stagingbuffers[info.resolution].handle != RENDERER_INVALIDHANDLE) {
            // we already have a damn buffer!
            texture->mutex.unlock();
            free(work);
            return; // nothing to do.
        }

        if (info.resolution > texture->updateinfo.resolution) { // Upgrade work.
            struct buffer staging = { }; // Initialise a staging buffer.

            printf("upgrading to %u async.\n", info.resolution);
            size_t levelsize = texture->headers.levels[info.resolution].size;
            context->createbuffer(&staging, levelsize, BUFFER_TRANSFERSRC, MEMPROP_CPUVISIBLE | MEMPROP_CPUCOHERENT | MEMPROP_CPUSEQUENTIALWRITE, 0); // Initialise a buffer for use as a transfer source (ie. from CPU to GPU) with optimisation properties like the lack of random access needed.
            struct buffermap stagingmap = { };
            context->mapbuffer(&stagingmap, staging, 0, texture->headers.levels[info.resolution].size); // Create a CPU-writable memory map for this data.
            texture->stagingbuffers[info.resolution] = staging; // pass the reference of the staging buffer over to here. XXX: Could this potentially be lost and memory leaked? Make sure we have a way to get rid of it in every circumstance.
            texture->mutex.unlock();

            OResource::Texture::loadlevel(texture->headers, info.resolution, texture->resource, ((uint8_t *)stagingmap.mapped[0]), levelsize, 0); // Load the level from the disk. This is done within the worker thread to allow parallel operation.
            texturemanager.fence.wait();
        }
        free(work);
    }

    void Texture::meetresolution(struct updateinfo info) {
        ZoneScoped;
        OJob::ScopedMutex(&this->mutex);

        if (info.timestamp < this->updateinfo.timestamp) { // Early exit (aka. discard this work as this is an out of order resolution request)
            return;
        }

        if (info.resolution > this->updateinfo.resolution) {
//             OUtils::print("Accepting request to upgrade resolution to %u.\n", info.resolution);
//             // Create and map a buffer for chunked uploading.
//             struct buffer staging = { };
//             // Allocate staging buffer the size of the highest mip level to be loaded in.
//             // XXX: Consider mapping a large staging buffer like this constantly for the purpose of transfers.
//     size_t usage = 0;
//             for (size_t i = this->updateinfo.resolution; i < info.resolution + 1; i++) {
//                 usage += this->headers.levels[i].size;
//             }
//             // Allocate a staging buffer big enough to accomodate all of this.
//             context->createbuffer(&staging, usage, BUFFER_TRANSFERSRC, MEMPROP_CPUVISIBLE | MEMPROP_CPUCOHERENT | MEMPROP_CPUSEQUENTIALWRITE, 0);
//             struct buffermap stagingmap = { };
// // XXX: -> This only maps the size for a single level.
//             context->mapbuffer(&stagingmap, staging, 0, this->headers.levels[info.resolution].size);
//
//
//             // XXX: Ownership transfer! resources in Vulkan have to be explicitly shared between queues!
//             // The summation of my understanding is as follows:
//             // Initial queue: Relinquish ownership of queue to different family and submit immediately, following a semaphore on its completion (signal semaphore) we submit this work here which acquires the image, before finally submitting another main queue ownership change by acquiring it immediately again by waiting on the semaphore of this work's conclusion. Lots of work to be done!!!
//             // Streams should be allowed to depend on one another! Register a semaphore pool (I like pools) and have them sort out stream inter-dependency on dispatch. Do like stream->dependency(info) so that it'll work with arbitrary streams as well as the per-frame ones, that way all the streams can know to wait on or signal one another. This'll aid in allowing the dispatch of a stream on one queue that hands off resources to another which'll then handle more stuff. All as planned! got to order things somehow of course.
//
//             ORenderer::Stream *totransfer = ORenderer::context->requeststream(ORenderer::STREAM_IMMEDIATE);
//
//             ORenderer::Stream *tomain = ORenderer::context->requeststream(ORenderer::STREAM_IMMEDIATE);
//
//             ORenderer::Stream *stream = ORenderer::context->requeststream(ORenderer::STREAM_TRANSFER);
//             stream->claim();
//             tomain->claim();
//             totransfer->claim();
//
//             // Wait on stream
//             tomain->adddependency(stream);
//             // Wait on transfer.
// stream->adddependency(totransfer); // We want to wait on the ownership transfer to the transfer queue, then signal the ownership transfer to the main queue when we're done.
//
//             totransfer->begin();
// totransfer->marker("begin ownership transfer from main to transfer");
//             // Ownership release.
//             totransfer->barrier(this->texture, this->headers.header.format, ORenderer::LAYOUT_SHADERRO, ORenderer::LAYOUT_TRANSFERSRC, ORenderer::PIPELINE_STAGEFRAGMENT, ORenderer::PIPELINE_STAGETRANSFER, ORenderer::ACCESS_SHADERREAD | ORenderer::ACCESS_INPUTREAD, ORenderer::ACCESS_TRANSFERREAD, ORenderer::STREAM_FRAME, ORenderer::STREAM_FRAME);
//
//             totransfer->barrier(this->texture, this->headers.header.format, ORenderer::LAYOUT_TRANSFERSRC, ORenderer::LAYOUT_TRANSFERSRC, ORenderer::PIPELINE_STAGETRANSFER, ORenderer::PIPELINE_STAGETRANSFER, ORenderer::ACCESS_TRANSFERREAD, ORenderer::ACCESS_TRANSFERREAD, ORenderer::STREAM_FRAME, ORenderer::STREAM_TRANSFER);
//             totransfer->marker("work done for main to transfer queue");
//             totransfer->end();
//             totransfer->release();
//
// stream->begin();
//             stream->marker("acquire ownership");
//             // Ownership acquire.
//             stream->barrier(this->texture, this->headers.header.format, ORenderer::LAYOUT_TRANSFERSRC, ORenderer::LAYOUT_TRANSFERSRC, ORenderer::PIPELINE_STAGETRANSFER, ORenderer::PIPELINE_STAGETRANSFER, 0, ORenderer::ACCESS_TRANSFERREAD, ORenderer::STREAM_FRAME, ORenderer::STREAM_TRANSFER);
//
//             struct texture texture = { };
//             struct texturedesc desc = { };
//             desc.type = this->headers.header.type;
//             // Backtrack from our version of mip levels all the way to the original width/height of this mip level.
//             const size_t width = glm::max(1.0f,
//                 floor((float)this->headers.header.width /
//                     (1 << ((0 - info.resolution) + this->headers.header.levelcount - 1))
//                 )
//             );
//             const size_t height = glm::max(1.0f,
//                 floor((float)this->headers.header.height /
//                     (1 << ((0 - info.resolution) + this->headers.header.levelcount - 1))
//                 )
//             );
//             desc.width = width;
//             desc.height = height;
//             desc.depth = this->headers.header.depth;
//             desc.mips = info.resolution + 1;
//             desc.layers = this->headers.header.layercount;
//             desc.samples = SAMPLE_X1;
//             desc.format = this->headers.header.format;
//             desc.memlayout = MEMLAYOUT_OPTIMAL;
//             desc.usage = USAGE_SAMPLED | USAGE_DST | USAGE_SRC;
//             context->createtexture(&desc, &texture); // And allocate! Also: How do we destroy the old version without affecting existing in flight work? Perhaps dispatch a destroy on this resource after some frames of not being used?
//             char name[64];
//             snprintf(name, 64, "Streamed %lux%lu", width, height);
//             context->setdebugname(texture, name);
//
//             struct textureview textureview = { };
//             struct textureviewdesc viewdesc = { };
//             viewdesc.texture = texture;
//             viewdesc.format = this->headers.header.format;
//             viewdesc.layercount = this->headers.header.layercount;
//             viewdesc.aspect = ASPECT_COLOUR;
//             viewdesc.baselayer = 0;
//             viewdesc.basemiplevel = 0;
//             viewdesc.mipcount = info.resolution + 1;
//             viewdesc.type = this->headers.header.type;
//             context->createtextureview(&viewdesc, &textureview); // This will replace our old texture view in due course.
//
//             stream->marker("newly created texture transfer to layout");
//             // Transition to transfer layout.
//             stream->barrier(
//                 texture, this->headers.header.format,
//                 ORenderer::LAYOUT_UNDEFINED, ORenderer::LAYOUT_TRANSFERDST,
//                 ORenderer::PIPELINE_STAGETOP, ORenderer::PIPELINE_STAGETRANSFER,
//                 0, ORenderer::ACCESS_TRANSFERWRITE,
//                 ORenderer::STREAM_TRANSFER, ORenderer::STREAM_TRANSFER
//             );
//             stream->marker("data copy");
//
//             // Copy over old data to new data. On-GPU memory operation, doesn't incur bus bandwidth costs!
//             for (size_t i = 0; i < this->updateinfo.resolution + 1; i++) { // For all existing resolutions.
//                 struct imagecopy region = { };
//                 size_t level = ((0 - i) + info.resolution);
//                 size_t oldlevel = ((0 - i) + this->updateinfo.resolution);
//                 size_t mw = glm::max(1.0f, floor((float)width / (1 << level)));
//                 size_t mh = glm::max(1.0f, floor((float)height / (1 << level)));
//
//                 region.srcmip = oldlevel;
//                 region.dstmip = level;
//                 region.srcbaselayer = 0;
//                 region.dstbaselayer = 0;
//                 region.srclayercount = this->headers.header.layercount;
//                 region.dstlayercount = this->headers.header.layercount;
//                 region.srcaspect = ASPECT_COLOUR;
//                 region.dstaspect = ASPECT_COLOUR;
//                 region.srcoff = { 0, 0, 0 };
//                 region.dstoff = { 0, 0, 0 };
//                 region.extent = { mw, mh, this->headers.header.depth };
//                 stream->copyimage(region, this->texture, texture, LAYOUT_TRANSFERSRC, LAYOUT_TRANSFERDST);
//             }
//
//             stream->marker("finished data copy");
//
//             // XXX: Somehow, between these points something takes ages!
//
//             uint32_t iterator = this->updateinfo.resolution + 1;
//             size_t dataoffset = 0;
//             do {
//                 size_t reading = this->headers.levels[iterator].size;
//                 OResource::Texture::loadlevel(this->headers, iterator, this->resource, ((uint8_t *)stagingmap.mapped[0] + dataoffset), reading, 0); // Read in full chunk (or up until the mip level size)
//                 size_t level = ((0 - iterator) + info.resolution);
//                 size_t mw = glm::max(1.0f, floor((float)width / (1 << level)));
//                 size_t mh = glm::max(1.0f, floor((float)height / (1 << level)));
//
//                 struct ORenderer::bufferimagecopy region = { };
//                 region.offset = dataoffset;
//                 dataoffset += reading; // Increment data offset so we know how much we've written so far and how much is left.
//                 region.rowlen = mw;
//                 region.imgheight = mh;
//                 region.imgoff = { 0, 0, 0 };
//                 region.imgextent = { mw, mh, this->headers.header.depth };
//                 region.aspect = ORenderer::ASPECT_COLOUR;
//                 region.mip = level;
//                 region.baselayer = 0;
//                 region.layercount = this->headers.header.layercount;
//                 stream->copybufferimage(region, staging, texture);
//
//                 // if (dataoffset == this->headers.levels[iterator].size) { // we're done!
//                     // XXX: Transition the layout to its final Shader RO form and move to the next bit of work.
//                     iterator++;
//                 // }
//             } while (iterator < info.resolution + 1); // Use a do-while here so it runs at least once, we do requested + 1 here because otherwise it is NOT going to do all the work required of it and will early exit before the current working resolution is equal to the requested.
//
//             stream->marker("release ownership of both");
//             // Release texture back to main queue.
//             stream->barrier(
//                 this->texture, this->headers.header.format,
//                 ORenderer::LAYOUT_TRANSFERSRC, ORenderer::LAYOUT_TRANSFERSRC,
//                 ORenderer::PIPELINE_STAGETRANSFER, ORenderer::PIPELINE_STAGETRANSFER,
//                 ORenderer::ACCESS_TRANSFERREAD, 0,
//                 ORenderer::STREAM_TRANSFER, ORenderer::STREAM_FRAME
//             );
//             // Release texture to main queue.
//             stream->barrier(
//                 texture, this->headers.header.format,
//                 ORenderer::LAYOUT_TRANSFERDST, ORenderer::LAYOUT_TRANSFERDST,
//                 ORenderer::PIPELINE_STAGETRANSFER, ORenderer::PIPELINE_STAGETRANSFER,
//                 ORenderer::ACCESS_TRANSFERWRITE, 0,
//                 ORenderer::STREAM_TRANSFER, ORenderer::STREAM_FRAME
//             );
//             stream->end(); // Something better should be done about this, but it works for now. Ideally this'd be a critical section style lock, so we'll wait a little bit blocking before jumping over to a yield() loop so we don't need to do more. Waiting here is required as we must ensure all barrier work has been completed before changing what the texture points to CPU side to prevent out-of-order memory access.
//             stream->release();
//
//             tomain->begin();
//             tomain->marker("acquire on main queue");
//             // Reacquire the texture on main queue so typical usage can continue.
//             tomain->barrier(
//                 this->texture, this->headers.header.format,
//                 ORenderer::LAYOUT_TRANSFERSRC, ORenderer::LAYOUT_TRANSFERSRC,
//                 ORenderer::PIPELINE_STAGETRANSFER, ORenderer::PIPELINE_STAGETRANSFER,
//                 0, ORenderer::ACCESS_TRANSFERREAD,
//                 ORenderer::STREAM_TRANSFER, ORenderer::STREAM_FRAME
//             );
//             // Acquire the new texture and transition its layout in the process.
//             tomain->barrier(
//                 texture, this->headers.header.format,
//                 ORenderer::LAYOUT_TRANSFERDST, ORenderer::LAYOUT_TRANSFERDST,
//                 ORenderer::PIPELINE_STAGETRANSFER, ORenderer::PIPELINE_STAGETRANSFER,
//                 0, ORenderer::ACCESS_TRANSFERWRITE,
//                 ORenderer::STREAM_TRANSFER, ORenderer::STREAM_FRAME
//             );
//
//             tomain->barrier(
//                 this->texture, this->headers.header.format,
//                 ORenderer::LAYOUT_TRANSFERSRC, ORenderer::LAYOUT_SHADERRO,
//                 ORenderer::PIPELINE_STAGETRANSFER, ORenderer::PIPELINE_STAGEFRAGMENT,
//                 ACCESS_TRANSFERREAD, ORenderer::ACCESS_SHADERREAD | ORenderer::ACCESS_INPUTREAD,
//                 ORenderer::STREAM_FRAME, ORenderer::STREAM_FRAME
//             );
//             // Final layout change.
//             tomain->barrier(
//                 texture, this->headers.header.format,
//                 ORenderer::LAYOUT_TRANSFERDST, ORenderer::LAYOUT_SHADERRO, // this never happens?
//                 ORenderer::PIPELINE_STAGETRANSFER, ORenderer::PIPELINE_STAGEFRAGMENT,
//                 ACCESS_TRANSFERWRITE, ORenderer::ACCESS_SHADERREAD | ORenderer::ACCESS_INPUTREAD,
//                 ORenderer::STREAM_FRAME, ORenderer::STREAM_FRAME
//             );
//             tomain->marker("completed all work!!!");
//             tomain->end();
//             tomain->release();
//             // XXX: Implement locks that respect locking already on the current thread. What if something happens to the stream between now and submission?
//             context->submitstream(totransfer); // pass access to transfer queue
//             context->submitstream(stream); // copy data to new texture before passing access back to graphics queue
//             context->submitstream(tomain); // Wait on this one, as its when the work is finished.
//
//             // update everything!
//             // Because we update it here on the CPU before we actually finish doing all the barrier work, it ends up using a version that doesn't yet have any barriers set up.
//             setmanager.updatetexture(this->bindlessid, textureview);
//
//             struct work *destroy = (struct work *)malloc(sizeof(struct work)); // XXX: Guaranteed to be freed once no longer used.
//             ASSERT(destroy != NULL, "Failed to allocate struct for deferred destroy work.\n");
//             destroy->deadline = context->frameid + TextureManager::EVICTDEADLINE; // Destroy when we can guarantee this is no longer being used.
//             destroy->texture = this->texture;
//             destroy->stream = stream;
//             destroy->tomain = tomain;
//             destroy->totransfer = totransfer;
//             destroy->textureview = this->textureview;
//             destroy->upgrade = true; // this is from a resolution upgrade.
//             destroy->staging = staging; // Even though it was created in stack, it's just a handle, so we can pass it off here.
//             OJob::Job *deferred = new OJob::Job(deferreddestroy, (uintptr_t)destroy);
//             deferred->priority = OJob::Job::PRIORITY_NORMAL; // XXX: Implement LOW!
//             OJob::kickjob(deferred);
//
//             texturemanager.operationsmutex.lock();
//             texturemanager.activeoperations.push_back(tomain); // push the last (in order) stream to the texture manager's list, so that we can later use this information.
//             texturemanager.operationsmutex.unlock();
//
//             // XXX: Destroy old one!
//             // Just dispatch a "deferred resource destroy" low priority job that'll loop and yield waiting for the frames in flight deadline to expire (by which time, we can reasonably assume there are no frames using this resource).
//             this->texture = texture;
//             this->textureview = textureview;
//
//             // We're done. Get rid of this allocation.
//             context->unmapbuffer(stagingmap);

            // context->destroybuffer(&staging);

            OUtils::print("scheduling resolution increase.\n");
            struct updatework *work = (struct updatework *)malloc(sizeof(struct updatework));
            ASSERT(work != NULL, "Failed to allocate memory for update work.\n");
            memset(work, 0, sizeof(struct updatework));
            work->texture = this;
            work->info = info;
            OJob::Job *job = new OJob::Job(updateworker, (uintptr_t)work);
            job->priority = OJob::Job::PRIORITY_NORMAL; // XXX: Low
            OJob::kickjob(job);
        } else if (info.resolution < this->updateinfo.resolution && info.resolution >= residentlevels - 1) {
            OUtils::print("Accepting request to downgrade resolution to %u.\n", info.resolution);

            ORenderer::Stream *stream = ORenderer::context->requeststream(ORenderer::STREAM_IMMEDIATE);
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

                OUtils::print("Copying texture from level %lu to new texture at res %lux%lu.\n", i, mw, mh);

                stream->barrier(
                    texture, this->headers.header.format,
                    ORenderer::LAYOUT_UNDEFINED, ORenderer::LAYOUT_TRANSFERDST,
                    ORenderer::PIPELINE_STAGETOP, ORenderer::PIPELINE_STAGETRANSFER,
                    0, ORenderer::ACCESS_TRANSFERWRITE,
                    ORenderer::STREAM_IMMEDIATE, ORenderer::STREAM_IMMEDIATE,
                    level, 1
                );
                stream->barrier(
                    this->texture, this->headers.header.format,
                    ORenderer::LAYOUT_SHADERRO, ORenderer::LAYOUT_TRANSFERSRC,
                    ORenderer::PIPELINE_STAGEFRAGMENT, ORenderer::PIPELINE_STAGETRANSFER,
                    ORenderer::ACCESS_SHADERREAD | ORenderer::ACCESS_INPUTREAD,
                    ORenderer::ACCESS_TRANSFERREAD,
                    ORenderer::STREAM_IMMEDIATE, ORenderer::STREAM_IMMEDIATE,
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
                    ORenderer::STREAM_IMMEDIATE, ORenderer::STREAM_IMMEDIATE,
                    oldlevel, 1
                );
                stream->barrier(
                    texture, this->headers.header.format,
                    ORenderer::LAYOUT_TRANSFERDST, ORenderer::LAYOUT_SHADERRO,
                    ORenderer::PIPELINE_STAGETRANSFER, ORenderer::PIPELINE_STAGEFRAGMENT,
                    ORenderer::ACCESS_TRANSFERWRITE,
                    ORenderer::ACCESS_SHADERREAD | ORenderer::ACCESS_INPUTREAD,
                    ORenderer::STREAM_IMMEDIATE, ORenderer::STREAM_IMMEDIATE,
                    level, 1
                );

                // Simple power of two to get the width and height for the next level. Significantly faster than calculating the width and height every single time by deriving it from the mip level.
                mw <<= 1;
                mh <<= 1;
            }
            stream->end();
            stream->release();
            context->submitstream(stream, true);

            setmanager.updatetexture(this->bindlessid, textureview);
            struct work *destroy = (struct work *)malloc(sizeof(struct work));
            ASSERT(destroy != NULL, "Failed to allocate struct for deferred destroy work.\n");
            destroy->deadline = context->frameid + TextureManager::EVICTDEADLINE;
            destroy->texture = this->texture;
            destroy->stream = stream;
            destroy->textureview = this->textureview;
            destroy->upgrade = false; // this is from a resolution downgrade.
            OJob::Job *deferred = new OJob::Job(deferreddestroy, (uintptr_t)destroy);
            deferred->priority = OJob::Job::PRIORITY_NORMAL; // XXX: Implement LOW!
            OJob::kickjob(deferred);

            texturemanager.operationsmutex.lock();
            texturemanager.activeoperations.push_back(stream);
            texturemanager.operationsmutex.unlock();

            this->texture = texture;
            this->textureview = textureview;
        } else {
            return;
        }

        this->updateinfo.resolution = info.resolution; // update info to represent the request.
        this->updateinfo.timestamp = utils_getcounter(); // use an up to date timestamp so now old requests don't get accepted as if they matter anymore.
    }

    OUtils::Handle<OResource::Resource> TextureManager::create(OUtils::Handle<OResource::Resource> resource) {
        resource->claim();

        size_t len = strnlen(resource->path, 128);
        char *newpath = (char *)malloc(len + 3);
        ASSERT(newpath != NULL, "Failed to allocate memory for VFS path.\n");
        snprintf(newpath, len + 2, "%s*", resource->path);
        newpath[len + 2] = '\0';

        OUtils::Handle<OResource::Resource> ret = RESOURCE_INVALIDHANDLE;
        if ((ret = OResource::manager.get(newpath)) == RESOURCE_INVALIDHANDLE) {
            ret = OResource::manager.create(newpath, new Texture(resource));
            ret->as<Texture>()->self = ret;
            resource->release();
            return ret;
        } else {
            free(newpath); // we can free the allocation here, because it never needs to be given to the resource.
            resource->release();
            return ret; // Texture already exists, silently continue on as always.
        }

        resource->release();
        return RESOURCE_INVALIDHANDLE;
    }

    void TextureManager::tick(void) {
        this->fence.join(); // Force active-but-completed texture work to exit and return their work.

    }

    OUtils::Handle<OResource::Resource> TextureManager::create(const char *path) {
        return this->create(OResource::manager.get(path));
    }

}
