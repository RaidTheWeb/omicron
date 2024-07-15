#ifndef _ENGINE__RESOURCES__TEXTURE_HPP
#define _ENGINE__RESOURCES__TEXTURE_HPP

#include <ktx.h>
#include <engine/resources/resource.hpp>
#include <engine/renderer/renderer.hpp>
#include <vulkan/vulkan.h>

namespace OResource {
    class Texture {
        public:
            struct header {
                char magic[5]; // OTEX\0
                uint32_t type; // Image type (2D, 3D, etc.)
                uint32_t format; // Generic format for the image.
                uint32_t width; // Width (assumed to be the lowest mip level)
                uint32_t height; // Height (assumed to be the lowest mip level)
                uint32_t depth; // Depth
                uint32_t facecount;
                uint32_t layercount; // Number of layers in the image (or faces on a cubemap).
                uint32_t levelcount; // Mipmap levels
                uint32_t compression; // Compression enabled? (and what kind? supercompression? CPU-only?)
            };

            struct levelhdr {
                size_t offset; // Offset of mip level data within the file
                size_t size; // Size of texture data (will be the compressed size if the texture is marked to use host or GPU compression)
                size_t uncompressedsize; // Will represent the original data size (differing from the compressed data size) only *if* the texture is marked to use host compression or supercompression
                uint32_t level;
            };

            struct loaded {
                OUtils::Handle<OResource::Resource> resource;
                struct header header;
                struct levelhdr *levels;
            };

            struct work  {
                ktxTexture *texture;
                struct header *header;
                size_t *offset;
                FILE *f;
            };

            // Every mip level stores its internal byte data formatted by encapsulating layers, faces, and pixels in a tightly packed format.
            // layers
            //      - faces
            //          - depth
            //              - raw pixel data
            // This layout transfers over to the GPU in about the same layout. Thanks to each level being separated this way, a single mip upload through texture streaming will upload the layers, faces, and pixels over in one upload.

            static struct loaded loadheaders(OUtils::Handle<OResource::Resource> resource);
            // Load individual mip level into a buffer (XXX: level here represents the index inside the texture file, which is (0 - mip) + levelcount where mip is the typical GPU representation, this is because level headers are stored in reverse order in OTexture files (basically to enable incremental reads for streaming)).
            static void loadlevel(struct loaded &loaded, uint32_t level, OUtils::Handle<Resource> resource, void *data, size_t size, size_t offset = 0);
            static struct loaded loadheaders(const char *path) {
                return loadheaders(manager.get(path));
            }
            static void convertktx2(const char *path, const char *out);
            static void convertstbi(const char *path, const char *out, bool mips = false);

