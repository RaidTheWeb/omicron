#include <engine/resources/texture.hpp>

namespace OResource {

    static void iteratektx2(uint32_t mip, uint32_t face, uint32_t width, uint32_t height, uint32_t depth, ktx_uint64_t facelodsize, void *pixels, struct Texture::work *work) {
        struct Texture::levelhdr header = {
            .offset = *work->offset,
            .size = facelodsize, // Assuming that the data is as it says it is.
            .uncompressedsize = facelodsize, // XXX: No compression supported yet.
            .level = mip
        };
        *work->offset += facelodsize;
        ASSERT(fwrite(&header, sizeof(struct Texture::levelhdr), 1, work->f) > 0, "Failed to write mip level header.\n");

        // Here we store the current location (in the headers array) and seek to where our allocated offset is before writing the data and returning.
        size_t offset = ftell(work->f); // Save point.
        ASSERT(!fseek(work->f, header.offset, SEEK_SET), "Failed to seek to location for data.\n");
        ASSERT(fwrite(pixels, facelodsize, 1, work->f) > 0, "Failed to write level data.\n");
        ASSERT(!fseek(work->f, offset, SEEK_SET), "Failed to restore position before seek.\n");
    }

    void Texture::convertktx2(const char *path, const char *out) {
        ktxTexture *texture = NULL;
        KTX_error_code res = ktxTexture_CreateFromNamedFile(path, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);
        ASSERT(res == KTX_SUCCESS, "Failed to load KTX2 texture from file %u.\n", res);

        ktx_uint8_t *data = ktxTexture_GetData(texture); // Gather the data of the entire file.
        ASSERT(data != NULL, "Failed to acquire KTX2 texture data.\n");

        // XXX: Sort through these based on what we can get through ktxTexture_GetImageOffset
        // Because the file format is designed to be so similar *to* KTX2 we can just as easily yank all the data level by level, but for other encoded formats it won't be so easy.
        // The reason we useour own format in the engine instead of KTX2 is because libktx (and by extension the format itself) *requires* loading the entire file every time you wish to grab data out of it. Our format means you can index any layer from just an open file descriptor (thereby bypassing the requirement for more memory to load *all* data).

        FILE *f = fopen(out, "w");
        ASSERT(f != NULL, "Failed to open OTexture output.\n");

        struct header header = { };

        ASSERT(texture->numDimensions == 3 ? !texture->isArray : true, "3D images as an array is not supported.\n");
        header.type = texture->isCubemap ?
            texture->isArray ? ORenderer::IMAGETYPE_CUBEARRAY : ORenderer::IMAGETYPE_CUBE :
            texture->numDimensions == 1 ?
                texture->isArray ? ORenderer::IMAGETYPE_1DARRAY : ORenderer::IMAGETYPE_1D :
            texture->numDimensions == 2 ?
                texture->isArray ? ORenderer::IMAGETYPE_2DARRAY : ORenderer::IMAGETYPE_2D :
            texture->numDimensions == 3 ? ORenderer::IMAGETYPE_3D : UINT8_MAX;

        header.width = texture->baseWidth;
        header.height = texture->baseHeight;
        header.depth = texture->baseDepth;
        header.levelcount = texture->numLevels;
        header.layercount = texture->numLayers;
        header.facecount = texture->numFaces;
        header.compression = 0; // XXX: No compression supported for now.
        header.format = convertformat(((ktxTexture2 *)texture)->vkFormat); // Convert KTX2 format to our format system.
        strcpy(header.magic, "OTEX"); // Insert header magic

        ASSERT(fwrite(&header, sizeof(struct header), 1, f) > 0, "Failed to write header.\n");

        size_t offset = sizeof(struct header) + (sizeof(struct levelhdr) * header.levelcount);

        struct work work = {
            .header = &header,
            .offset = &offset,
            .f = f
        };

        // Callback iterate over all levels and faces to allow manipulation of output at the end, only at the first bit of data for a level will we actually write out a header.
        ktxTexture_IterateLevels(texture, (PFNKTXITERCB)iteratektx2, &work);

        ktxTexture_Destroy(texture);
        fclose(f);
    }

