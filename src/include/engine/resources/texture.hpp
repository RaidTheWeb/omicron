#ifndef _ENGINE__RESOURCES__TEXTURE_HPP
#define _ENGINE__RESOURCES__TEXTURE_HPP

#include <ktx.h>
#include <engine/resources/resource.hpp>
#include <engine/renderer/renderer.hpp>
#include <vulkan/vulkan.h>

namespace OResource {
    class Texture {
        public:
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

                ORenderer::Stream *stream = ORenderer::context->getimmediate();
                struct ORenderer::bufferimagecopy region = { };
                region.offset = 0;
                region.rowlen = 0;
                region.imgheight = 0;
                region.imgoff = { 0, 0, 0 };
                region.imgextent = { desc->width, desc->height, desc->depth };
                region.aspect = ORenderer::ASPECT_COLOUR;
                region.mip = 0;
                region.baselayer = 0;
                region.layercount = desc->layers;

                struct ORenderer::texture texture;

                ORenderer::context->createtexture(desc, &texture);

                stream->claim();
                stream->begin();

                stream->transitionlayout(texture, desc->format, ORenderer::LAYOUT_TRANSFERDST);
                stream->copybufferimage(region, staging, texture);
                stream->transitionlayout(texture, desc->format, ORenderer::LAYOUT_SHADERRO);

                stream->end();
                stream->release();

                ORenderer::context->destroybuffer(&staging);
                return texture;
            }

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