            // Deduce bytes per pixel based on format.
            static uint8_t strideformat(uint8_t format) {
                switch (format) {
                    case ORenderer::FORMAT_RGBA4:
                    case ORenderer::FORMAT_BGRA4:

                    case ORenderer::FORMAT_R5G6B5:
                    case ORenderer::FORMAT_B5G6R5:

                    case ORenderer::FORMAT_RGB5A1:
                    case ORenderer::FORMAT_BGR5A1:

                    case ORenderer::FORMAT_RG8:
                    case ORenderer::FORMAT_RG8U:
                    case ORenderer::FORMAT_RG8I:
                    case ORenderer::FORMAT_RG8SRGB:

                    case ORenderer::FORMAT_R16:
                    case ORenderer::FORMAT_R16S:
                    case ORenderer::FORMAT_R16U:
                    case ORenderer::FORMAT_R16I:
                    case ORenderer::FORMAT_R16F:

                    case ORenderer::FORMAT_D16:
                        return 2;

                    case ORenderer::FORMAT_RGB8:
                    case ORenderer::FORMAT_RGB8S:
                    case ORenderer::FORMAT_RGB8U:
                    case ORenderer::FORMAT_RGB8I:
                    case ORenderer::FORMAT_RGB8SRGB:
                        return 3;

                    case ORenderer::FORMAT_RGBA8:
                    case ORenderer::FORMAT_RGBA8S:
                    case ORenderer::FORMAT_RGBA8U:
                    case ORenderer::FORMAT_RGBA8I:
                    case ORenderer::FORMAT_RGBA8SRGB:

                    case ORenderer::FORMAT_BGRA8U:
                    case ORenderer::FORMAT_BGRA8S:
                    case ORenderer::FORMAT_BGRA8I:
                    case ORenderer::FORMAT_BGRA8SRGB:

                    case ORenderer::FORMAT_RG16:
                    case ORenderer::FORMAT_RG16S:
                    case ORenderer::FORMAT_RG16U:
                    case ORenderer::FORMAT_RG16I:
                    case ORenderer::FORMAT_RG16F:

                    case ORenderer::FORMAT_R32U:
                    case ORenderer::FORMAT_R32I:
                    case ORenderer::FORMAT_R32F:
                    case ORenderer::FORMAT_D32F:
                    case ORenderer::FORMAT_D16S8U:
                    case ORenderer::FORMAT_D24S8U:
                        return 4;

                    case ORenderer::FORMAT_RGB16:
                    case ORenderer::FORMAT_RGB16S:
                    case ORenderer::FORMAT_RGB16U:
                    case ORenderer::FORMAT_RGB16I:
                    case ORenderer::FORMAT_RGB16F:
                        return 6;

                    case ORenderer::FORMAT_RGBA16:
                    case ORenderer::FORMAT_RGBA16S:
                    case ORenderer::FORMAT_RGBA16U:
                    case ORenderer::FORMAT_RGBA16I:
                    case ORenderer::FORMAT_RGBA16F:

                    case ORenderer::FORMAT_RG32F:
                        return 8;
                    case ORenderer::FORMAT_RGB32F:
                    case ORenderer::FORMAT_D32FS8U:
                        return 12;
                    case ORenderer::FORMAT_RGBA32U:
                    case ORenderer::FORMAT_RGBA32I:
                    case ORenderer::FORMAT_RGBA32F:
                        return 16;
                    case ORenderer::FORMAT_R8:
                    case ORenderer::FORMAT_R8S:
                    case ORenderer::FORMAT_R8U:
                    case ORenderer::FORMAT_R8I:
                    case ORenderer::FORMAT_R8SRGB:
                    default:
                        return 1;
                }
            }

