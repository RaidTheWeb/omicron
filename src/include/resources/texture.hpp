#ifndef _RESOURCES__TEXTURE_HPP
#define _RESOURCES__TEXTURE_HPP

#include <ktx.h>
#include <resources/resource.hpp>
#include <renderer/renderer.hpp>
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

            static struct ORenderer::texture load(Resource *resource) {
                struct ORenderer::texture texture;
                switch (resource->type) {
                    case Resource::SOURCE_RPAK:

                        break;
                    case Resource::SOURCE_VIRTUAL:
                        break;
                    case Resource::SOURCE_OSFS: {
                        ktxTexture *ktxtexture;
                        KTX_error_code res = ktxTexture_CreateFromNamedFile(resource->path, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxtexture);

                        ASSERT(res == KTX_SUCCESS, "Failed to create KTX texture from file %u.\n", res);
                        ASSERT(ktxtexture->classId == ktxTexture2_c, "KTX file must be KTX2\n");

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
                        desc.usage = ORenderer::USAGE_SAMPLED;
                        ktxTexture_Destroy(ktxtexture);
                        printf("%ux%ux%u (%u mips, type %u), format %u\n", desc.width, desc.height, desc.depth, desc.mips, desc.type, desc.format);
                        break;
                    }
                }

                return texture;
            }

            static struct ORenderer::texture load(const char *path) {
                return load(manager.get(path));
            }
    };
};

#endif