    // Recursively generate mipmaps until no more can be generated.
    static void genmips(FILE *f, uint8_t *data, size_t width, size_t height, size_t bpp, struct Texture::levelhdr *levels, size_t idx) {
        if (width == 1 && height == 1) { // Reached highest
            free(data);
            return; // Cannot go lower than this or we'll have zero width/height images.
        }

        // Divide by decimal and floor here to let us scale down non-power of two images.
        size_t mw = floor(width / 2.0f);
        size_t mh = floor(height / 2.0f);

        uint8_t *mip = (uint8_t *)malloc(mw * mh * bpp);
        ASSERT(mip != NULL, "Failed to allocate memory for smaller mipmap level.\n");

        // Call STBIR to scale the image down to new size.
        stbir_resize_uint8_srgb(
            data, width, height, 0, mip, mw, mh, 0, // Do not specify stride or STBIR will not function.
            bpp == 4 ? STBIR_RGBA : STBIR_1CHANNEL
        );
        free(data); // Free data of previous mip level to avoid memory leaks.

        fseek(f, levels[idx].offset, SEEK_SET);
        ASSERT(fwrite(mip, mw * mh * bpp, 1, f) > 0, "Failed to write generated mip level.\n");
        genmips(f, mip, mw, mh, bpp, levels, idx - 1); // Recurse.
    }

    void Texture::convertstbi(const char *path, const char *out, bool mips) {

        int width, height, bpp = 0;
        stbi_info(path, &width, &height, &bpp);

        uint8_t *data = stbi_load(path, &width, &height, &bpp, bpp == 3 ? STBI_rgb_alpha : bpp); // Force RGBA.
        ASSERT(data != NULL, "STBI Failed to load texture data from RPak.\n");
        bpp = bpp == STBI_rgb ? STBI_rgb_alpha : bpp; // Force 32-bit on 24-bit formats.

        FILE *f = fopen(out, "w");
        ASSERT(f != NULL, "Failed to open OTexture output.\n");

        struct header header = { };
        header.type = ORenderer::IMAGETYPE_2D;
        header.width = width;
        header.height = height;
        header.depth = 1;
        header.layercount = 1;
        header.levelcount = 1 + floor(log2(glm::max(width, height))); // Generate mip maps?
        header.facecount = 1;
        header.compression = 0;
        header.format =
            bpp == STBI_rgb_alpha ? ORenderer::FORMAT_RGBA8SRGB :
            bpp == STBI_grey ? ORenderer::FORMAT_R8 :
            UINT8_MAX;
        strcpy(header.magic, "OTEX");

        ASSERT(fwrite(&header, sizeof(struct header), 1, f) > 0, "Failed to write header.\n");

        size_t offset = sizeof(struct header) + (sizeof(struct levelhdr) * header.levelcount);
        size_t initial = offset;
        struct levelhdr *levelhdrs = (struct levelhdr *)malloc(sizeof(struct levelhdr) * header.levelcount);
        size_t mwidth = glm::max(1.0f, floor((float)width / (1 << (header.levelcount - 1))));
        size_t mheight = glm::max(1.0f, floor((float)height / (1 << (header.levelcount - 1))));
        for (int32_t i = header.levelcount - 1, j = 0; i >= 0; i--, j++) {
            levelhdrs[j].level = i;
            levelhdrs[j].size = mwidth * mheight * bpp;
            levelhdrs[j].uncompressedsize = levelhdrs[j].size;
            levelhdrs[j].offset = offset;
            offset += levelhdrs[j].size; // Add back the initial value.
            // Continually reduce by a factor of two.
            mwidth = ceil(mwidth * 2.0f);
            mheight = ceil(mheight * 2.0f);
            ASSERT(fwrite(&levelhdrs[j], sizeof(struct levelhdr), 1, f) > 0, "Failed to write mip level header.\n");
        }

        // Seek to end of file to insert lowest mip level at its respective location (eof).
        ASSERT(!fseek(f, levelhdrs[header.levelcount - 1].offset, SEEK_SET), "Failed to seek to mip level 0");
        ASSERT(fwrite(data, levelhdrs[header.levelcount - 1].size, 1, f) > 0, "Failed to write first mip level data.\n");

        // Work backwards from the lowest mip level to the highest progressively downsizing the image.
        genmips(f, data, header.width, header.height, bpp, levelhdrs, header.levelcount - 2); // levelcount - 2 here means the second highest mip.
        fclose(f);
    }

    struct Texture::loaded Texture::loadheaders(OUtils::Handle<Resource> resource) {
        // XXX: Claimless! (Work out a system for allowing locks already held by the current fibre to lock an already locked lock)