            static uint8_t convertformat(uint32_t vkformat) {
                switch (vkformat) {
                    // Vulkan to our render format (since KTX2 provides us a vulkan format member already!)
                    case 2: return ORenderer::FORMAT_RGBA4;
                    case 3: return ORenderer::FORMAT_BGRA4;
                    case 4: return ORenderer::FORMAT_R5G6B5;
                    case 5: return ORenderer::FORMAT_B5G6R5;
                    case 6: return ORenderer::FORMAT_RGB5A1;
                    case 7: return ORenderer::FORMAT_BGR5A1;
                    case 9: return ORenderer::FORMAT_R8;
                    case 10: return ORenderer::FORMAT_R8S;
                    case 13: return ORenderer::FORMAT_R8U;
                    case 14: return ORenderer::FORMAT_R8I;
                    case 15: return ORenderer::FORMAT_R8SRGB;
                    case 16: return ORenderer::FORMAT_RG8;
                    case 17: return ORenderer::FORMAT_RG8S;
                    case 20: return ORenderer::FORMAT_RG8U;
                    case 21: return ORenderer::FORMAT_RG8I;
                    case 22: return ORenderer::FORMAT_RG8SRGB;
                    case 23: return ORenderer::FORMAT_RGB8;
                    case 24: return ORenderer::FORMAT_RGB8S;
                    case 27: return ORenderer::FORMAT_RGB8U;
                    case 28: return ORenderer::FORMAT_RGB8I;
                    case 29: return ORenderer::FORMAT_RGB8SRGB;
                    case 37: return ORenderer::FORMAT_RGBA8;
                    case 38: return ORenderer::FORMAT_RGBA8S;
                    case 41: return ORenderer::FORMAT_RGBA8U;
                    case 42: return ORenderer::FORMAT_RGBA8I;
                    case 43: return ORenderer::FORMAT_RGBA8SRGB;
                    case 44: return ORenderer::FORMAT_BGRA8U;
                    case 45: return ORenderer::FORMAT_BGRA8S;
                    case 48: return ORenderer::FORMAT_BGRA8U;
                    case 49: return ORenderer::FORMAT_BGRA8I;
                    case 50: return ORenderer::FORMAT_BGRA8SRGB;
                    case 58: return ORenderer::FORMAT_RGB10A2;
                    case 70: return ORenderer::FORMAT_R16;
                    case 71: return ORenderer::FORMAT_R16S;
                    case 74: return ORenderer::FORMAT_R16U;
                    case 75: return ORenderer::FORMAT_R16I;
                    case 76: return ORenderer::FORMAT_R16F;
                    case 77: return ORenderer::FORMAT_RG16;
                    case 78: return ORenderer::FORMAT_RG16S;
                    case 81: return ORenderer::FORMAT_RG16U;
                    case 82: return ORenderer::FORMAT_RG16I;
                    case 83: return ORenderer::FORMAT_RG16F;
                    case 84: return ORenderer::FORMAT_RGB16;
                    case 85: return ORenderer::FORMAT_RGB16S;
                    case 88: return ORenderer::FORMAT_RGB16U;
                    case 89: return ORenderer::FORMAT_RGB16I;
                    case 90: return ORenderer::FORMAT_RGB16F;
                    case 91: return ORenderer::FORMAT_RGBA16;
                    case 92: return ORenderer::FORMAT_RGBA16S;
                    case 95: return ORenderer::FORMAT_RGBA16U;
                    case 96: return ORenderer::FORMAT_RGBA16I;
                    case 97: return ORenderer::FORMAT_RGBA16F;
                    case 98: return ORenderer::FORMAT_R32U;
                    case 99: return ORenderer::FORMAT_R32I;
                    case 100: return ORenderer::FORMAT_R32F;
                    case 103: return ORenderer::FORMAT_RG32F;
                    case 106: return ORenderer::FORMAT_RGB32F;
                    case 107: return ORenderer::FORMAT_RGBA32U;
                    case 108: return ORenderer::FORMAT_RGBA32I;
                    case 109: return ORenderer::FORMAT_RGBA32F;
                    case 124: return ORenderer::FORMAT_D16;
                    case 126: return ORenderer::FORMAT_D32F;
                    case 127: return ORenderer::FORMAT_D0S8U;
                    case 128: return ORenderer::FORMAT_D16S8U;
                    case 129: return ORenderer::FORMAT_D24S8U;
                    case 130: return ORenderer::FORMAT_D32FS8U;
                    default: return ORenderer::FORMAT_COUNT;
                }
            }

            static struct ORenderer::texture loadfromdata(struct ORenderer::texturedesc *desc, uint8_t *data, size_t size) {
                struct ORenderer::buffer staging = { };
                ASSERT(ORenderer::context->createbuffer(
                    &staging, size, ORenderer::BUFFER_TRANSFERSRC,
                    ORenderer::MEMPROP_CPUVISIBLE | ORenderer::MEMPROP_CPUCOHERENT | ORenderer::MEMPROP_CPUSEQUENTIALWRITE,
                    0
                ) == ORenderer::RESULT_SUCCESS, "Failed to create staging buffer.\n");

                struct ORenderer::buffermap stagingmap = { };
                ASSERT(ORenderer::context->mapbuffer(&stagingmap, staging, 0, size) == ORenderer::RESULT_SUCCESS, "Failed to map staging buffer.\n");
                memcpy(stagingmap.mapped[0], data, size);
                ORenderer::context->unmapbuffer(stagingmap);

                ORenderer::Stream *stream = ORenderer::context->requeststream(ORenderer::STREAM_IMMEDIATE);

                struct ORenderer::texture texture;
                ORenderer::context->createtexture(desc, &texture);

                stream->claim();
                stream->begin();

                // Derive the size of the minimum.
                size_t width = glm::max(1.0f, floor((float)desc->width / (1 << (desc->mips - 1))));
                size_t height = glm::max(1.0f, floor((float)desc->height / (1 << (desc->mips - 1))));
                size_t depth = desc->depth;
                size_t offset = 0;

                size_t layout = ORenderer::LAYOUT_UNDEFINED;
                size_t stage = ORenderer::PIPELINE_STAGETOP;
                for (ssize_t i = desc->mips - 1; i >= 0; i--) {
                    // We include the transitions here as they force a barrier around the memory.
                    stream->barrier(
                        texture, desc->format, layout, ORenderer::LAYOUT_TRANSFERDST,
                        stage, ORenderer::PIPELINE_STAGETRANSFER, // undefined/shaderro -> transfer dst
                        layout == ORenderer::LAYOUT_UNDEFINED ? 0 : ORenderer::ACCESS_SHADERREAD | ORenderer::ACCESS_INPUTREAD,
                        ORenderer::ACCESS_TRANSFERWRITE
                    );
                    stage = ORenderer::PIPELINE_STAGETRANSFER;
                    layout = ORenderer::LAYOUT_TRANSFERDST;
                    struct ORenderer::bufferimagecopy region = { };
                    region.offset = offset;
                    region.rowlen = width;
                    region.imgheight = height;
                    region.imgoff = { 0, 0, 0 };
                    region.imgextent = { .width = width, .height = height, .depth = depth };
                    region.aspect = ORenderer::ASPECT_COLOUR;
                    region.mip = i;
                    region.baselayer = 0;
                    region.layercount = desc->layers;
                    offset += (width * height * depth * strideformat(desc->format)); // XXX: Does not respect block sizes.

                    stream->copybufferimage(region, staging, texture);
                    stream->barrier(
                        texture, desc->format, layout, ORenderer::LAYOUT_SHADERRO,
                        stage, ORenderer::PIPELINE_STAGEFRAGMENT, // transfer dst -> shaderro
                        ORenderer::ACCESS_TRANSFERWRITE, ORenderer::ACCESS_SHADERREAD | ORenderer::ACCESS_INPUTREAD
                    );
                    stage = ORenderer::PIPELINE_STAGEFRAGMENT;
                    layout = ORenderer::LAYOUT_SHADERRO;
                    if (width == desc->width && height == desc->height) {
                        break;
                    }

                    // Decrease scale by power of two.
                    width <<= 1;
                    height <<= 1;
                }

                stream->end();
                stream->release();
                ORenderer::context->submitstream(stream, true);

                ORenderer::context->destroybuffer(&staging);
                return texture;
            }

            static struct ORenderer::texture texload(OUtils::Handle<Resource> resource);