        ASSERT(resource != RESOURCE_INVALIDHANDLE, "Attempting to load texture from invalid resource.\n");

        struct loaded loaded = { };
        loaded.resource = resource;
        if (resource->type == Resource::SOURCE_OSFS) {
            FILE *f = fopen(resource->path, "r");
            ASSERT(f != NULL, "Failed to open OTexture input.\n");
            ASSERT(!fseek(f, 0, SEEK_END), "Failed to seek end.\n");
            size_t size = ftell(f);
            ASSERT(!fseek(f, 0, SEEK_SET), "Failed to seek beginning.\n");
            ASSERT(size > sizeof(struct header), "File too small, or has empty data.\n");

            ASSERT(fread(&loaded.header, sizeof(struct header), 1, f) > 0, "Failed to read header.\n");
            ASSERT(!strncmp(loaded.header.magic, "OTEX", sizeof(loaded.header.magic)), "Invalid magic bytes.\n");
            ASSERT(loaded.header.levelcount > 0, "Need at least one mip level to be valid.\n");
            ASSERT(loaded.header.width > 0, "Invalid width.\n");
            ASSERT(loaded.header.height > 0, "Invalid height.\n");
            ASSERT(size > sizeof(struct header) + (sizeof(struct levelhdr) * loaded.header.levelcount), "Too small for level headers.\n");
            loaded.levels = (struct levelhdr *)malloc(sizeof(struct levelhdr) * loaded.header.levelcount);
            ASSERT(loaded.levels != NULL, "Failed to allocate buffer to accomodate for levels.\n");
            ASSERT(fread(loaded.levels, sizeof(struct levelhdr) * loaded.header.levelcount, 1, f) > 0, "Failed to read level headers.\n");
            fclose(f);
        } else if (resource->type == Resource::SOURCE_RPAK) {
            struct RPak::tableentry entry = resource->rpakentry;
            ASSERT(entry.uncompressedsize > sizeof(struct header), "File too small, or has empty data.\n");
            ASSERT(resource->rpak->read(resource->path, &loaded.header, sizeof(struct header), 0) > 0, "Failed to read RPak file.\n");

            ASSERT(!strncmp(loaded.header.magic, "OTEX", sizeof(loaded.header.magic)), "Invalid magic bytes.\n");
            ASSERT(loaded.header.levelcount > 0, "Need at least one mip level to be valid.\n");
            ASSERT(loaded.header.width > 0, "Invalid width.\n");
            ASSERT(loaded.header.height > 0, "Invalid height.\n");
            ASSERT(entry.uncompressedsize > sizeof(struct header) + (sizeof(struct levelhdr) * loaded.header.levelcount), "Too small for level headers.\n");
            loaded.levels = (struct levelhdr *)malloc(sizeof(struct levelhdr) * loaded.header.levelcount);
            ASSERT(loaded.levels != NULL, "Failed to allocate buffer to accomodate for levels.\n");
            ASSERT(resource->rpak->read(resource->path, loaded.levels, sizeof(struct levelhdr) * loaded.header.levelcount, sizeof(struct header)) > 0, "Failed to read level headers.\n");
        } else {
            ASSERT(false, "Invalid resource type.\n");
        }
        return loaded;
    }

    void Texture::loadlevel(struct loaded &loaded, uint32_t level, OUtils::Handle<Resource> resource, void *data, size_t size, size_t offset) {
        ZoneScoped;
        ASSERT(data != NULL, "Given NULL for provided buffer.\n");
        ASSERT(level < loaded.header.levelcount, "Level exceeds available levels in resource.\n");
        ASSERT(size + offset <= loaded.levels[level].size, "Reading beyond available data.\n");

        if (loaded.resource->type == Resource::SOURCE_RPAK) {
            struct RPak::tableentry entry = loaded.resource->rpakentry;
            ASSERT(resource->rpak->read(
                resource->path, data, loaded.levels[level].size,
                loaded.levels[level].offset + offset)
            > 0,"Failed to read RPak file.\n");
            // And with that, we're done with loading the data out of the file for a specific mip level. Pretty simple actually.
        } else if (loaded.resource->type == Resource::SOURCE_OSFS) {
            // The following is actually more expensive than the RPak version as we have to acquire a file descriptor from the kernel before actually getting our file data.
            FILE *f = fopen(resource->path, "r");
            ASSERT(f != NULL, "Failed to open OTexture file.\n");
            ASSERT(!fseek(f, loaded.levels[level].offset + offset, SEEK_SET), "Failed to seek to mip level data.\n");
            ASSERT(fread(data, loaded.levels[level].size, 1, f) > 0, "Failed to read mip level data.\n");
            fclose(f); // Close the file, and with that we're done.
        }

    }