            static struct ORenderer::texture ktxload(OUtils::Handle<Resource> resource) {
                ASSERT(resource != RESOURCE_INVALIDHANDLE, "Attempting to load texture from invalid resource.\n");
                struct ORenderer::texture texture;
                switch (resource->type) {
                    case Resource::SOURCE_RPAK: {
                        struct RPak::tableentry entry = resource->rpakentry;
                        uint8_t *buffer = (uint8_t *)malloc(entry.uncompressedsize);
                        ASSERT(buffer != NULL, "Failed to allocate buffer for RPak texture data read.\n");
                        ASSERT(resource->rpak->read(resource->path, buffer, entry.uncompressedsize, 0) > 0, "Failed to read RPak file.\n");
                        ktxTexture *ktxtexture;
                        KTX_error_code res = ktxTexture_CreateFromMemory(buffer, entry.uncompressedsize, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxtexture);
                        ASSERT(res == KTX_SUCCESS, "Failed to create KTX texture from RPak file %u.\n", res);
                        ASSERT(ktxtexture->classId == ktxTexture2_c, "KTX file must be KTX2.\n");

                        ktx_uint8_t *data = ktxTexture_GetData(ktxtexture);
                        struct ORenderer::texturedesc desc = { };
                        ASSERT(ktxtexture->numDimensions == 3 ? ktxtexture->isArray == false : true, "3D image cannot be an array.\n");
                        desc.type = ktxtexture->isCubemap ? ktxtexture->isArray ? ORenderer::IMAGETYPE_CUBEARRAY : ORenderer::IMAGETYPE_CUBE :
                            ktxtexture->numDimensions == 1 ? ktxtexture->isArray ? ORenderer::IMAGETYPE_1DARRAY : ORenderer::IMAGETYPE_1D :
                            ktxtexture->numDimensions == 2 ? ktxtexture->isArray ? ORenderer::IMAGETYPE_2DARRAY : ORenderer::IMAGETYPE_2D :
                            ktxtexture->numDimensions == 3 ? ORenderer::IMAGETYPE_3D :
                            UINT8_MAX;
                        ASSERT(desc.type != UINT8_MAX, "Unsupported KTX2 format.\n");
                        desc.width = ktxtexture->baseWidth;
                        desc.height = ktxtexture->baseHeight;
                        desc.depth = ktxtexture->baseDepth;
                        printf("mips: %u\n", ktxtexture->numLevels);
                        desc.mips = ktxtexture->numLevels;
                        desc.layers = ktxtexture->isCubemap ? 6 : ktxtexture->numLayers;
                        desc.samples = ORenderer::SAMPLE_X1;
                        // we assume KTX2 for everything
                        desc.format = convertformat(((ktxTexture2 *)ktxtexture)->vkFormat);
                        ASSERT(desc.format != ORenderer::FORMAT_COUNT, "Could not infer ktx2 image format from provided Vulkan format %d.\n", ((ktxTexture2 *)ktxtexture)->vkFormat);
                        desc.memlayout = ORenderer::MEMLAYOUT_OPTIMAL;
                        desc.usage = ORenderer::USAGE_SAMPLED | ORenderer::USAGE_DST;
                        texture = loadfromdata(&desc, data, ktxtexture->dataSize);

                        char text[256];
                        sprintf(text, "RPak KTX2 Texture %s", basename(resource->path));
                        ORenderer::context->setdebugname(texture, text);
                        ktxTexture_Destroy(ktxtexture);
                        free(buffer);

                        break;
                    }
                    case Resource::SOURCE_VIRTUAL:
                        break;
                    case Resource::SOURCE_OSFS: {
                        ktxTexture *ktxtexture;
                        KTX_error_code res = ktxTexture_CreateFromNamedFile(resource->path, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxtexture);

                        ASSERT(res == KTX_SUCCESS, "Failed to create KTX texture from file %u.\n", res);
                        ASSERT(ktxtexture->classId == ktxTexture2_c, "KTX file must be KTX2.\n");

                        // XXX: Stream this in instead of just shoving it into VRAM immediately
                        ktx_uint8_t *data = ktxTexture_GetData(ktxtexture);
                        struct ORenderer::texturedesc desc = { };
                        ASSERT(ktxtexture->numDimensions == 3 ? ktxtexture->isArray == false : true, "3D image cannot be an array.\n");
                        desc.type = ktxtexture->isCubemap ? ktxtexture->isArray ? ORenderer::IMAGETYPE_CUBEARRAY : ORenderer::IMAGETYPE_CUBE :
                            ktxtexture->numDimensions == 1 ? ktxtexture->isArray ? ORenderer::IMAGETYPE_1DARRAY : ORenderer::IMAGETYPE_1D :
                            ktxtexture->numDimensions == 2 ? ktxtexture->isArray ? ORenderer::IMAGETYPE_2DARRAY : ORenderer::IMAGETYPE_2D :
                            ktxtexture->numDimensions == 3 ? ORenderer::IMAGETYPE_3D :
                            UINT8_MAX;
                        ASSERT(desc.type != UINT8_MAX, "Unsupported KTX2 format.\n");
                        desc.width = ktxtexture->baseWidth;
                        desc.height = ktxtexture->baseHeight;
                        desc.depth = ktxtexture->baseDepth;
                        desc.mips = ktxtexture->numLevels;
                        desc.layers = ktxtexture->isCubemap ? 6 : ktxtexture->numLayers;
                        desc.samples = ORenderer::SAMPLE_X1;
                        // we assume KTX2 for everything
                        desc.format = convertformat(((ktxTexture2 *)ktxtexture)->vkFormat);
                        ASSERT(desc.format != ORenderer::FORMAT_COUNT, "Could not infer ktx2 image format from provided Vulkan format %d.\n", ((ktxTexture2 *)ktxtexture)->vkFormat);
                        desc.memlayout = ORenderer::MEMLAYOUT_OPTIMAL;
                        desc.usage = ORenderer::USAGE_SAMPLED | ORenderer::USAGE_DST;
                        texture = loadfromdata(&desc, data, ktxtexture->dataSize);

                        char text[256];
                        sprintf(text, "OSFS KTX2 Texture %s", basename(resource->path));
                        ORenderer::context->setdebugname(texture, text);
                        ktxTexture_Destroy(ktxtexture);
                        break;
                    }
                }

                return texture;
            }

            static struct ORenderer::texture stbiload(OUtils::Handle<Resource> resource) {
                ASSERT(resource != RESOURCE_INVALIDHANDLE, "Attempting to load texture from invalid resource.\n");
                struct ORenderer::texture texture;
                switch (resource->type) {
                    case Resource::SOURCE_RPAK: {
                        struct RPak::tableentry entry = resource->rpakentry;
                        uint8_t *buffer = (uint8_t *)malloc(entry.uncompressedsize);
                        ASSERT(buffer != NULL, "Failed to allocate buffer for RPak texture data read.\n");
                        ASSERT(resource->rpak->read(resource->path, buffer, entry.uncompressedsize, 0) > 0, "Failed to read RPak file.\n");

                        int width, height, bpp = 0;
                        stbi_info_from_memory(buffer, entry.uncompressedsize, &width, &height, &bpp);
                        uint8_t *data = stbi_load_from_memory(buffer, entry.uncompressedsize, &width, &height, &bpp, bpp == 3 ? STBI_rgb_alpha : 0); // Force RGB to be RGBA
                        printf("image %s of %dx%dx%d.\n", resource->path, width, height, bpp);
                        ASSERT(data != NULL, "STBI failed to load texture data from RPak.\n");
                        struct ORenderer::texturedesc desc = { };
                        desc.type = ORenderer::IMAGETYPE_2D;
                        desc.width = width;
                        desc.height = height;
                        desc.depth = 1;
                        desc.mips = 1;
                        desc.layers = 1;
                        desc.samples = ORenderer::SAMPLE_X1;
                        desc.format = bpp == STBI_rgb_alpha || bpp == STBI_rgb ? ORenderer::FORMAT_RGBA8SRGB :
                            bpp == STBI_grey ? ORenderer::FORMAT_R8 : UINT8_MAX;
                        ASSERT(desc.format != UINT8_MAX, "Unsupported image format.\n");
                        desc.memlayout = ORenderer::MEMLAYOUT_OPTIMAL;
                        desc.usage = ORenderer::USAGE_SAMPLED | ORenderer::USAGE_DST;
                        texture = loadfromdata(&desc, data, width * height * (bpp == 3 ? 4 : bpp));

                        char text[256];
                        sprintf(text, "RPak STBI Texture %s", basename(resource->path));
                        ORenderer::context->setdebugname(texture, text);
                        stbi_image_free(data);
                        free(buffer);
                        // exit(1);

                        break;
                    }
                    case Resource::SOURCE_VIRTUAL:
                        break;
                    case Resource::SOURCE_OSFS: {
                        ktxTexture *ktxtexture;
                        KTX_error_code res = ktxTexture_CreateFromNamedFile(resource->path, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxtexture);

                        ASSERT(res == KTX_SUCCESS, "Failed to create KTX texture from file %u.\n", res);
                        ASSERT(ktxtexture->classId == ktxTexture2_c, "KTX file must be KTX2.\n");

                        // XXX: Stream this in instead of just shoving it into VRAM immediately
                        ktx_uint8_t *data = ktxTexture_GetData(ktxtexture);
                        struct ORenderer::texturedesc desc = { };
                        ASSERT(ktxtexture->numDimensions == 3 ? ktxtexture->isArray == false : true, "3D image cannot be an array.\n");
                        desc.type = ktxtexture->isCubemap ? ktxtexture->isArray ? ORenderer::IMAGETYPE_CUBEARRAY : ORenderer::IMAGETYPE_CUBE :
                            ktxtexture->numDimensions == 1 ? ktxtexture->isArray ? ORenderer::IMAGETYPE_1DARRAY : ORenderer::IMAGETYPE_1D :
                            ktxtexture->numDimensions == 2 ? ktxtexture->isArray ? ORenderer::IMAGETYPE_2DARRAY : ORenderer::IMAGETYPE_2D :
                            ktxtexture->numDimensions == 3 ? ORenderer::IMAGETYPE_3D :
                            UINT8_MAX;
                        ASSERT(desc.type != UINT8_MAX, "Unsupported KTX2 format.\n");
                        desc.width = ktxtexture->baseWidth;
                        desc.height = ktxtexture->baseHeight;
                        desc.depth = ktxtexture->baseDepth;
                        desc.mips = ktxtexture->numLevels;
                        desc.layers = ktxtexture->isCubemap ? 6 : ktxtexture->numLayers;
                        desc.samples = ORenderer::SAMPLE_X1;
                        // we assume KTX2 for everything
                        desc.format = convertformat(((ktxTexture2 *)ktxtexture)->vkFormat);
                        desc.memlayout = ORenderer::MEMLAYOUT_OPTIMAL;
                        desc.usage = ORenderer::USAGE_SAMPLED | ORenderer::USAGE_DST;
                        texture = loadfromdata(&desc, data, ktxtexture->dataSize);

                        char text[256];
                        sprintf(text, "OSFS KTX2 Texture %s", basename(resource->path));
                        ORenderer::context->setdebugname(texture, text);
                        ktxTexture_Destroy(ktxtexture);
                        break;
                    }
                }

                return texture;
            }

            // XXX: Generic for utils.
            static bool endswith(const char* str, const char* suffix) {
                ASSERT(str != NULL && suffix != NULL, "Invalid string or suffic.\n");

                const size_t len = strlen(str);
                const size_t suffixlen = strlen(suffix);

                if (suffixlen > len) {
                    return false; // Implicitly false as a string cannot end with something it isn't even long enough to contain.
                }
                return !strncmp(str + len - suffixlen, suffix, suffixlen);
            }


            static struct ORenderer::texture load(const char *path) {
                printf("'%s'\n", path);
                // XXX: File type inference?
                if (endswith(path, ".ktx2")) {
                    return ktxload(manager.get(path));
                } else if (endswith(path, ".otex")) {
                    return texload(manager.get(path));
                } else if (endswith(path, ".png") || endswith(path, ".jpg") || endswith(path, ".jpeg")) { // All handled with stbi.
                    return stbiload(manager.get(path));
                } else {
                    ASSERT(false, "No viable conversion for unknown file type inferred from path '%s'.\n", path);
                    return (struct ORenderer::texture) { .handle = RENDERER_INVALIDHANDLE };
                }
            }
    };
};

#endif