    struct ORenderer::texture Texture::texload(OUtils::Handle<Resource> resource) {
        ZoneScoped;
        ASSERT(resource != RESOURCE_INVALIDHANDLE, "Attemping to load texture from invalid resource.\n");

        resource->claim();
        struct header header = { };
        size_t size = 0;
        uint8_t *data = NULL;
        struct ORenderer::texture texture;
        if (resource->type == Resource::SOURCE_OSFS) {
            FILE *f = fopen(resource->path, "r");
            ASSERT(f != NULL, "Failed to open OTexture input.\n");
            ASSERT(!fseek(f, 0, SEEK_END), "Failed to seek end.\n");
            size = ftell(f);
            ASSERT(!fseek(f, 0, SEEK_SET), "Failed to seek beginning.\n");
            ASSERT(size > sizeof(struct header), "File too small, or has empty data.\n");

            ASSERT(fread(&header, sizeof(struct header), 1, f) > 0, "Failed to read header.\n");
            ASSERT(!strncmp(header.magic, "OTEX", sizeof(header.magic)), "Invalid magic bytes.\n");
            ASSERT(header.levelcount > 0, "Need at least one mip level to be valid.\n");
            ASSERT(header.width > 0, "Invalid width.\n");
            ASSERT(header.height > 0, "Invalid height.\n");
            ASSERT(size > sizeof(struct header) + (sizeof(struct levelhdr) * header.levelcount), "Too small for level headers.\n");

            size_t offset = sizeof(struct header) + (sizeof(struct levelhdr) * header.levelcount); // Ignore everything else and just grab the data.
            ASSERT(!fseek(f, offset, SEEK_SET), "Failed to seek to the beginning of image data.\n");

            size -= offset;
            data = (uint8_t *)malloc(size);
            ASSERT(data != NULL, "Could not allocate buffer to accomodate for data size.\n");
            ASSERT(fread(data, size, 1, f) > 0, "Failed to read image data.\n");
            fclose(f);
        } else if (resource->type == Resource::SOURCE_RPAK) {
            struct RPak::tableentry entry = resource->rpakentry;
            ASSERT(entry.uncompressedsize > sizeof(struct header), "File too small, or has empty data.\n");
            ASSERT(resource->rpak->read(resource->path, &header, sizeof(struct header), 0) > 0, "Failed to read RPak file.\n");

            ASSERT(!strncmp(header.magic, "OTEX", sizeof(header.magic)), "Invalid magic bytes.\n");
            ASSERT(header.levelcount > 0, "Need at least one mip level to be valid.\n");
            ASSERT(header.width > 0, "Invalid width.\n");
            ASSERT(header.height > 0, "Invalid height.\n");
            ASSERT(entry.uncompressedsize > sizeof(struct header) + (sizeof(struct levelhdr) * header.levelcount), "Too small for level headers.\n");

            size_t offset = sizeof(struct header) + (sizeof(struct levelhdr) * header.levelcount); // Ignore everything else and just grab the data.
            size = entry.uncompressedsize - offset;
            data = (uint8_t *)malloc(size);
            ASSERT(data != NULL, "Could not allocate buffer to accomodate for data size.\n");
            ASSERT(resource->rpak->read(resource->path, data, size, offset) > 0, "Failed to read RPak file.\n");
        } else {
            return texture;
        }

        struct ORenderer::texturedesc desc = { };
        desc.type = header.type;
        desc.width = header.width;
        desc.height = header.height;
        desc.depth = header.depth;
        desc.mips = header.levelcount;
        desc.layers = header.layercount;
        desc.samples = ORenderer::SAMPLE_X1;
        desc.format = header.format;
        desc.memlayout = ORenderer::MEMLAYOUT_OPTIMAL;
        desc.usage = ORenderer::USAGE_SAMPLED | ORenderer::USAGE_DST;
        texture = loadfromdata(&desc, data, size);

        char text[256];
        sprintf(text, "OTexture Texture %s", basename(resource->path));
        ORenderer::context->setdebugname(texture, text);
        free(data);

        resource->release();
        return texture;
    }
}
