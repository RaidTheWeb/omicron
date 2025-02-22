#include <engine/renderer/backend/vulkan.hpp>
#include <engine/renderer/bindless.hpp>
#include <engine/utils/print.hpp>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include <engine/ext/vk_mem_alloc.hpp>

#define VK_IMPORT_FUNC(OPT, FUNC) PFN_##FUNC FUNC
#define VK_IMPORT_INSTANCE_FUNC VK_IMPORT_FUNC
#define VK_IMPORT_DEVICE_FUNC VK_IMPORT_FUNC
VK_IMPORT
VK_IMPORT_INSTANCE
VK_IMPORT_DEVICE
#undef VK_IMPORT_FUNC
#undef VK_IMPORT_INSTANCE_FUNC
#undef VK_IMPORT_DEVICE_FUNC

namespace OVulkan {
    static inline VkResult enumextensions(VkPhysicalDevice phy, const char *layer, uint32_t *count, VkExtensionProperties *props) {
        return (phy == VK_NULL_HANDLE) ? vkEnumerateInstanceExtensionProperties(layer, count, props) : vkEnumerateDeviceExtensionProperties(phy, layer, count, props);
    }

    static void checkextensions(VkPhysicalDevice phy, struct extension *extensions, size_t count) {
        uint32_t propcount;
        VkResult res = enumextensions(phy, NULL, &propcount, NULL);
        ASSERT(res == VK_SUCCESS, "Failed to enumerate extensions %d.\n", res);
        if (res == VK_SUCCESS && propcount > 0) {
            VkExtensionProperties *props = (VkExtensionProperties *)malloc(sizeof(VkExtensionProperties) * propcount);
            ASSERT(props != NULL, "Failed to allocate memory for Vulkan extension properties.\n");
            res = enumextensions(phy, NULL, &propcount, props);
            ASSERT(res == VK_SUCCESS, "Failed to enumerate extensions %d.\n", res);
            for (size_t i = 0; i < propcount; i++) {
                for (size_t j = 0; j < count; j++) {
                    if (!strcmp(extensions[j].name, props[i].extensionName)) {
                        extensions[j].supported = true;
                    }
                }
            }
            free(props);
        }
    }

    static struct extension extensions[] = {
        { VK_KHR_SURFACE_EXTENSION_NAME, false, true, false },

#ifdef VK_USE_PLATFORM_XCB_KHR
        { VK_KHR_XCB_SURFACE_EXTENSION_NAME, false, true, false },
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
        { VK_KHR_XLIB_SURFACE_EXTENSION_NAME, false, true, false },
#endif
#ifdef OMICRON_DEBUG
        { VK_EXT_DEBUG_UTILS_EXTENSION_NAME, false, true, false }
#endif
    };

    static struct extension devextensions[] = {
        // TODO: Consider bindless rendering architecture
        { VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, false, false, false },
        { VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, false, false, false },
        { VK_KHR_SWAPCHAIN_EXTENSION_NAME, false, false, false },
        { VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME, false, false, false },
        { VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME, false, false, false }
    };

    // XXX: Translation table should account for SRGB
    VkFormat formattable[] = {
        VK_FORMAT_R8_UNORM,
        VK_FORMAT_R8_SINT,
        VK_FORMAT_R8_UINT,
        VK_FORMAT_R8_SNORM,
        VK_FORMAT_R8_SRGB,

        VK_FORMAT_R16_UNORM,
        VK_FORMAT_R16_SINT,
        VK_FORMAT_R16_UINT,
        VK_FORMAT_R16_SFLOAT,
        VK_FORMAT_R16_SNORM,

        VK_FORMAT_R32_SINT,
        VK_FORMAT_R32_UINT,
        VK_FORMAT_R32_SFLOAT,

        VK_FORMAT_R8G8_UNORM,
        VK_FORMAT_R8G8_SINT,
        VK_FORMAT_R8G8_UINT,
        VK_FORMAT_R8G8_SNORM,
        VK_FORMAT_R8G8_SRGB,

        VK_FORMAT_R16G16_UNORM,
        VK_FORMAT_R16G16_SINT,
        VK_FORMAT_R16G16_UINT,
        VK_FORMAT_R16G16_SFLOAT,
        VK_FORMAT_R16G16_SNORM,

        VK_FORMAT_R32G32_SFLOAT,

        VK_FORMAT_R8G8B8_UNORM,
        VK_FORMAT_R8G8B8_SINT,
        VK_FORMAT_R8G8B8_UINT,
        VK_FORMAT_R8G8B8_SNORM,
        VK_FORMAT_R8G8B8_SRGB,

        VK_FORMAT_R16G16B16_UNORM,
        VK_FORMAT_R16G16B16_SINT,
        VK_FORMAT_R16G16B16_UINT,
        VK_FORMAT_R16G16B16_SFLOAT,
        VK_FORMAT_R16G16B16_SNORM,

        VK_FORMAT_R32G32B32_SFLOAT,

        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_B8G8R8A8_SINT,
        VK_FORMAT_B8G8R8A8_UINT,
        VK_FORMAT_B8G8R8A8_SNORM,
        VK_FORMAT_B8G8R8A8_SRGB,

        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R8G8B8A8_SINT,
        VK_FORMAT_R8G8B8A8_UINT,
        VK_FORMAT_R8G8B8A8_SNORM,
        VK_FORMAT_R8G8B8A8_SRGB,

        VK_FORMAT_R16G16B16A16_UNORM,
        VK_FORMAT_R16G16B16A16_SINT,
        VK_FORMAT_R16G16B16A16_UINT,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R16G16B16A16_SNORM,

        VK_FORMAT_R32G32B32A32_SINT,
        VK_FORMAT_R32G32B32A32_UINT,
        VK_FORMAT_R32G32B32A32_SFLOAT,

        VK_FORMAT_B5G6R5_UNORM_PACK16,
        VK_FORMAT_R5G6B5_UNORM_PACK16,

        VK_FORMAT_B4G4R4A4_UNORM_PACK16,
        VK_FORMAT_R4G4B4A4_UNORM_PACK16,

        VK_FORMAT_B5G5R5A1_UNORM_PACK16,
        VK_FORMAT_R5G5B5A1_UNORM_PACK16,

        VK_FORMAT_A2R10G10B10_UNORM_PACK32,

        VK_FORMAT_D16_UNORM,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT
    };

    static VkImageType imagetypetable[] = {
        VK_IMAGE_TYPE_1D, // RENDERER_IMAGETYPE1D
        VK_IMAGE_TYPE_2D, // RENDERER_IMAGETYPE2D
        VK_IMAGE_TYPE_3D, // RENDERER_IMAGETYPE3D
        VK_IMAGE_TYPE_2D, // RENDERER_IMAGETYPECUBE
        VK_IMAGE_TYPE_1D, // RENDERER_IMAGETYPE1DARRAY
        VK_IMAGE_TYPE_2D, // RENDERER_IMAGETYPE2DARRAY
        VK_IMAGE_TYPE_2D // RENDERER_IMAGETYPECUBEARRAY
    };

    static VkImageViewType imageviewtypetable[] = {
        VK_IMAGE_VIEW_TYPE_1D, // RENDERER_IMAGETYPE1D
        VK_IMAGE_VIEW_TYPE_2D, // RENDERER_IMAGETYPE2D
        VK_IMAGE_VIEW_TYPE_3D, // RENDERER_IMAGETYPE3D
        VK_IMAGE_VIEW_TYPE_CUBE, // RENDERER_IMAGETYPECUBE
        VK_IMAGE_VIEW_TYPE_1D_ARRAY, // RENDERER_IMAGETYPE1DARRAY
        VK_IMAGE_VIEW_TYPE_2D_ARRAY, // RENDERER_IMAGETYPE2DARRAY
        VK_IMAGE_VIEW_TYPE_CUBE_ARRAY // RENDERER_IMAGETYPECUBEARRAY
    };

    static VkAttachmentLoadOp loadoptable[] = {
        VK_ATTACHMENT_LOAD_OP_LOAD,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    };

    static VkAttachmentStoreOp storeoptable[] = {
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE
    };

    static VkImageLayout layouttable[] = {
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    // vulkan vertex/instance layout attribute translation table
    static VkFormat attribtable[] = {
        VK_FORMAT_R32_SFLOAT, // RENDERER_VTXATTRIBFLOAT
        VK_FORMAT_R32G32_SFLOAT, // RENDERER_VTXATTRIBVEC2
        VK_FORMAT_R32G32B32_SFLOAT, // RENDERER_VTXATTRIBVEC3
        VK_FORMAT_R32G32B32A32_SFLOAT, // RENDERER_VTXATTRIBVEC4
        VK_FORMAT_R32G32_SINT, // RENDERER_VTXATTRIBIVEC2
        VK_FORMAT_R32G32B32_SINT, // RENDERER_VTXATTRIBIVEC3
        VK_FORMAT_R32G32B32A32_SINT, // RENDERER_VTXATTRIBIVEC4
        VK_FORMAT_R32G32_UINT, // RENDERER_VTXATTRIBUVEC2
        VK_FORMAT_R32G32B32_UINT, // RENDERER_VTXATTRIBUVEC3
        VK_FORMAT_R32G32B32A32_UINT // RENDERER_VTXATTRIBUVEC4
    };

    static VkSampleCountFlagBits sampletable[] = {
        VK_SAMPLE_COUNT_1_BIT,
        VK_SAMPLE_COUNT_2_BIT,
        VK_SAMPLE_COUNT_4_BIT,
        VK_SAMPLE_COUNT_8_BIT,
        VK_SAMPLE_COUNT_16_BIT,
        VK_SAMPLE_COUNT_32_BIT,
        VK_SAMPLE_COUNT_64_BIT
    };

    static VkPrimitiveTopology topologytable[] = {
        VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
        VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
        VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
        VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
        VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY,
        VK_PRIMITIVE_TOPOLOGY_PATCH_LIST
    };

    static VkPolygonMode polygonmodetable[] = {
        VK_POLYGON_MODE_FILL,
        VK_POLYGON_MODE_LINE,
        VK_POLYGON_MODE_POINT
    };

    static VkCullModeFlags cullmodetable[] = {
        VK_CULL_MODE_NONE,
        VK_CULL_MODE_BACK_BIT,
        VK_CULL_MODE_FRONT_BIT,
        VK_CULL_MODE_FRONT_AND_BACK
    };

    static VkFrontFace frontfacetable[] = {
        VK_FRONT_FACE_CLOCKWISE,
        VK_FRONT_FACE_COUNTER_CLOCKWISE
    };

    static VkBlendFactor blendfactortable[] = {
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_SRC_COLOR,
        VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
        VK_BLEND_FACTOR_DST_COLOR,
        VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
        VK_BLEND_FACTOR_SRC_ALPHA,
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        VK_BLEND_FACTOR_DST_ALPHA,
        VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
        VK_BLEND_FACTOR_CONSTANT_COLOR,
        VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
        VK_BLEND_FACTOR_CONSTANT_ALPHA,
        VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
        VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
        VK_BLEND_FACTOR_SRC1_COLOR,
        VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
        VK_BLEND_FACTOR_SRC1_ALPHA,
        VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA
    };

    static VkBlendOp blendoptable[] = {
        VK_BLEND_OP_ADD,
        VK_BLEND_OP_SUBTRACT,
        VK_BLEND_OP_REVERSE_SUBTRACT,
        VK_BLEND_OP_MIN,
        VK_BLEND_OP_MAX
    };

    static VkLogicOp logicoptable[] = {
        VK_LOGIC_OP_CLEAR,
        VK_LOGIC_OP_AND,
        VK_LOGIC_OP_AND_REVERSE,
        VK_LOGIC_OP_COPY,
        VK_LOGIC_OP_AND_INVERTED,
        VK_LOGIC_OP_NO_OP,
        VK_LOGIC_OP_XOR,
        VK_LOGIC_OP_OR,
        VK_LOGIC_OP_NOR,
        VK_LOGIC_OP_EQUIVALENT,
        VK_LOGIC_OP_INVERT,
        VK_LOGIC_OP_OR_REVERSE,
        VK_LOGIC_OP_COPY_INVERTED,
        VK_LOGIC_OP_OR_INVERTED,
        VK_LOGIC_OP_NAND,
        VK_LOGIC_OP_SET
    };

    static VkCompareOp compareoptable[] = {
        VK_COMPARE_OP_NEVER,
        VK_COMPARE_OP_LESS,
        VK_COMPARE_OP_EQUAL,
        VK_COMPARE_OP_LESS_OR_EQUAL,
        VK_COMPARE_OP_GREATER,
        VK_COMPARE_OP_NOT_EQUAL,
        VK_COMPARE_OP_GREATER_OR_EQUAL,
        VK_COMPARE_OP_ALWAYS
    };

    static VkFilter filtertable[] = {
        VK_FILTER_NEAREST,
        VK_FILTER_LINEAR
    };

    static VkSamplerAddressMode addrmodetable[] = {
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE
    };

    static VkSamplerMipmapMode mipmodetable[] = {
        VK_SAMPLER_MIPMAP_MODE_NEAREST,
        VK_SAMPLER_MIPMAP_MODE_LINEAR
    };

    static VkStencilOp stenciloptable[] = {
        VK_STENCIL_OP_KEEP, // RENDERER_STENCILKEEP
        VK_STENCIL_OP_ZERO, // RENDERER_STENCILZERO
        VK_STENCIL_OP_REPLACE, // RENDERER_STENCILREPLACE
        VK_STENCIL_OP_INCREMENT_AND_CLAMP, // RENDERER_STENCILINCCLAMP
        VK_STENCIL_OP_DECREMENT_AND_CLAMP, // RENDERER_STENCILDECCLAMP
        VK_STENCIL_OP_INVERT, // RENDERER_STENCILINV
        VK_STENCIL_OP_INCREMENT_AND_WRAP, // RENDERER_STENCILINCWRAP
        VK_STENCIL_OP_DECREMENT_AND_WRAP // RENDERER_STENILDECWRAP
    };

    static VkShaderStageFlagBits shaderstagetable[] = {
        VK_SHADER_STAGE_VERTEX_BIT, // RENDERER_SHADERVERTEX
        VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, // RENDERER_SHADERTESSCONTROL
        VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, // RENDERER_SHADERTESSEVAL
        VK_SHADER_STAGE_GEOMETRY_BIT, // RENDERER_SHADERGEOMETRY
        VK_SHADER_STAGE_FRAGMENT_BIT, // RENDERER_SHADERFRAGMENT
        VK_SHADER_STAGE_COMPUTE_BIT // RENDERER_SHADERCOMPUTE
    };

    static VkDescriptorType descriptortypetable[] = {
        VK_DESCRIPTOR_TYPE_SAMPLER, // RENDERER_RESOURCESAMPLER
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, // RENDERER_RESOURCETEXTURE
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, // RENDERER_RESOURCESTORAGETEXTURE
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // RENDERER_RESOURCEUNIFORM
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER // RENDERER_RESOURCESTORAGE
    };

    static VkVertexInputRate vulkan_inputratetable[] = {
        VK_VERTEX_INPUT_RATE_VERTEX, // RENDERER_RATEVERTEX
        VK_VERTEX_INPUT_RATE_INSTANCE // RENDERER_RATEINSTANCE
    };

    VkCommandBuffer VulkanContext::beginimmediate(void) {
        // TODO: Consider another locking for this to prevent overwriting the immediate work of another thread.
        VkResult res = vkResetFences(this->dev, 1, &this->imfence);
        ASSERT(res == VK_SUCCESS, "Failed to reset fences %d.\n", res);
        res = vkResetCommandBuffer(this->imcmd, 0);
        ASSERT(res == VK_SUCCESS, "Failed to reset command buffer %d.\n", res);

        VkCommandBufferBeginInfo begininfo = { };
        begininfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begininfo.pNext = NULL;
        begininfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        res = vkBeginCommandBuffer(this->imcmd, &begininfo);
        ASSERT(res == VK_SUCCESS, "Failed to begin Vulkan command buffer for immediate submission.\n");
        return this->imcmd; // Easier if we just drop the access directly so code can alias the immediate buffer.
    }

    void VulkanContext::endimmediate(void) {
        ZoneScoped;
        vkEndCommandBuffer(this->imcmd);

        VkSubmitInfo submitinfo = { };
        submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitinfo.pNext = NULL;
        submitinfo.commandBufferCount = 1;
        submitinfo.pCommandBuffers = &this->imcmd;

        vkQueueSubmit(this->graphicsqueue, 1, &submitinfo, this->imfence);
        // vkQueueWaitIdle(this->graphicsqueue); // Force the CPU to wait on the GPU being done with our work.
        vkWaitForFences(this->dev, 1, &this->imfence, VK_TRUE, UINT64_MAX); // Wait for the completion of this buffer before handing the control back to the CPU.
    }

    VkPipelineStageFlags pipelinestageflags(size_t srcstage) {
        return 0 |
            (srcstage & ORenderer::PIPELINE_STAGEALL ? VK_PIPELINE_STAGE_ALL_COMMANDS_BIT : 0) |
            (srcstage & ORenderer::PIPELINE_STAGEVERTEXSHADER ? VK_PIPELINE_STAGE_VERTEX_SHADER_BIT : 0) |
            (srcstage & ORenderer::PIPELINE_STAGEVERTEXINPUT ? VK_PIPELINE_STAGE_VERTEX_INPUT_BIT : 0) |
            (srcstage & ORenderer::PIPELINE_STAGETESSCONTROL ? VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT : 0) |
            (srcstage & ORenderer::PIPELINE_STAGETESSEVALUATION ? VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT : 0) |
            (srcstage & ORenderer::PIPELINE_STAGEGEOMETRY ? VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT : 0) |
            (srcstage & ORenderer::PIPELINE_STAGEFRAGMENT ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : 0) |
            (srcstage & ORenderer::PIPELINE_STAGECOMPUTE ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : 0) |
            (srcstage & ORenderer::PIPELINE_STAGETRANSFER ? VK_PIPELINE_STAGE_TRANSFER_BIT : 0) |
            (srcstage & ORenderer::PIPELINE_STAGEHOST ? VK_PIPELINE_STAGE_HOST_BIT : 0) |
            (srcstage & ORenderer::PIPELINE_STAGEEARLYFRAG ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT : 0) |
            (srcstage & ORenderer::PIPELINE_STAGELATEFRAG ? VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT : 0) |
            (srcstage & ORenderer::PIPELINE_STAGETOP ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : 0) |
            (srcstage & ORenderer::PIPELINE_STAGEBOTTOM ? VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT : 0);
    }

    VkAccessFlags accessflags(size_t srcflags) {
        return 0 |
            (srcflags & ORenderer::ACCESS_MEMREAD ? VK_ACCESS_MEMORY_READ_BIT : 0) |
            (srcflags & ORenderer::ACCESS_MEMWRITE ? VK_ACCESS_MEMORY_WRITE_BIT : 0) |
            (srcflags & ORenderer::ACCESS_COLOURREAD ? VK_ACCESS_COLOR_ATTACHMENT_READ_BIT : 0) |
            (srcflags & ORenderer::ACCESS_COLOURWRITE ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : 0) |
            (srcflags & ORenderer::ACCESS_DEPTHREAD ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT : 0) |
            (srcflags & ORenderer::ACCESS_DEPTHWRITE ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0) |
            (srcflags & ORenderer::ACCESS_TRANSFERREAD ? VK_ACCESS_TRANSFER_READ_BIT : 0) |
            (srcflags & ORenderer::ACCESS_TRANSFERWRITE ? VK_ACCESS_TRANSFER_WRITE_BIT : 0) |
            (srcflags & ORenderer::ACCESS_HOSTREAD ? VK_ACCESS_HOST_READ_BIT : 0) |
            (srcflags & ORenderer::ACCESS_HOSTWRITE ? VK_ACCESS_HOST_WRITE_BIT : 0) |
            (srcflags & ORenderer::ACCESS_SHADERREAD ? VK_ACCESS_SHADER_READ_BIT : 0) |
            (srcflags & ORenderer::ACCESS_SHADERWRITE ? VK_ACCESS_SHADER_WRITE_BIT : 0) |
            (srcflags & ORenderer::ACCESS_INPUTREAD ? VK_ACCESS_INPUT_ATTACHMENT_READ_BIT : 0);
    }

    static uint32_t convertqueue(ORenderer::RendererContext *ctx, uint8_t type) {
        const VulkanContext *vkctx = (VulkanContext *)ctx;
        return
            type == UINT8_MAX ? VK_QUEUE_FAMILY_IGNORED :
            type == ORenderer::STREAM_TRANSFER ? vkctx->transferfamily :
            type == ORenderer::STREAM_COMPUTE ? vkctx->computefamily :
            vkctx->initialfamily;
    }

    static void barrier(VkCommandBuffer cmd, struct texture *texture, size_t format, size_t oldlayout, size_t newlayout, size_t srcstage, size_t dststage, size_t srcaccess, size_t dstaccess, uint8_t srcqueue, uint8_t dstqueue, size_t basemip, size_t mipcount, size_t baselayer, size_t layercount) {
        VkImageMemoryBarrier barrier = { };
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = NULL;
        barrier.oldLayout = layouttable[oldlayout];
        barrier.newLayout = layouttable[newlayout];
        barrier.srcQueueFamilyIndex = convertqueue(ORenderer::context, srcqueue);
        barrier.dstQueueFamilyIndex = convertqueue(ORenderer::context, dstqueue);
        barrier.image = texture->image;
        barrier.subresourceRange.baseMipLevel = basemip;
        barrier.subresourceRange.levelCount = mipcount != SIZE_MAX ? mipcount : VK_REMAINING_MIP_LEVELS;
        barrier.subresourceRange.baseArrayLayer = baselayer;
        barrier.subresourceRange.layerCount = layercount != SIZE_MAX ? layercount : VK_REMAINING_ARRAY_LAYERS;
        barrier.subresourceRange.aspectMask =
            // colour format
            (ORenderer::iscolourformat(format) ? VK_IMAGE_ASPECT_COLOR_BIT : 0) |
            // depth format
            (ORenderer::isdepthformat(format) ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
            // depth format with stencil
            (ORenderer::isstencilformat(format) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);

        VkPipelineStageFlags srcstagemask = pipelinestageflags(srcstage);
        VkPipelineStageFlags dststagemask = pipelinestageflags(dststage);

        barrier.srcAccessMask = accessflags(srcaccess);
        barrier.dstAccessMask = accessflags(dstaccess);

        vkCmdPipelineBarrier(cmd, srcstagemask, dststagemask, 0, 0, NULL, 0, NULL, 1, &barrier);
    }

    static void transitionlayout(VkCommandBuffer cmd, struct texture *texture, size_t format, size_t state, size_t basemip, size_t mipcount, size_t baselayer, size_t layercount) {
        VkImageMemoryBarrier barrier = { };
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = NULL;
        barrier.oldLayout = texture->state;
        barrier.newLayout = layouttable[state];
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = texture->image;
        barrier.subresourceRange.baseMipLevel = basemip;
        barrier.subresourceRange.levelCount = mipcount != SIZE_MAX ? mipcount : VK_REMAINING_MIP_LEVELS;
        barrier.subresourceRange.baseArrayLayer = baselayer;
        barrier.subresourceRange.layerCount = layercount != SIZE_MAX ? layercount : VK_REMAINING_ARRAY_LAYERS;
        barrier.subresourceRange.aspectMask =
            // colour format
            (ORenderer::iscolourformat(format) ? VK_IMAGE_ASPECT_COLOR_BIT : 0) |
            // depth format
            (ORenderer::isdepthformat(format) ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
            // depth format with stencil
            (ORenderer::isstencilformat(format) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);

        VkPipelineStageFlags srcstagemask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dststagemask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        // anything that is written needs a write access mask, anything read doesn't matter
        VkAccessFlags srcaccess = 0;
        VkAccessFlags dstaccess = 0;

        VkImageLayout oldlayout = texture->state;
        VkImageLayout newlayout = layouttable[state];

        const VkPipelineStageFlags shaderstagemask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        const VkPipelineStageFlags depthstagemask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

        switch (oldlayout) {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                break;
            case VK_IMAGE_LAYOUT_GENERAL:
                srcstagemask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                srcaccess = VK_ACCESS_MEMORY_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                srcstagemask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                srcaccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                srcstagemask = depthstagemask;
                srcaccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
                srcstagemask = depthstagemask | shaderstagemask;
                break;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                srcstagemask = shaderstagemask;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                srcstagemask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                srcstagemask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                srcaccess = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_PREINITIALIZED:
                srcstagemask = VK_PIPELINE_STAGE_HOST_BIT;
                srcaccess = VK_ACCESS_HOST_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                break;
            default:
                ASSERT(false, "Unknown Vulkan texture layout.\n");
                break;
        }

        switch (newlayout) {
            case VK_IMAGE_LAYOUT_GENERAL:
                dststagemask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                dstaccess = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                dststagemask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dstaccess = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                dststagemask = depthstagemask;
                dstaccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
                dststagemask = depthstagemask | shaderstagemask;
                // can be used for a depth attachment, shader sampled image, or input attachment
                dstaccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
                break;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                dststagemask = shaderstagemask;
                // can be used for shader sampled image, or input attachment
                dstaccess = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                dststagemask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                dstaccess = VK_ACCESS_TRANSFER_READ_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                dststagemask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                dstaccess = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                break;
            default:
                ASSERT(false, "Unknown Vulkan texture layout.\n");
                break;
        }
        // Remember to set avvess masks.
        barrier.srcAccessMask = srcaccess;
        barrier.dstAccessMask = dstaccess;

        vkCmdPipelineBarrier(cmd, srcstagemask, dststagemask, 0, 0, NULL, 0, NULL, 1, &barrier);
        texture->state = layouttable[state];
    }

    uint8_t VulkanContext::transitionlayout(struct ORenderer::texture texture, size_t format, size_t state) {
        VkCommandBuffer cmd = this->beginimmediate();

        resourcemutex.lock();
        struct texture *vktexture = &textures[texture.handle].vkresource;
        OVulkan::transitionlayout(cmd, vktexture, format, state, 0, SIZE_MAX, 0, SIZE_MAX);
        resourcemutex.unlock();

        this->endimmediate();
        return ORenderer::RESULT_SUCCESS;
    }

    uint8_t VulkanContext::createfence(struct ORenderer::fence *fence) {
        ASSERT(fence != NULL, "Fence must not be NULL.\n");
        VkFenceCreateInfo info = { };
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.pNext = NULL;
        info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (fence->handle == RENDERER_INVALIDHANDLE) {
            this->resourcemutex.lock();
            fence->handle = this->resourcehandle++;
            this->fences[fence->handle].vkresource = (struct fence) { };
            this->resourcemutex.unlock();
        }
        this->resourcemutex.lock();
        struct fence *vkfence = &this->fences[fence->handle].vkresource;

        VkResult res = vkCreateFence(this->dev, &info, NULL, &vkfence->fence);
        this->resourcemutex.unlock();

        return ORenderer::RESULT_SUCCESS;
    }

    uint8_t VulkanContext::createsemaphore(struct ORenderer::semaphore *semaphore) {
        ASSERT(semaphore != NULL, "Semaphore must not be NULL.\n");
        VkSemaphoreCreateInfo info = { };
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        info.pNext = NULL;
        info.flags = 0;

        if (semaphore->handle == RENDERER_INVALIDHANDLE) {
            this->resourcemutex.lock();
            semaphore->handle = this->resourcehandle++;
            this->semaphores[semaphore->handle].vkresource = (struct semaphore) { };
            this->resourcemutex.unlock();
        }
        this->resourcemutex.lock();
        struct semaphore *vksemaphore = &this->semaphores[semaphore->handle].vkresource;

        VkResult res = vkCreateSemaphore(this->dev, &info, NULL, &vksemaphore->semaphore);
        this->resourcemutex.unlock();

        return ORenderer::RESULT_SUCCESS;

    }

    uint8_t VulkanContext::createtexture(struct ORenderer::texturedesc *desc, struct ORenderer::texture *texture) {
        ASSERT(desc != NULL, "Description must not be NULL.\n");
        ASSERT(texture != NULL, "Texture must not be NULL.\n");
        ASSERT(desc->format < ORenderer::FORMAT_COUNT, "Invalid texture format.\n");
        ASSERT(desc->type < ORenderer::IMAGETYPE_COUNT, "Invalid texture type.\n");

        VkImageUsageFlags usage =
            (desc->usage & ORenderer::USAGE_COLOUR ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0) |
            (desc->usage & ORenderer::USAGE_DEPTHSTENCIL ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : 0) |
            (desc->usage & ORenderer::USAGE_SAMPLED ? VK_IMAGE_USAGE_SAMPLED_BIT : 0) |
            (desc->usage & ORenderer::USAGE_STORAGE ? VK_IMAGE_USAGE_STORAGE_BIT : 0) |
            (desc->usage & ORenderer::USAGE_SRC ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0) |
            (desc->usage & ORenderer::USAGE_DST ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0) |
            0;

        if (texture->handle == RENDERER_INVALIDHANDLE) {
            this->resourcemutex.lock();
            texture->handle = this->resourcehandle++;
            this->textures[texture->handle].vkresource = (struct texture) { };
            this->resourcemutex.unlock();
        }
        this->resourcemutex.lock();
        struct texture *vktexture = &this->textures[texture->handle].vkresource;

        VkImageCreateInfo imagecreate = { };
        imagecreate.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imagecreate.pNext = NULL;
        imagecreate.imageType = imagetypetable[desc->type];
        imagecreate.extent.width = desc->width;
        imagecreate.extent.height = desc->height;
        imagecreate.extent.depth = desc->depth;
        imagecreate.mipLevels = desc->mips;
        imagecreate.arrayLayers = desc->layers;
        imagecreate.format = formattable[desc->format];
        imagecreate.tiling = desc->memlayout == ORenderer::MEMLAYOUT_LINEAR ?
            VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
        imagecreate.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imagecreate.usage = usage;
        imagecreate.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imagecreate.samples = VK_SAMPLE_COUNT_1_BIT;
        imagecreate.flags = desc->type == ORenderer::IMAGETYPE_CUBE ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

        VmaAllocationCreateInfo allocinfo = { };
        allocinfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        allocinfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        VmaAllocation alloc = NULL;
        VmaAllocationInfo info = { };
        VkResult res = vmaCreateImage(this->allocator, &imagecreate, &allocinfo, &vktexture->image, &alloc, &info);
        ASSERT(alloc != NULL, "Failed to create VMA allocation for texture %d.\n", res);

        vktexture->offset = info.offset;
        vktexture->memory = info.deviceMemory;
        vktexture->allocation = alloc;

        this->resourcemutex.unlock();

        return ORenderer::RESULT_SUCCESS;
    }

    uint8_t VulkanContext::createbuffer(struct ORenderer::bufferdesc *desc, struct ORenderer::buffer *buffer) {
        ASSERT(desc != NULL, "Description must not be NULL.\n");
        ASSERT(buffer != NULL, "Buffer must not be NULL.\n");
        ASSERT(desc->size > 0, "Invalid buffer size.\n");

        VkBufferCreateInfo buffercreate = { };
        buffercreate.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffercreate.pNext = NULL;
        buffercreate.size = desc->size;
        buffercreate.usage =
            (desc->usage & ORenderer::BUFFER_VERTEX ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : 0) |
            (desc->usage & ORenderer::BUFFER_INDEX ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0) |
            (desc->usage & ORenderer::BUFFER_STORAGE ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0) |
            (desc->usage & ORenderer::BUFFER_UNIFORM ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : 0) |
            (desc->usage & ORenderer::BUFFER_INDIRECT ? VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT : 0) |
            (desc->usage & ORenderer::BUFFER_TRANSFERDST ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0) |
            (desc->usage & ORenderer::BUFFER_TRANSFERSRC ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : 0) |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT; // We want to be able to use it for this regardless.
        buffercreate.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocinfo = { };
        allocinfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocinfo.flags =
            (desc->memprops & ORenderer::MEMPROP_CPURANDOMACCESS ? VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT : 0) |
            (desc->memprops & ORenderer::MEMPROP_CPUSEQUENTIALWRITE ? VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT : 0) |
            0;
        allocinfo.requiredFlags =
            (desc->memprops & ORenderer::MEMPROP_GPULOCAL ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0) |
            (desc->memprops & ORenderer::MEMPROP_CPUCACHED ? VK_MEMORY_PROPERTY_HOST_CACHED_BIT : 0) |
            (desc->memprops & ORenderer::MEMPROP_CPUVISIBLE ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 0) |
            (desc->memprops & ORenderer::MEMPROP_CPUCOHERENT ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0) |
            (desc->memprops & ORenderer::MEMPROP_PROTECTED ? VK_MEMORY_PROPERTY_PROTECTED_BIT : 0) |
            (desc->memprops & ORenderer::MEMPROP_LAZYALLOC ? VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT : 0) |
            0;

        if (buffer->handle == RENDERER_INVALIDHANDLE) {
            this->resourcemutex.lock();
            buffer->handle = this->resourcehandle++;
            this->buffers[buffer->handle].vkresource = (struct buffer) { };
            this->resourcemutex.unlock();
        }
        this->resourcemutex.lock();
        struct buffer *vkbuffer = &this->buffers[buffer->handle].vkresource;
        vkbuffer->flags = desc->flags;
        if (!(desc->flags & ORenderer::BUFFERFLAG_PERFRAME)) {
            VmaAllocation alloc;
            VkResult res = vmaCreateBuffer(this->allocator, &buffercreate, &allocinfo, &vkbuffer->buffer[0], &alloc, NULL);
            ASSERT(res == VK_SUCCESS, "Failed to allocate Vulkan buffer %d.\n", res);
            VmaAllocationInfo info;
            vmaGetAllocationInfo(this->allocator, alloc, &info);
            vkbuffer->memory[0] = info.deviceMemory;
            vkbuffer->offset[0] = info.offset;
            vkbuffer->allocation[0] = alloc;
        } else {
            for (size_t i = 0; i < RENDERER_MAXLATENCY; i++) {
                VmaAllocation alloc;
                VkResult res = vmaCreateBuffer(this->allocator, &buffercreate, &allocinfo, &vkbuffer->buffer[i], &alloc, NULL);
                ASSERT(res == VK_SUCCESS, "Failed to allocate Vulkan buffer %d.\n", res);
                VmaAllocationInfo info;
                vmaGetAllocationInfo(this->allocator, alloc, &info);
                vkbuffer->memory[i] = info.deviceMemory;
                vkbuffer->offset[i] = info.offset;
                vkbuffer->allocation[i] = alloc;
            }
        }
        this->resourcemutex.unlock();

        return ORenderer::RESULT_SUCCESS;
    }

    uint8_t VulkanContext::createframebuffer(struct ORenderer::framebufferdesc *desc, struct ORenderer::framebuffer *framebuffer) {
        ASSERT(desc != NULL, "Description must not be NULL.\n");
        ASSERT(framebuffer != NULL, "framebuffer must not be NULL.\n");
        ASSERT(desc->attachmentcount > 0, "Invalid number of attachments.\n");
        ASSERT(desc->attachments != NULL, "Invalid attachments.\n");
        ASSERT(desc->width > 0, "Invalid framebuffer width.\n");
        ASSERT(desc->height > 0, "Invalid framebuffer height.\n");
        ASSERT(desc->layers > 0, "Invalid framebuffer layer count.\n");

        VkImageView *attachments = (VkImageView *)malloc(sizeof(VkImageView) * desc->attachmentcount);
        ASSERT(attachments != NULL, "Failed to allocate memory for Vulkan framebuffer attachments.\n");
        for (size_t i = 0; i < desc->attachmentcount; i++) {
            attachments[i] = this->textureviews[desc->attachments[i].handle].vkresource.imageview;
        }

        if (framebuffer->handle == RENDERER_INVALIDHANDLE) {
            this->resourcemutex.lock();
            framebuffer->handle = this->resourcehandle++;
            this->framebuffers[framebuffer->handle].vkresource = (struct framebuffer) { };
            this->resourcemutex.unlock();
        }
        this->resourcemutex.lock();
        VkFramebufferCreateInfo fbcreate = { };
        fbcreate.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbcreate.renderPass = this->renderpasses[desc->pass.handle].vkresource.renderpass;
        fbcreate.attachmentCount = desc->attachmentcount;
        fbcreate.pAttachments = attachments;
        fbcreate.width = desc->width;
        fbcreate.height = desc->height;
        fbcreate.layers = desc->layers;
        VkResult res = vkCreateFramebuffer(this->dev, &fbcreate, NULL, &this->framebuffers[framebuffer->handle].vkresource.framebuffer);
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan framebuffer %d.\n", res);
        free(attachments);
        this->resourcemutex.unlock();

        return ORenderer::RESULT_SUCCESS;
    }

    uint8_t VulkanContext::createrenderpass(struct ORenderer::renderpassdesc *desc, struct ORenderer::renderpass *pass) {
        ASSERT(desc != NULL, "Description must not be NULL.\n");
        ASSERT(pass != NULL, "Pass must not be NULL.\n");
        ASSERT(desc->attachmentcount > 0, "Invalid number of attachments.\n");
        ASSERT(desc->attachments != NULL, "Invalid attachments.\n");
        ASSERT(desc->depthref == NULL || desc->colourrefcount > 0, "No depth reference yet colour reference count is invalid.\n");
        if (desc->colourrefs == NULL && desc->colourrefcount > 0) {
            ASSERT(false, "Told there are colour references despite having none specified.\n");
        }

        VkAttachmentReference *colourrefs = (VkAttachmentReference *)malloc(sizeof(VkAttachmentReference) * (desc->colourrefcount ? desc->colourrefcount : 1));
        ASSERT(colourrefs != NULL, "Failed to allocate memory for Vulkan renderpass colour references.\n");
        memset(colourrefs, 0, sizeof(VkAttachmentReference) * (desc->colourrefcount ? desc->colourrefcount : 1));

        VkAttachmentDescription *attachments = (VkAttachmentDescription *)malloc(sizeof(VkAttachmentDescription) * desc->attachmentcount);
        ASSERT(attachments != NULL, "Failed to allocate memory for Vulkan renderpass attachments.\n");
        memset(attachments, 0, sizeof(VkAttachmentDescription) * desc->attachmentcount);

        for (size_t i = 0; i < desc->attachmentcount; i++) {
            attachments[i].format = formattable[desc->attachments[i].format];
            attachments[i].samples = sampletable[desc->attachments[i].samples];
            attachments[i].loadOp = loadoptable[desc->attachments[i].loadop];
            attachments[i].storeOp = storeoptable[desc->attachments[i].storeop];
            attachments[i].stencilLoadOp = loadoptable[desc->attachments[i].stencilloadop];
            attachments[i].stencilStoreOp = storeoptable[desc->attachments[i].stencilstoreop];
            attachments[i].initialLayout = layouttable[desc->attachments[i].initiallayout];
            attachments[i].finalLayout = layouttable[desc->attachments[i].finallayout];
        }

        for (size_t i = 0; i < desc->colourrefcount; i++) {
            colourrefs[i].attachment = desc->colourrefs[i].attachment;
            colourrefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        VkAttachmentReference depthref = { };
        if (desc->depthref != NULL) {
            if (!ORenderer::isdepthformat(desc->attachments[desc->depthref->attachment].format)) {
                free(attachments);
                free(colourrefs);
                return ORenderer::RESULT_INVALIDARG;
            }
            depthref.attachment = desc->depthref->attachment;
            depthref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }


        VkSubpassDescription subpass = { };
        subpass.colorAttachmentCount = desc->colourrefcount;
        subpass.pColorAttachments = colourrefs;
        if (desc->depthref != NULL) {
            subpass.pDepthStencilAttachment = &depthref;
        }

        VkRenderPassCreateInfo passcreate = { };
        passcreate.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        passcreate.pNext = NULL;
        passcreate.attachmentCount = desc->attachmentcount;
        passcreate.pAttachments = attachments;
        passcreate.subpassCount = 1;
        passcreate.pSubpasses = &subpass;

        const VkPipelineStageFlags graphics = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        const VkPipelineStageFlags outside = graphics | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;

        VkSubpassDependency dep[2] = { };
        dep[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dep[0].dstSubpass = 0;
        dep[0].srcStageMask = outside;
        dep[0].dstStageMask = graphics;
        dep[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        dep[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        dep[0].dependencyFlags = 0;

        dep[1].srcSubpass = 0;
        dep[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dep[1].srcStageMask = graphics;
        dep[1].dstStageMask = outside;
        dep[1].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        dep[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        dep[1].dependencyFlags = 0;

        passcreate.dependencyCount = 2;
        passcreate.pDependencies = dep;

        if (pass->handle == RENDERER_INVALIDHANDLE) {
            this->resourcemutex.lock();
            pass->handle = this->resourcehandle++;
            this->renderpasses[pass->handle].vkresource = (struct renderpass) { };
            resourcemutex.unlock();
        }
        this->resourcemutex.lock();
        VkResult res = vkCreateRenderPass(this->dev, &passcreate, NULL, &this->renderpasses[pass->handle].vkresource.renderpass);
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan renderpass %d.\n", res);
        free(colourrefs);
        free(attachments);
        this->resourcemutex.unlock();

        return ORenderer::RESULT_SUCCESS;
    }

    uint8_t VulkanContext::createcomputepipelinestate(struct ORenderer::computepipelinestatedesc *desc, struct ORenderer::pipelinestate *state) {
        ASSERT(desc != NULL, "Description must not be NULL.\n");
        ASSERT(state != NULL, "State must not be NULL.\n");
        ASSERT(desc->stage.type == ORenderer::SHADER_COMPUTE, "Using a non-compute shader inside a compute pipeline.\n");

        VkPipelineShaderStageCreateInfo stage = { };
        stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage.pNext = NULL;

        if (state->handle == RENDERER_INVALIDHANDLE) {
            this->resourcemutex.lock();
            state->handle = this->resourcehandle++;
            this->pipelinestates[state->handle].vkresource = (struct pipelinestate) { };
            this->resourcemutex.unlock();
        }


        stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stage.module = this->createshadermodule(desc->stage);
        stage.pName = "main";


        this->resourcemutex.lock();

        VkResult res;
        struct pipelinestate *vkstate = &this->pipelinestates[state->handle].vkresource;

        if (desc->reslayout != NULL) {
            vkstate->descsetlayout = this->layouts[desc->reslayout->handle].vkresource.layout;
        }


        VkPipelineLayoutCreateInfo pipelayoutcreate = { };
        pipelayoutcreate.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelayoutcreate.pNext = NULL;
        if (desc->reslayout != NULL) {
            pipelayoutcreate.setLayoutCount = 1;
            pipelayoutcreate.pSetLayouts = &vkstate->descsetlayout;
        } else {
            pipelayoutcreate.setLayoutCount = 0;
            pipelayoutcreate.pSetLayouts = NULL;
        }

        VkPushConstantRange range = { };
        if (desc->constantssize > 0) {
            pipelayoutcreate.pushConstantRangeCount = 1;

            range.offset = 0;
            range.size = desc->constantssize;
            range.stageFlags = VK_SHADER_STAGE_ALL;
            pipelayoutcreate.pPushConstantRanges = &range;
        } else {
            pipelayoutcreate.pPushConstantRanges = 0;
        }
        res = vkCreatePipelineLayout(this->dev, &pipelayoutcreate, NULL, &vkstate->pipelinelayout);
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan pipeline layout %d.\n", res);

        VkComputePipelineCreateInfo pipelinecreate = { };
        pipelinecreate.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelinecreate.pNext = NULL;
        pipelinecreate.stage = stage;
        pipelinecreate.layout = vkstate->pipelinelayout;

        vkstate->type = ORenderer::COMPUTEPIPELINE;
        vkstate->shaders[0] = stage.module;
        vkstate->shadercount = 1;

        res = vkCreateComputePipelines(this->dev, VK_NULL_HANDLE, 1, &pipelinecreate, NULL, &vkstate->pipeline);
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan compute pipeline %d.\n", res);

        this->resourcemutex.unlock();

        return ORenderer::RESULT_SUCCESS;
    }

    uint8_t VulkanContext::createpipelinestate(struct ORenderer::pipelinestatedesc *desc, struct ORenderer::pipelinestate *state) {
        ASSERT(desc != NULL, "Description must not be NULL.\n");
        ASSERT(state != NULL, "State must not be NULL.\n");
        ASSERT(desc->stagecount > 0, "Need at least one stage in a pipeline.\n");

        VkPipelineTessellationStateCreateInfo tesscreate = { };
        tesscreate.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
        tesscreate.pNext = NULL;
        tesscreate.patchControlPoints = desc->tesspoints;

        VkPipelineShaderStageCreateInfo *stages = (VkPipelineShaderStageCreateInfo *)malloc(sizeof(VkPipelineShaderStageCreateInfo) * desc->stagecount);
        ASSERT(stages != NULL, "Failed to allocate memory for Vulkan pipeline shader stages.\n");
        memset(stages, 0, sizeof(VkPipelineShaderStageCreateInfo) * desc->stagecount);
        for (size_t i = 0; i < desc->stagecount; i++) {
            stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stages[i].pNext = NULL;
            if (desc->stages[i].type == ORenderer::STAGE_COMPUTE) {
                free(stages);
                ASSERT(false, "Using a compute shader inside a normal pipeline.\n");
            }
            stages[i].stage = shaderstagetable[desc->stages[i].type];
            stages[i].module = this->createshadermodule(desc->stages[i]);
            stages[i].pName = "main";
        }

        if (state->handle == RENDERER_INVALIDHANDLE) {
            this->resourcemutex.lock();
            state->handle = this->resourcehandle++;
            this->pipelinestates[state->handle].vkresource = (struct pipelinestate) { };
            this->resourcemutex.unlock();
        }

        VkPipelineDynamicStateCreateInfo dynamiccreate = { };
        dynamiccreate.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamiccreate.pNext = NULL;
        VkDynamicState dynamicstates[] = {
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_VIEWPORT
        };
        dynamiccreate.dynamicStateCount = sizeof(dynamicstates) / sizeof(dynamicstates[0]);
        dynamiccreate.pDynamicStates = dynamicstates;

        VkVertexInputBindingDescription *binddescs = (VkVertexInputBindingDescription *)malloc(sizeof(VkVertexInputBindingDescription) * desc->vtxinput->bindcount);
        ASSERT(binddescs != NULL, "Failed to allocate memory for Vulkan vertex input bindings.\n");
        VkVertexInputAttributeDescription *attribdescs = (VkVertexInputAttributeDescription *)malloc(sizeof(VkVertexInputAttributeDescription));
        ASSERT(attribdescs != NULL, "Failed to allocate memory for Vulkan input attributes.\n");
        size_t attribs = 0;

        for (size_t i = 0; i < desc->vtxinput->bindcount; i++) {
            struct ORenderer::vtxbinddesc bind = desc->vtxinput->binddescs[i];
            binddescs[i].binding = bind.layout.binding;
            binddescs[i].stride = bind.layout.stride;
            binddescs[i].inputRate = vulkan_inputratetable[bind.rate];
            attribs += bind.layout.attribcount;
            attribdescs = (VkVertexInputAttributeDescription *)realloc(attribdescs, sizeof(VkVertexInputAttributeDescription) * attribs);
            ASSERT(attribdescs != NULL, "Failed to reallocate memory for vertex input attribute descriptions.\n");

            for (size_t j = 0; j < bind.layout.attribcount; j++) {
                attribdescs[j].binding = i;
                attribdescs[j].location = j;
                attribdescs[j].offset = bind.layout.attribs[j].offset;
                attribdescs[j].format = attribtable[bind.layout.attribs[j].attribtype];
            }
        }

        VkPipelineVertexInputStateCreateInfo vtxinputcreate = { };
        vtxinputcreate.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vtxinputcreate.pNext = NULL;
        vtxinputcreate.vertexBindingDescriptionCount = desc->vtxinput->bindcount;
        vtxinputcreate.vertexAttributeDescriptionCount = attribs;
        vtxinputcreate.pVertexBindingDescriptions = binddescs;
        vtxinputcreate.pVertexAttributeDescriptions = attribdescs;

        VkPipelineInputAssemblyStateCreateInfo assemblycreate = { };
        assemblycreate.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        assemblycreate.pNext = NULL;
        assemblycreate.topology = topologytable[desc->primtopology];
        assemblycreate.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportcreate = { };
        viewportcreate.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportcreate.pNext = NULL;
        viewportcreate.scissorCount = desc->scissorcount;
        viewportcreate.viewportCount = desc->viewportcount;

        VkPipelineRasterizationStateCreateInfo rasterisercreate = { };
        rasterisercreate.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterisercreate.pNext = NULL;
        rasterisercreate.depthClampEnable = desc->rasteriser->depthclamp ? VK_TRUE : VK_FALSE;
        rasterisercreate.rasterizerDiscardEnable = desc->rasteriser->rasteriserdiscard ? VK_TRUE : VK_FALSE;
        rasterisercreate.polygonMode = polygonmodetable[desc->rasteriser->polygonmode];
        rasterisercreate.lineWidth = 1.0f;
        rasterisercreate.cullMode = cullmodetable[desc->rasteriser->cullmode];
        rasterisercreate.frontFace = frontfacetable[desc->rasteriser->frontface];
        rasterisercreate.depthBiasEnable = desc->rasteriser->depthbias ? VK_TRUE : VK_FALSE;
        rasterisercreate.depthBiasConstantFactor = desc->rasteriser->depthbiasconstant;
        rasterisercreate.depthBiasClamp = desc->rasteriser->depthbiasclamp;
        rasterisercreate.depthBiasSlopeFactor = desc->rasteriser->depthbiasslope;

        VkPipelineMultisampleStateCreateInfo mscreate = { };
        mscreate.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        mscreate.pNext = NULL;
        mscreate.sampleShadingEnable = desc->multisample->sampleshading ? VK_TRUE : VK_FALSE;
        mscreate.rasterizationSamples = sampletable[desc->multisample->samples];
        mscreate.minSampleShading = desc->multisample->minsampleshading;
        mscreate.pSampleMask = desc->multisample->samplemask;
        mscreate.alphaToCoverageEnable = desc->multisample->alphatocoverage ? VK_TRUE : VK_FALSE;
        mscreate.alphaToOneEnable = desc->multisample->alphatoone ? VK_TRUE : VK_FALSE;

        VkPipelineColorBlendAttachmentState *colourblendattachments = (VkPipelineColorBlendAttachmentState *)malloc(sizeof(VkPipelineColorBlendAttachmentState) * desc->blendstate->attachmentcount);
        ASSERT(colourblendattachments, "Failed to allocate memory for Vulkan pipeline colour blend attachments.\n");
        for (size_t i = 0; i < desc->blendstate->attachmentcount; i++) {
            struct ORenderer::blendattachmentdesc *blend = &desc->blendstate->attachments[i];
            colourblendattachments[i].colorWriteMask =
                (blend->writemask & ORenderer::WRITE_R ? VK_COLOR_COMPONENT_R_BIT : 0) |
                (blend->writemask & ORenderer::WRITE_G ? VK_COLOR_COMPONENT_G_BIT : 0) |
                (blend->writemask & ORenderer::WRITE_B ? VK_COLOR_COMPONENT_B_BIT : 0) |
                (blend->writemask & ORenderer::WRITE_A ? VK_COLOR_COMPONENT_A_BIT : 0);
            colourblendattachments[i].blendEnable = blend->blend ? VK_TRUE : VK_FALSE;
            colourblendattachments[i].srcColorBlendFactor = blendfactortable[blend->srccolourblendfactor];
            colourblendattachments[i].dstColorBlendFactor = blendfactortable[blend->dstcolourblendfactor];
            colourblendattachments[i].colorBlendOp = blendoptable[blend->colourblendop];
            colourblendattachments[i].srcAlphaBlendFactor = blendfactortable[blend->srcalphablendfactor];
            colourblendattachments[i].dstAlphaBlendFactor = blendfactortable[blend->dstalphablendfactor];
            colourblendattachments[i].alphaBlendOp = blendoptable[blend->alphablendop];
        }

        VkPipelineColorBlendStateCreateInfo blendstatecreate = { };
        blendstatecreate.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blendstatecreate.pNext = NULL;
        blendstatecreate.logicOpEnable = desc->blendstate->logicopenable ? VK_TRUE : VK_FALSE;
        blendstatecreate.logicOp = logicoptable[desc->blendstate->logicop];
        blendstatecreate.attachmentCount = desc->blendstate->attachmentcount;
        blendstatecreate.pAttachments = colourblendattachments;

        VkPipelineDepthStencilStateCreateInfo depthstencilcreate = { };
        if (desc->depthstencil != NULL) {
            depthstencilcreate.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthstencilcreate.pNext = NULL;
            depthstencilcreate.depthTestEnable = desc->depthstencil->depthtest ? VK_TRUE : VK_FALSE;
            depthstencilcreate.depthWriteEnable = desc->depthstencil->depthwrite ? VK_TRUE : VK_FALSE;
            depthstencilcreate.depthCompareOp = compareoptable[desc->depthstencil->depthcompareop];
            depthstencilcreate.depthBoundsTestEnable = desc->depthstencil->depthboundstest ? VK_TRUE : VK_FALSE;
            depthstencilcreate.minDepthBounds = desc->depthstencil->mindepth;
            depthstencilcreate.maxDepthBounds = desc->depthstencil->maxdepth;
            depthstencilcreate.stencilTestEnable = desc->depthstencil->stencil ? VK_TRUE : VK_FALSE;
            depthstencilcreate.front = (VkStencilOpState) {
                .failOp = stenciloptable[desc->depthstencil->front.failop],
                .passOp = stenciloptable[desc->depthstencil->front.passop],
                .depthFailOp = stenciloptable[desc->depthstencil->front.depthfailop],
                .compareOp = compareoptable[desc->depthstencil->front.cmpop],
                .compareMask = desc->depthstencil->front.cmpmask,
                .writeMask = desc->depthstencil->front.writemask,
                .reference = desc->depthstencil->front.ref
            };
            depthstencilcreate.back = (VkStencilOpState) {
                .failOp = stenciloptable[desc->depthstencil->back.failop],
                .passOp = stenciloptable[desc->depthstencil->back.passop],
                .depthFailOp = stenciloptable[desc->depthstencil->back.depthfailop],
                .compareOp = compareoptable[desc->depthstencil->back.cmpop],
                .compareMask = desc->depthstencil->back.cmpmask,
                .writeMask = desc->depthstencil->back.writemask,
                .reference = desc->depthstencil->back.ref
            };
        }

        this->resourcemutex.lock();

        VkResult res;
        struct pipelinestate *vkstate = &this->pipelinestates[state->handle].vkresource;
        if (desc->reslayout != NULL) {
            vkstate->descsetlayout = this->layouts[desc->reslayout->handle].vkresource.layout;
        }

        VkPipelineLayoutCreateInfo pipelayoutcreate = { };
        pipelayoutcreate.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelayoutcreate.pNext = NULL;
        if (desc->reslayout != NULL) {
            pipelayoutcreate.setLayoutCount = 1;
            pipelayoutcreate.pSetLayouts = &vkstate->descsetlayout;
        } else {
            pipelayoutcreate.setLayoutCount = 0;
            pipelayoutcreate.pSetLayouts = NULL;
        }

        VkPushConstantRange range = { };
        if (desc->constantssize > 0) {
            pipelayoutcreate.pushConstantRangeCount = 1;

            range.offset = 0;
            range.size = desc->constantssize;
            range.stageFlags = VK_SHADER_STAGE_ALL;
            pipelayoutcreate.pPushConstantRanges = &range;
        } else {
            pipelayoutcreate.pPushConstantRanges = 0;
        }

        res = vkCreatePipelineLayout(this->dev, &pipelayoutcreate, NULL, &vkstate->pipelinelayout);
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan pipeline layout %d.\n", res);

        VkGraphicsPipelineCreateInfo pipelinecreate = { };
        pipelinecreate.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelinecreate.pNext = NULL;
        pipelinecreate.stageCount = desc->stagecount;
        pipelinecreate.pStages = stages;
        pipelinecreate.pVertexInputState = &vtxinputcreate;
        pipelinecreate.pInputAssemblyState = &assemblycreate;
        pipelinecreate.pViewportState = &viewportcreate;
        pipelinecreate.pRasterizationState = &rasterisercreate;
        pipelinecreate.pMultisampleState = &mscreate;
        pipelinecreate.pDepthStencilState = desc->depthstencil != NULL ? &depthstencilcreate : NULL;
        pipelinecreate.pColorBlendState = &blendstatecreate;
        pipelinecreate.pDynamicState = &dynamiccreate;
        pipelinecreate.pTessellationState = &tesscreate;
        pipelinecreate.layout = vkstate->pipelinelayout;
    std::vector<VkShaderModule> shaders;
        pipelinecreate.renderPass = this->renderpasses[desc->renderpass->handle].vkresource.renderpass;
        pipelinecreate.subpass = 0;

        vkstate->type = ORenderer::GRAPHICSPIPELINE;

        res = vkCreateGraphicsPipelines(this->dev, VK_NULL_HANDLE, 1, &pipelinecreate, NULL, &vkstate->pipeline);
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan graphics pipeline %d.\n", res);

        for (size_t i = 0; i < desc->stagecount; i++) {
            vkstate->shaders[i] = stages[i].module;
        }
        vkstate->shadercount = desc->stagecount;
        free(stages);
        free(binddescs);
        free(attribdescs);
        free(colourblendattachments);

        this->resourcemutex.unlock();

        return ORenderer::RESULT_SUCCESS;
    }

    uint8_t VulkanContext::createtextureview(struct ORenderer::textureviewdesc *desc, struct ORenderer::textureview *view) {
        ASSERT(desc != NULL, "Description must not be NULL.\n");
        ASSERT(view != NULL, "View must not be NULL.\n");
        ASSERT(desc->layercount > 0, "Invalid layer count.\n");
        ASSERT(desc->mipcount > 0, "Invalid mip level count.\n");
        ASSERT(desc->texture.handle != RENDERER_INVALIDHANDLE, "Invalid source texture.\n");

        if (view->handle == RENDERER_INVALIDHANDLE) {
            this->resourcemutex.lock();
            view->handle = this->resourcehandle++;
            this->textureviews[view->handle].vkresource = (struct textureview) { };
            this->resourcemutex.unlock();
        }

        this->resourcemutex.lock();

        struct textureview *vkview = &this->textureviews[view->handle].vkresource;

        VkImageViewCreateInfo viewcreate = { };
        viewcreate.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewcreate.image = this->textures[desc->texture.handle].vkresource.image;
        viewcreate.viewType = imageviewtypetable[desc->type];
        viewcreate.format = formattable[desc->format];
        viewcreate.subresourceRange.aspectMask = 0 |
            // colour format
            (ORenderer::iscolourformat(desc->format) ? VK_IMAGE_ASPECT_COLOR_BIT : 0) |
            // depth format
            (ORenderer::isdepthformat(desc->format) ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
            // depth format with stencil
            (ORenderer::isstencilformat(desc->format) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);

        viewcreate.subresourceRange.baseMipLevel = desc->basemiplevel;
        viewcreate.subresourceRange.levelCount = desc->mipcount;
        viewcreate.subresourceRange.baseArrayLayer = desc->baselayer;
        viewcreate.subresourceRange.layerCount = desc->layercount;
        VkResult res = vkCreateImageView(this->dev, &viewcreate, NULL, &vkview->imageview);
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan image view %d.\n", res);

        this->resourcemutex.unlock();

        return ORenderer::RESULT_SUCCESS;
    }

    uint8_t VulkanContext::createresourcesetlayout(struct ORenderer::resourcesetdesc *desc, struct ORenderer::resourcesetlayout *layout) {
        ASSERT(desc != NULL, "Description must not be NULL.\n");
        ASSERT(layout != NULL, "Layout must not be NULL.\n");
        ASSERT(desc->resources != NULL, "Invalid resources for resource set layout.\n");
        ASSERT(desc->resourcecount > 0, "No resources for resource set layout.\n");

        if (layout->handle == RENDERER_INVALIDHANDLE) {
            this->resourcemutex.lock();
            layout->handle = this->resourcehandle++;
            this->layouts[layout->handle].vkresource = (struct resourcesetlayout) { };
            this->resourcemutex.unlock();
        }
        VkDescriptorSetLayoutBinding *layoutbindings = (VkDescriptorSetLayoutBinding *)malloc(sizeof(VkDescriptorSetLayoutBinding) * desc->resourcecount);
        VkDescriptorBindingFlags *flagbindings = (VkDescriptorBindingFlags *)malloc(sizeof(VkDescriptorBindingFlags) * desc->resourcecount);
        ASSERT(layoutbindings != NULL, "Failed to allocate memory for Vulkan descriptor set layout bindings.\n");
        ASSERT(flagbindings != NULL, "Failed to allocate memory for Vulkan descriptor set flag bindings.\n");
        for (size_t i = 0; i < desc->resourcecount; i++) {
            struct ORenderer::resourcedesc *resource = &desc->resources[i];
            layoutbindings[i].binding = resource->binding;
            layoutbindings[i].descriptorCount = resource->count;
            layoutbindings[i].stageFlags =
                (resource->stages & ORenderer::STAGE_VERTEX ? VK_SHADER_STAGE_VERTEX_BIT : 0) |
                (resource->stages & ORenderer::STAGE_TESSCONTROL ? VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT : 0) |
                (resource->stages & ORenderer::STAGE_TESSEVALUATION ? VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT : 0) |
                (resource->stages & ORenderer::STAGE_GEOMETRY ? VK_SHADER_STAGE_GEOMETRY_BIT : 0) |
                (resource->stages & ORenderer::STAGE_FRAGMENT ? VK_SHADER_STAGE_FRAGMENT_BIT : 0);
            layoutbindings[i].pImmutableSamplers = NULL;
            layoutbindings[i].descriptorType = descriptortypetable[resource->type];

            flagbindings[i] = (resource->flag & ORenderer::RESOURCEFLAG_BINDLESS ? VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT : 0);
        }

        this->resourcemutex.lock();

        struct resourcesetlayout *vkstate = &this->layouts[layout->handle].vkresource;

        VkDescriptorSetLayoutBindingFlagsCreateInfo flags = { };
        flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        flags.pNext = NULL;
        flags.bindingCount = desc->resourcecount;
        flags.pBindingFlags = flagbindings;

        VkDescriptorSetLayoutCreateInfo layoutcreate = { };
        layoutcreate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutcreate.pNext = &flags;
        layoutcreate.flags = (desc->flag & ORenderer::RESOURCEFLAG_BINDLESS ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT : 0);
        layoutcreate.bindingCount = desc->resourcecount;
        layoutcreate.pBindings = layoutbindings;
        VkResult res = vkCreateDescriptorSetLayout(this->dev, &layoutcreate, NULL, &vkstate->layout);
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan descriptor set layout %d.\n", res);
        free(layoutbindings);
        free(flagbindings);
        this->resourcemutex.unlock();

        return ORenderer::RESULT_SUCCESS;
    }

    uint8_t VulkanContext::createresourceset(struct ORenderer::resourcesetlayout *layout, struct ORenderer::resourceset *set) {
        ASSERT(layout != NULL, "Layout must not be NULL.\n");
        ASSERT(set != NULL, "Set must not be NULL.\n");
        ASSERT(layout->handle != RENDERER_INVALIDHANDLE, "Invalid layout.\n");

        if (set->handle == RENDERER_INVALIDHANDLE) {
            this->resourcemutex.lock();
            set->handle = this->resourcehandle++;
            this->sets[set->handle].vkresource = (struct resourceset) { };
            this->resourcemutex.unlock();
        }


        this->resourcemutex.lock();
        VkDescriptorSetAllocateInfo info = { };
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        info.pNext = NULL;
        info.descriptorSetCount = 1;
        info.descriptorPool = this->descpool;
        info.pSetLayouts = &this->layouts[layout->handle].vkresource.layout;

        VkResult res = vkAllocateDescriptorSets(this->dev, &info, &this->sets[set->handle].vkresource.set);
        ASSERT(res == VK_SUCCESS, "Failed to allocate Vulkan descriptor set %d.\n", res);

        this->resourcemutex.unlock();
        return ORenderer::RESULT_SUCCESS;
    }

    uint8_t VulkanContext::createsampler(struct ORenderer::samplerdesc *desc, struct ORenderer::sampler *sampler) {
        ASSERT(desc != NULL, "Description must not be NULL.\n");
        ASSERT(sampler != NULL, "Sampler must not be NULL.\n");
        if (sampler->handle == RENDERER_INVALIDHANDLE) {
            this->resourcemutex.lock();
            sampler->handle = this->resourcehandle++;
            this->samplers[sampler->handle].vkresource = (struct sampler) { };
            this->resourcemutex.unlock();
        }

        this->resourcemutex.lock();

        struct sampler *vksampler = &this->samplers[sampler->handle].vkresource;

        VkSamplerCreateInfo samplercreate = { };
        samplercreate.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplercreate.magFilter = filtertable[desc->magfilter];
        samplercreate.minFilter = filtertable[desc->minfilter];
        samplercreate.mipmapMode = mipmodetable[desc->mipmode];
        samplercreate.addressModeU = addrmodetable[desc->addru];
        samplercreate.addressModeV = addrmodetable[desc->addrv];
        samplercreate.addressModeW = addrmodetable[desc->addrw];
        samplercreate.mipLodBias = desc->lodbias;
        samplercreate.anisotropyEnable = desc->anisotropyenable;
        samplercreate.maxAnisotropy = desc->maxanisotropy;
        samplercreate.compareEnable = desc->cmpenable;
        samplercreate.compareOp = compareoptable[desc->cmpop];
        samplercreate.minLod = desc->minlod;
        samplercreate.maxLod = desc->maxlod;
        samplercreate.unnormalizedCoordinates = desc->unnormalisedcoords;

        VkResult res = vkCreateSampler(this->dev, &samplercreate, NULL, &vksampler->sampler);
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan sampler %d.\n", res);

        this->resourcemutex.unlock();

        return ORenderer::RESULT_SUCCESS;
    }

    uint8_t VulkanContext::mapbuffer(struct ORenderer::buffermapdesc *desc, struct ORenderer::buffermap *map) {
        ASSERT(desc != NULL, "Description must not be NULL.\n");
        ASSERT(map != NULL, "Map must not be NULL.\n");
        ASSERT(desc->buffer.handle != RENDERER_INVALIDHANDLE, "Invalid buffer.\n");
        ASSERT(desc->size > 0, "Invalid mapping size.\n");

        this->resourcemutex.lock();
        struct buffer *buffer = &this->buffers[desc->buffer.handle].vkresource;
        map->buffer = desc->buffer;
        if (buffer->flags & ORenderer::BUFFERFLAG_PERFRAME) {
            for (size_t i = 0; i < RENDERER_MAXLATENCY; i++) {
                VkResult res = vmaMapMemory(allocator, buffer->allocation[i], &map->mapped[i]);
                if (res != VK_SUCCESS) {
                    resourcemutex.unlock();
                    ASSERT(false, "Failed to map memory.\n");
                }
            }
            resourcemutex.unlock();
            buffer->mapped = true;
        } else {
            VkResult res = vmaMapMemory(allocator, buffer->allocation[0], &map->mapped[0]);
            resourcemutex.unlock();
            ASSERT(res == VK_SUCCESS, "Failed to map memory.\n");
            buffer->mapped = true;
        }
        return ORenderer::RESULT_SUCCESS;
    }

    uint8_t VulkanContext::unmapbuffer(struct ORenderer::buffermap map) {
        ASSERT(map.buffer.handle != RENDERER_INVALIDHANDLE, "Invalid buffer.\n");
        this->resourcemutex.lock();
        struct buffer *buffer = &this->buffers[map.buffer.handle].vkresource;
        if (buffer->flags & ORenderer::BUFFERFLAG_PERFRAME) {
            for (size_t i = 0; i < RENDERER_MAXLATENCY; i++) {
                vmaUnmapMemory(allocator, buffer->allocation[i]);
            }
        } else {
            vmaUnmapMemory(allocator, buffer->allocation[0]);
        }
        buffer->mapped = false;
        this->resourcemutex.unlock();
        return ORenderer::RESULT_SUCCESS;
    }

    uint8_t VulkanContext::getlatency(void) {
        return (uint8_t)(this->frame & 0xFF);
    }

    uint8_t VulkanContext::copybuffer(struct ORenderer::buffercopydesc *desc) {
        ASSERT(desc != NULL, "Description must not be NULL.\n");
        ASSERT(desc->src.handle != RENDERER_INVALIDHANDLE, "Invalid source buffer.\n");
        ASSERT(desc->dst.handle != RENDERER_INVALIDHANDLE, "Invalid destination buffer.\n");

        VkCommandBuffer cmd = this->beginimmediate();

        VkBufferCopy copy = { };
        copy.srcOffset = desc->srcoffset;
        copy.dstOffset = desc->dstoffset;
        copy.size = desc->size;

        this->resourcemutex.lock();
        vkCmdCopyBuffer(cmd,
            this->buffers[desc->src.handle].vkresource.buffer[
            this->buffers[desc->src.handle].vkresource.flags & ORenderer::BUFFERFLAG_PERFRAME ? this->frame : 0],
            this->buffers[desc->dst.handle].vkresource.buffer[
            this->buffers[desc->dst.handle].vkresource.flags & ORenderer::BUFFERFLAG_PERFRAME ? this->frame : 0], 1, &copy);
        this->resourcemutex.unlock();

        this->endimmediate();
        return ORenderer::RESULT_SUCCESS;
    }

    void VulkanContext::destroytexture(struct ORenderer::texture *texture) {
        ASSERT(texture != NULL, "Texture must not be NULL.\n");
        ASSERT(texture->handle != RENDERER_INVALIDHANDLE, "Invalid texture.\n");

        this->resourcemutex.lock();
        struct texture *vktexture = &this->textures[texture->handle].vkresource;
        if (vktexture->flags & VULKAN_TEXTURESWAPCHAIN) {
            texture->handle = RENDERER_INVALIDHANDLE;
            resourcemutex.unlock();
            return; // we don't destroy the swapchain texture, we destroy the swapchain (but since that's handled by the context itself, that doesn't happen)
        } else {
            vmaDestroyImage(allocator, vktexture->image, vktexture->allocation);
            this->textures.erase(texture->handle);
            texture->handle = RENDERER_INVALIDHANDLE;
        }
        this->resourcemutex.unlock();
    }

    void VulkanContext::destroytextureview(struct ORenderer::textureview *textureview) {
        ASSERT(textureview != NULL, "View must not be NULL.\n");
        ASSERT(textureview->handle != RENDERER_INVALIDHANDLE, "Invalid texture view.\n");
        this->resourcemutex.lock();
        struct textureview *vktextureview = &this->textureviews[textureview->handle].vkresource;
        vkDestroyImageView(this->dev, vktextureview->imageview, NULL);
        this->textureviews.erase(textureview->handle);
        textureview->handle = RENDERER_INVALIDHANDLE;
        this->resourcemutex.unlock();
    }

    void VulkanContext::destroybuffer(struct ORenderer::buffer *buffer) {
        ASSERT(buffer != NULL, "Buffer must not be NULL.\n");
        ASSERT(buffer->handle != RENDERER_INVALIDHANDLE, "Invalid buffer.\n");

        this->resourcemutex.lock();
        struct buffer *vkbuffer = &this->buffers[buffer->handle].vkresource;
        if (vkbuffer->flags & ORenderer::BUFFERFLAG_PERFRAME) {
            for (size_t i = 0; i < RENDERER_MAXLATENCY; i++) {
                if (vkbuffer->mapped) {
                    vmaUnmapMemory(allocator, vkbuffer->allocation[i]);
                }
                vmaDestroyBuffer(allocator, vkbuffer->buffer[i], vkbuffer->allocation[i]);
                buffers.erase(buffer->handle);
                buffer->handle = RENDERER_INVALIDHANDLE;
            }
        } else {
            if (vkbuffer->mapped) {
                vmaUnmapMemory(allocator, vkbuffer->allocation[0]);
            }
            vmaDestroyBuffer(allocator, vkbuffer->buffer[0], vkbuffer->allocation[0]);
            this->buffers.erase(buffer->handle);
            buffer->handle = RENDERER_INVALIDHANDLE;
        }
        this->resourcemutex.unlock();
    }

    void VulkanContext::destroyframebuffer(struct ORenderer::framebuffer *framebuffer) {
        ASSERT(framebuffer != NULL, "Framebuffer must not be NULL.\n");
        ASSERT(framebuffer->handle != RENDERER_INVALIDHANDLE, "Invalid framebuffer.\n");

        this->resourcemutex.lock();
        struct framebuffer *vkframebuffer = &this->framebuffers[framebuffer->handle].vkresource;
        vkDestroyFramebuffer(this->dev, vkframebuffer->framebuffer, NULL);
        this->framebuffers.erase(framebuffer->handle);
        framebuffer->handle = RENDERER_INVALIDHANDLE;
        this->resourcemutex.unlock();
    }

    void VulkanContext::destroyrenderpass(struct ORenderer::renderpass *pass) {
        ASSERT(pass != NULL, "Pass must not be NULL.\n");
        ASSERT(pass->handle != RENDERER_INVALIDHANDLE, "Invalid renderpass.\n");

        this->resourcemutex.lock();
        struct renderpass *vkrenderpass = &this->renderpasses[pass->handle].vkresource;
        vkDestroyRenderPass(this->dev, vkrenderpass->renderpass, NULL);
        this->renderpasses.erase(pass->handle);
        pass->handle = RENDERER_INVALIDHANDLE;
        this->resourcemutex.unlock();
    }

    void VulkanContext::destroypipelinestate(struct ORenderer::pipelinestate *state) {
        ASSERT(state != NULL, "State must not be NULL.\n");
        ASSERT(state->handle != RENDERER_INVALIDHANDLE, "Invalid pipelinestate.\n");

        this->resourcemutex.lock();
        struct pipelinestate *vkpipelinestate = &this->pipelinestates[state->handle].vkresource;

        for (size_t i = 0; i < vkpipelinestate->shadercount; i++) {
            vkDestroyShaderModule(this->dev, vkpipelinestate->shaders[i], NULL);
        }
        vkDestroyPipelineLayout(this->dev, vkpipelinestate->pipelinelayout, NULL);
        vkFreeDescriptorSets(this->dev, this->descpool, RENDERER_MAXLATENCY, vkpipelinestate->descsets);
        // vkDestroyDescriptorSetLayout(this->dev, vkpipelinestate->descsetlayout, NULL);
        vkDestroyPipeline(this->dev, vkpipelinestate->pipeline, NULL);
        this->pipelinestates.erase(state->handle);
        state->handle = RENDERER_INVALIDHANDLE;
        this->resourcemutex.unlock();
    }

    void VulkanContext::setdebugname(struct ORenderer::texture texture, const char *name) {
        ASSERT(texture.handle != RENDERER_INVALIDHANDLE, "Invalid texture.\n");
#ifdef OMICRON_DEBUG
        VkDebugUtilsObjectNameInfoEXT nameinfo = { };
        nameinfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameinfo.pNext = NULL;
        nameinfo.objectHandle = (uint64_t)this->textures[texture.handle].vkresource.image;
        nameinfo.objectType = VK_OBJECT_TYPE_IMAGE;
        nameinfo.pObjectName = name;
        VkResult res = vkSetDebugUtilsObjectNameEXT(this->dev, &nameinfo);
        ASSERT(res == VK_SUCCESS, "Failed to set debug name for texture %lu to `%s`.\n", texture.handle, name);
#endif
    }

    void VulkanContext::setdebugname(struct ORenderer::textureview view, const char *name) {
        ASSERT(view.handle != RENDERER_INVALIDHANDLE, "Invalid texture view.\n");
#ifdef OMICRON_DEBUG
        VkDebugUtilsObjectNameInfoEXT nameinfo = { };
        nameinfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameinfo.pNext = NULL;
        nameinfo.objectHandle = (uint64_t)this->textureviews[view.handle].vkresource.imageview;
        nameinfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
        nameinfo.pObjectName = name;
        VkResult res = vkSetDebugUtilsObjectNameEXT(this->dev, &nameinfo);
        ASSERT(res == VK_SUCCESS, "Failed to set debug name for texture view %lu to `%s`.\n", view.handle, name);
#endif
    }

    void VulkanContext::setdebugname(struct ORenderer::buffer buffer, const char *name) {
        ASSERT(buffer.handle != RENDERER_INVALIDHANDLE, "Invalid buffer.\n");
#ifdef OMICRON_DEBUG
        if (!(this->buffers[buffer.handle].vkresource.flags & ORenderer::BUFFERFLAG_PERFRAME)) {
            VkDebugUtilsObjectNameInfoEXT nameinfo = { };
            nameinfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameinfo.pNext = NULL;
            nameinfo.objectHandle = (uint64_t)this->buffers[buffer.handle].vkresource.buffer[0];
            nameinfo.objectType = VK_OBJECT_TYPE_BUFFER;
            nameinfo.pObjectName = name;
            VkResult res = vkSetDebugUtilsObjectNameEXT(this->dev, &nameinfo);
            ASSERT(res == VK_SUCCESS, "Failed to set debug name for buffer %lu to `%s`.\n", buffer.handle, name);
        } else {
            VkDebugUtilsObjectNameInfoEXT nameinfos[RENDERER_MAXLATENCY] = { };
            for (size_t i = 0; i < RENDERER_MAXLATENCY; i++) {
                nameinfos[i].sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
                nameinfos[i].pNext = NULL;
                nameinfos[i].objectHandle = (uint64_t)this->buffers[buffer.handle].vkresource.buffer[i];
                nameinfos[i].objectType = VK_OBJECT_TYPE_BUFFER;
                nameinfos[i].pObjectName = name;
                VkResult res = vkSetDebugUtilsObjectNameEXT(this->dev, &nameinfos[i]);
                ASSERT(res == VK_SUCCESS, "Failed to set debug name for buffer %lu(latency %lu) to `%s`.\n", buffer.handle, i, name);

            }
        }
#endif
    }

    void VulkanContext::setdebugname(struct ORenderer::framebuffer framebuffer, const char *name) {
        ASSERT(framebuffer.handle != RENDERER_INVALIDHANDLE, "Invalid framebuffer.\n");
#ifdef OMICRON_DEBUG
        VkDebugUtilsObjectNameInfoEXT nameinfo = { };
        nameinfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameinfo.pNext = NULL;
        nameinfo.objectHandle = (uint64_t)this->framebuffers[framebuffer.handle].vkresource.framebuffer;
        nameinfo.objectType = VK_OBJECT_TYPE_FRAMEBUFFER;
        nameinfo.pObjectName = name;
        VkResult res = vkSetDebugUtilsObjectNameEXT(this->dev, &nameinfo);
        ASSERT(res == VK_SUCCESS, "Failed to set debug name for framebuffer %lu to `%s`.\n", framebuffer.handle, name);
#endif
    }

    void VulkanContext::setdebugname(struct ORenderer::renderpass renderpass, const char *name) {
        ASSERT(renderpass.handle != RENDERER_INVALIDHANDLE, "Invalid renderpass.\n");
#ifdef OMICRON_DEBUG
        VkDebugUtilsObjectNameInfoEXT nameinfo = { };
        nameinfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameinfo.pNext = NULL;
        nameinfo.objectHandle = (uint64_t)this->renderpasses[renderpass.handle].vkresource.renderpass;
        nameinfo.objectType = VK_OBJECT_TYPE_RENDER_PASS;
        nameinfo.pObjectName = name;
        VkResult res = vkSetDebugUtilsObjectNameEXT(this->dev, &nameinfo);
        ASSERT(res == VK_SUCCESS, "Failed to set debug name for renderpass %lu to `%s`.\n", renderpass.handle, name);
#endif
    }
    void VulkanContext::setdebugname(struct ORenderer::pipelinestate state, const char *name) {
        ASSERT(state.handle != RENDERER_INVALIDHANDLE, "Invalid state.\n");
#ifdef OMICRON_DEBUG
        VkDebugUtilsObjectNameInfoEXT nameinfo = { };
        nameinfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameinfo.pNext = NULL;
        nameinfo.objectHandle = (uint64_t)this->pipelinestates[state.handle].vkresource.pipeline;
        nameinfo.objectType = VK_OBJECT_TYPE_PIPELINE;
        nameinfo.pObjectName = name;
        VkResult res = vkSetDebugUtilsObjectNameEXT(this->dev, &nameinfo);
        ASSERT(res == VK_SUCCESS, "Failed to set debug name for pipeline state %lu to `%s`.\n", state.handle, name);
#endif
    }

    void VulkanContext::flushrange(struct ORenderer::buffer buffer, size_t size) {
        ASSERT(buffer.handle != RENDERER_INVALIDHANDLE, "Invalid buffer.\n");
        this->resourcemutex.lock();
        VkMappedMemoryRange range = { };
        range.size = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.pNext = NULL;
        range.memory = this->buffers[buffer.handle].vkresource.memory[0];
        range.offset = 0;
        range.size = size;
        VkResult res = vkFlushMappedMemoryRanges(this->dev, 1, &range);
        ASSERT(res == VK_SUCCESS, "Failed to flush Vulkan memory range.\n");
        this->resourcemutex.unlock();
    }

    uint8_t VulkanContext::requestbackbuffer(struct ORenderer::framebuffer *framebuffer) {
        ASSERT(framebuffer != NULL, "Framebuffer must not be NULL.\n");
        *framebuffer = this->swapfbs[this->swapimage];
        return ORenderer::RESULT_SUCCESS;
    }

    uint8_t VulkanContext::requestbackbufferinfo(struct ORenderer::backbufferinfo *info) {
        ASSERT(info != NULL, "Info must not be NULL.\n");

        info->format = ORenderer::FORMAT_BGRA8SRGB; // XXX: Return selected swapchain format (based on support)
        info->width = this->surfacecaps.currentExtent.width;
        info->height = this->surfacecaps.currentExtent.height;

        return ORenderer::RESULT_SUCCESS;
    }

    uint8_t VulkanContext::submitstream(ORenderer::Stream *stream, bool wait) {
        ASSERT(stream != NULL, "Stream must not be NULL.\n");
        stream->claim();

        VulkanStream *vkstream = (VulkanStream *)stream;

        VkSubmitInfo submit = { };
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.pNext = NULL;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &vkstream->cmd;


        VkPipelineStageFlags stagemask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT; // XXX: Better specifics.
        VkSemaphore *semaphores = NULL;
        VkPipelineStageFlags *stages = NULL;
        if (vkstream->waiton.size() > 0) {
            // Wait on this one.
            submit.waitSemaphoreCount = vkstream->waiton.size();
            printf("%u\n", submit.waitSemaphoreCount);
            semaphores = (VkSemaphore *)malloc(sizeof(VkSemaphore) * submit.waitSemaphoreCount);
            ASSERT(semaphores != NULL, "Failed to allocate memory for queue wait semaphores.\n");
            stages = (VkPipelineStageFlags *)malloc(sizeof(VkPipelineStageFlags) * submit.waitSemaphoreCount);
            ASSERT(semaphores != NULL, "Failed to allocate memory for queue wait stages.\n");
            for (size_t i = 0; i < submit.waitSemaphoreCount; i++) {
                printf("%lx %lx\n", semaphores, vkstream->waiton[i]);
                semaphores[i] = vkstream->waiton[i]->semaphore;
                stages[i] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            }
            submit.pWaitSemaphores = semaphores;
            // This'll mean we wait for the semaphore to trigger before even beginning to do any work.
            submit.pWaitDstStageMask = stages;
        }

        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &vkstream->semaphore; // Trigger our own semaphore (this is so that later on, other streams can reference this semaphore to await our completion).

        VkResult res = vkQueueSubmit(
            vkstream->type == ORenderer::STREAM_COMPUTE ? this->asynccompute :
            vkstream->type == ORenderer::STREAM_TRANSFER ? this->asynctransfer :
            this->graphicsqueue,
            1, &submit, wait ? vkstream->fence : VK_NULL_HANDLE);
        ASSERT(res == VK_SUCCESS, "Failed to submit Vulkan queue %d.\n", res);

        if (semaphores != NULL) {
            free(semaphores);
            free(stages);
        }

        if (wait) {
            vkWaitForFences(this->dev, 1, &vkstream->fence, VK_TRUE, UINT64_MAX);
            vkResetFences(this->dev, 1, &vkstream->fence);
        }

        vkstream->waiton.clear(); // clear dependencies on submission.

        stream->release();
        return ORenderer::RESULT_SUCCESS;
    }
    ORenderer::Stream *VulkanContext::getimmediate(void) {
        return &this->imstream;
    }

    ORenderer::Stream *VulkanContext::requeststream(uint8_t type) {
        if (type == ORenderer::STREAM_TRANSFER || type == ORenderer::STREAM_COMPUTE || type == ORenderer::STREAM_IMMEDIATE) {
            return this->streampool.alloc(type);
        } else if (type == ORenderer::STREAM_FRAME) {
            return &this->stream[this->frame];
        } else {
            ASSERT(false, "Invalid stream type %u requested.\n", type);
        }
    }

    void VulkanStream::wait(void) {
        vkWaitForFences(this->context->dev, 1, &this->fence, VK_TRUE, UINT64_MAX);
        vkResetFences(this->context->dev, 1, &this->fence);
    }

    void VulkanContext::freestream(ORenderer::Stream *stream) {
        VulkanStream *vkstream = (VulkanStream *)stream;
        if (vkstream->type == ORenderer::STREAM_TRANSFER || vkstream->type == ORenderer::STREAM_COMPUTE) {
            this->streampool.free(vkstream);
        }
    }

    void VulkanStream::begin(void) {
        VkCommandBufferBeginInfo info = { };
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.pNext = NULL;
        info.flags = 0;
        info.pInheritanceInfo = NULL;
        VkResult res = vkBeginCommandBuffer(this->cmd, &info);
        ASSERT(res == VK_SUCCESS, "Failed to begin Vulkan command buffer %d.\n", res);
    }

    uint64_t VulkanStream::zonebegin(const char *name) {
        ASSERT(name != NULL, "Invalid zone name.\n");
        void *mem = this->tempmem.alloc(sizeof(tracy::VkCtxScope)); // allocate

#ifdef OMICRON_DEBUG
        tracy::VkCtxScope *ctx = new (mem) tracy::VkCtxScope(this->context->tracyctx, TracyLine, TracyFile, strlen(TracyFile), TracyFunction, strlen(TracyFunction), name, strlen(name), this->cmd, true);
#else
        return 0;
#endif
        return (uint64_t)ctx;
    }

    void VulkanStream::zoneend(uint64_t zone) {
#ifdef OMICRON_DEBUG
        tracy::VkCtxScope *ctx = (tracy::VkCtxScope *)zone;
        ctx->~VkCtxScope();
#endif
    }

    void VulkanStream::marker(const char *name) {
#ifdef OMICRON_DEBUG
        VkDebugUtilsLabelEXT label = { };
        label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        label.pNext = NULL;
        label.pLabelName = name;
        glm::vec4 colour = glm::vec4(0.0, 0.0, 0.0, 1.0);
        memcpy(label.color, &colour, sizeof(glm::vec4));
        vkCmdInsertDebugUtilsLabelEXT(this->cmd, &label);
#endif
    }

    void VulkanStream::flushcmd(void) {
        this->tempmem.clear();
        this->mappings.clear();
        vkResetCommandBuffer(this->cmd, 0);
    }

    void VulkanStream::setviewport(struct ORenderer::viewport viewport) {
        ZoneScoped;
        VkViewport vkview;
        vkview.x = viewport.x;
        vkview.y = viewport.y;
        vkview.width = viewport.width;
        vkview.height = viewport.height;
        vkview.minDepth = viewport.mindepth;
        vkview.maxDepth = viewport.maxdepth;
        vkCmdSetViewport(this->cmd, 0, 1, &vkview);
    }

    void VulkanStream::setscissor(struct ORenderer::rect scissor) {
        ZoneScoped;
        VkRect2D vkscissor;
        vkscissor.extent.width = scissor.width;
        vkscissor.extent.height = scissor.height;
        vkscissor.offset.x = scissor.x;
        vkscissor.offset.y = scissor.y;
        vkCmdSetScissor(this->cmd, 0, 1, &vkscissor);
    }

    void VulkanStream::beginrenderpass(struct ORenderer::renderpass renderpass, struct ORenderer::framebuffer framebuffer, struct ORenderer::rect area, struct ORenderer::clearcolourdesc clear) {
        ZoneScoped;
        size_t marker = this->tempmem.getmarker();

        VkRenderPass rpass = this->context->renderpasses[renderpass.handle].vkresource.renderpass;
        VkFramebuffer fb = this->context->framebuffers[framebuffer.handle].vkresource.framebuffer;

        VkRenderPassBeginInfo begin = { };
        begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        begin.pNext = NULL;
        begin.renderPass = rpass;
        begin.framebuffer = fb;
        begin.renderArea.offset = (VkOffset2D) { .x = area.x, .y = area.y };
        begin.renderArea.extent = (VkExtent2D) { .width = area.width, .height = area.height };
        begin.clearValueCount = clear.count;
        VkClearValue *values = (VkClearValue *)this->tempmem.alloc(sizeof(VkClearValue) * clear.count);
        for (size_t i = 0; i < clear.count; i++) {
            if (clear.clear[i].isdepth) {
                values[i].depthStencil = { .depth = clear.clear[i].depth, .stencil = clear.clear[i].stencil };
            } else {
                values[i].color = { .float32 = { clear.clear[i].colour.r, clear.clear[i].colour.g, clear.clear[i].colour.b, clear.clear[i].colour.a } };
            }
        }
        begin.pClearValues = values;
        vkCmdBeginRenderPass(this->cmd, &begin, VK_SUBPASS_CONTENTS_INLINE);
        this->tempmem.freeto(marker);
    }

    void VulkanStream::setpipelinestate(struct ORenderer::pipelinestate pipeline) {
        ZoneScoped;
        this->pipelinestate = pipeline;
        struct pipelinestate *state = &this->context->pipelinestates[pipeline.handle].vkresource;
        vkCmdBindPipeline(this->cmd, state->type == ORenderer::GRAPHICSPIPELINE ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE, state->pipeline);
    }

    void VulkanStream::setidxbuffer(struct ORenderer::buffer buffer, size_t offset, bool index32) {
        ZoneScoped;
        struct buffer *vkbuffer = &this->context->buffers[buffer.handle].vkresource;
        vkCmdBindIndexBuffer(this->cmd, vkbuffer->buffer[vkbuffer->flags & ORenderer::BUFFERFLAG_PERFRAME ? this->context->frame : 0], offset, index32 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
    }

    void VulkanStream::setvtxbuffers(struct ORenderer::buffer *buffers, size_t *offsets, size_t firstbind, size_t bindcount) {
        ZoneScoped;
        size_t marker = this->tempmem.getmarker();

        VkBuffer *vkbuffers = (VkBuffer *)this->tempmem.alloc(sizeof(VkBuffer) * bindcount);
        for (size_t i = 0; i < bindcount; i++) {
            struct buffer *buffer = &this->context->buffers[buffers->handle].vkresource;
            vkbuffers[i] = buffer->buffer[buffer->flags & ORenderer::BUFFERFLAG_PERFRAME ? this->context->frame : 0];
        }
        vkCmdBindVertexBuffers(this->cmd, firstbind, bindcount, vkbuffers, offsets);
        this->tempmem.freeto(marker);
    }

    void VulkanStream::setvtxbuffer(struct ORenderer::buffer buffer, size_t offset) {
        ZoneScoped;
        struct buffer *vkbuffer = &this->context->buffers[buffer.handle].vkresource;
        vkCmdBindVertexBuffers(this->cmd, 0, 1, &vkbuffer->buffer[vkbuffer->flags & ORenderer::BUFFERFLAG_PERFRAME ? this->context->frame : 0], &offset);
    }

    void VulkanStream::draw(size_t vtxcount, size_t instancecount, size_t firstvtx, size_t firstinstance) {
        ZoneScoped;
        vkCmdDraw(this->cmd, vtxcount, instancecount, firstvtx, firstinstance);
    }

    void VulkanStream::drawindexed(size_t idxcount, size_t instancecount, size_t firstidx, size_t vtxoffset, size_t firstinstance) {
        ZoneScoped;
        vkCmdDrawIndexed(this->cmd, idxcount, instancecount, firstidx, vtxoffset, firstinstance);

    }

    void VulkanStream::commitresources(void) {
        ZoneScoped;
        ASSERT(this->pipelinestate.handle != RENDERER_INVALIDHANDLE, "Attempted to commit resources before deciding pipeline state.\n");
        size_t marker = this->tempmem.getmarker();

        VkWriteDescriptorSet *wds = (VkWriteDescriptorSet *)this->tempmem.alloc(sizeof(VkWriteDescriptorSet) * this->mappings.size());
        // XXX: Always be careful about scope! Higher optimisation levels seem to treat anything even one level out of scope as invalid (so we should make sure not to let that happen!)
        VkDescriptorBufferInfo *buffinfos = (VkDescriptorBufferInfo *)this->tempmem.alloc(sizeof(VkDescriptorBufferInfo) * this->mappings.size());
        VkDescriptorImageInfo *imginfos = (VkDescriptorImageInfo *)this->tempmem.alloc(sizeof(VkDescriptorImageInfo) * this->mappings.size()); // we're allocating with worst case in mind
        for (size_t i = 0; i < this->mappings.size(); i++) {
            wds[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            wds[i].pNext = NULL;
            wds[i].dstBinding = this->mappings[i].binding;
            wds[i].dstSet = this->context->pipelinestates[this->pipelinestate.handle].vkresource.descsets[this->context->frame];
            wds[i].dstArrayElement = 0;
            wds[i].descriptorCount = 1;
            if (this->mappings[i].type == ORenderer::RESOURCE_UNIFORM || this->mappings[i].type == ORenderer::RESOURCE_STORAGE) {
                struct ORenderer::bufferbind bind = this->mappings[i].bufferbind;
                struct buffer *buffer = &this->context->buffers[bind.buffer.handle].vkresource;
                wds[i].descriptorType = descriptortypetable[this->mappings[i].type];
                buffinfos[i].buffer = buffer->flags & ORenderer::BUFFERFLAG_PERFRAME ? buffer->buffer[this->context->frame] : buffer->buffer[0];
                buffinfos[i].offset = bind.offset;
                buffinfos[i].range = bind.range == SIZE_MAX ? VK_WHOLE_SIZE : bind.range;
                wds[i].pBufferInfo = &buffinfos[i];
            } else if (this->mappings[i].type == ORenderer::RESOURCE_SAMPLER || this->mappings[i].type == ORenderer::RESOURCE_STORAGETEXTURE || this->mappings[i].type == ORenderer::RESOURCE_TEXTURE) {
                struct ORenderer::sampledbind bind = this->mappings[i].sampledbind;
                struct sampler *sampler = &this->context->samplers[bind.sampler.handle].vkresource;
                struct textureview *view = &this->context->textureviews[bind.view.handle].vkresource;
                wds[i].descriptorType = descriptortypetable[this->mappings[i].type];
                imginfos[i].sampler = this->mappings[i].type == ORenderer::RESOURCE_SAMPLER ? sampler->sampler : VK_NULL_HANDLE;
                imginfos[i].imageView = this->mappings[i].type != ORenderer::RESOURCE_SAMPLER ? view->imageview : VK_NULL_HANDLE;
                imginfos[i].imageLayout = this->mappings[i].type != ORenderer::RESOURCE_SAMPLER ? layouttable[bind.layout] : VK_IMAGE_LAYOUT_UNDEFINED;
                wds[i].pImageInfo = &imginfos[i];
            }
        }

        struct pipelinestate *pipelinestate = &this->context->pipelinestates[this->pipelinestate.handle].vkresource;
        {
            ZoneScopedN("push descriptor\n");
            // Incredibly expensive.
            vkCmdPushDescriptorSetKHR(cmd, pipelinestate->type == ORenderer::GRAPHICSPIPELINE ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE, pipelinestate->pipelinelayout, 0, this->mappings.size(), wds);
        }


        this->mappings.clear();
        this->tempmem.freeto(marker);
    }

    void VulkanStream::bindresource(size_t binding, struct ORenderer::bufferbind bind, size_t type) {
        ZoneScoped;
        this->mappings.push_back((struct ORenderer::pipelinestateresourcemap) { .binding = binding, .type = type, .bufferbind = bind });
    }

    void VulkanStream::bindresource(size_t binding, struct ORenderer::sampledbind bind, size_t type) {
        ZoneScoped;
        this->mappings.push_back((struct ORenderer::pipelinestateresourcemap) { .binding = binding, .type = type, .sampledbind = bind });
    }

    void VulkanStream::barrier(struct ORenderer::texture texture, size_t format, size_t oldlayout, size_t newlayout, size_t srcstage, size_t dststage, size_t srcaccess, size_t dstaccess, uint8_t srcqueue, uint8_t dstqueue, size_t basemip, size_t mipcount, size_t baselayer, size_t layercount) {
        ZoneScoped;
        struct texture *vktexture = &this->context->textures[texture.handle].vkresource;
        OVulkan::barrier(this->cmd, vktexture, format, oldlayout != ORenderer::LAYOUT_COUNT ? oldlayout : vktexture->layout, newlayout, srcstage, dststage, srcaccess, dstaccess, srcqueue, dstqueue, basemip, mipcount, baselayer, layercount);
        vktexture->layout = newlayout;
    }

    void VulkanStream::copybufferimage(struct ORenderer::bufferimagecopy region, struct ORenderer::buffer buffer, struct ORenderer::texture texture, size_t layout) {
        ZoneScoped;
        struct buffer *vkbuffer = &this->context->buffers[buffer.handle].vkresource;
        struct texture *vktexture = &this->context->textures[texture.handle].vkresource;
        VkBufferImageCopy vkregion = { };
        vkregion.bufferOffset = region.offset;
        vkregion.bufferRowLength = region.rowlen;
        vkregion.bufferImageHeight = region.imgheight;
        vkregion.imageSubresource.aspectMask =
            (region.aspect & ORenderer::ASPECT_COLOUR ? VK_IMAGE_ASPECT_COLOR_BIT : 0) |
            (region.aspect & ORenderer::ASPECT_DEPTH ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
            (region.aspect & ORenderer::ASPECT_STENCIL ? VK_IMAGE_ASPECT_STENCIL_BIT : 0) |
            0;
        vkregion.imageSubresource.mipLevel = region.mip;
        vkregion.imageSubresource.layerCount = region.layercount;
        vkregion.imageSubresource.baseArrayLayer = region.baselayer;
        vkregion.imageExtent = { (uint32_t)region.imgextent.width, (uint32_t)region.imgextent.height, (uint32_t)region.imgextent.depth };
        vkregion.imageOffset = { (int32_t)region.imgoff.x, (int32_t)region.imgoff.y, (int32_t)region.imgoff.z };
        vkCmdCopyBufferToImage(cmd, vkbuffer->flags & ORenderer::BUFFERFLAG_PERFRAME ? vkbuffer->buffer[this->context->frame] : vkbuffer->buffer[0], vktexture->image, layouttable[layout], 1, &vkregion);
    }

    void VulkanStream::copyimage(struct ORenderer::imagecopy region, struct ORenderer::texture src, struct ORenderer::texture dst, size_t srclayout, size_t dstlayout) {
        ZoneScoped;
        struct texture *vksrc = &this->context->textures[src.handle].vkresource;
        struct texture *vkdst = &this->context->textures[dst.handle].vkresource;
        VkImageCopy vkregion = { };
        vkregion.srcOffset = { (int32_t)region.srcoff.x, (int32_t)region.srcoff.y, (int32_t)region.srcoff.z };
        vkregion.dstOffset = { (int32_t)region.dstoff.x, (int32_t)region.dstoff.y, (int32_t)region.dstoff.z };
        vkregion.extent = { (uint32_t)region.extent.width, (uint32_t)region.extent.height, (uint32_t)region.extent.depth };
        vkregion.srcSubresource.mipLevel = region.srcmip;
        vkregion.dstSubresource.mipLevel = region.dstmip;
        vkregion.srcSubresource.baseArrayLayer = region.srcbaselayer;
        vkregion.dstSubresource.baseArrayLayer = region.dstbaselayer;
        vkregion.srcSubresource.layerCount = region.srclayercount;
        vkregion.dstSubresource.layerCount = region.dstlayercount;
        vkregion.srcSubresource.aspectMask =
            (region.srcaspect & ORenderer::ASPECT_COLOUR ? VK_IMAGE_ASPECT_COLOR_BIT : 0) |
            (region.srcaspect & ORenderer::ASPECT_DEPTH ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
            (region.srcaspect & ORenderer::ASPECT_STENCIL ? VK_IMAGE_ASPECT_STENCIL_BIT : 0) |
            0;
        vkregion.dstSubresource.aspectMask =
            (region.dstaspect & ORenderer::ASPECT_COLOUR ? VK_IMAGE_ASPECT_COLOR_BIT : 0) |
            (region.dstaspect & ORenderer::ASPECT_DEPTH ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
            (region.dstaspect & ORenderer::ASPECT_STENCIL ? VK_IMAGE_ASPECT_STENCIL_BIT : 0) |
            0;
        vkCmdCopyImage(cmd, vksrc->image, layouttable[srclayout], vkdst->image, layouttable[dstlayout], 1, &vkregion);
    }

    void VulkanStream::pushconstants(struct ORenderer::pipelinestate state, void *data, size_t size) {
        this->context->resourcemutex.lock();
        struct pipelinestate *vkstate = &this->context->pipelinestates[state.handle].vkresource;
        vkCmdPushConstants(this->cmd, vkstate->pipelinelayout, VK_SHADER_STAGE_ALL, 0, size, data);
        this->context->resourcemutex.unlock();
    }

    void VulkanContext::updateset(struct ORenderer::resourceset set, struct ORenderer::samplerbind *bind) {
        VkDescriptorImageInfo info = { };
        struct sampler *sampler = &this->samplers[bind->sampler.handle].vkresource;
        struct resourceset *vkset = &this->sets[set.handle].vkresource;

        info.sampler = sampler->sampler;
        info.imageLayout = layouttable[bind->layout];

        VkWriteDescriptorSet wset = { };
        wset.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wset.pNext = NULL;
        wset.dstSet = vkset->set;
        wset.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        wset.descriptorCount = 1;
        wset.dstBinding = 0; // XXX: SAMPLER BINDING
        wset.pImageInfo = &info;
        wset.dstArrayElement = bind->id;
        vkUpdateDescriptorSets(this->dev, 1, &wset, 0, NULL);
    }

    void VulkanContext::updateset(struct ORenderer::resourceset set, struct ORenderer::texturebind *bind) {
        VkDescriptorImageInfo info = { };
        struct textureview *view = &this->textureviews[bind->view.handle].vkresource;
        struct resourceset *vkset = &this->sets[set.handle].vkresource;

        info.imageView = view->imageview;
        info.imageLayout = layouttable[bind->layout];

        VkWriteDescriptorSet wset = { };
        wset.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wset.pNext = NULL;
        wset.dstSet = vkset->set;
        wset.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        wset.descriptorCount = 1;
        wset.dstBinding = 1; // XXX: TEXTURE BINDING
        wset.pImageInfo = &info;
        wset.dstArrayElement = bind->id;
        vkUpdateDescriptorSets(this->dev, 1, &wset, 0, NULL);
    }

    void VulkanStream::bindset(struct ORenderer::resourceset set) {
        this->context->resourcemutex.lock();
        struct resourceset *vkset = &this->context->sets[set.handle].vkresource;
        struct pipelinestate *state = &this->context->pipelinestates[this->pipelinestate.handle].vkresource;
        vkCmdBindDescriptorSets(this->cmd, state->type == ORenderer::COMPUTEPIPELINE ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS, state->pipelinelayout, 0, 1, &vkset->set, 0, NULL);
        this->context->resourcemutex.unlock();
    }

    void VulkanStream::submitstream(ORenderer::Stream *stream) {
        stream->claim();
        // vkCmdExecuteCommands(this->cmd, 1, &((VulkanStream *)stream)->cmd);
        this->context->streampool.free((VulkanStream *)stream);
        stream->release();
    }

    void VulkanStream::endrenderpass(void) {
        ZoneScoped;
        vkCmdEndRenderPass(this->cmd);
    }

    void VulkanStream::end() {
        TracyVkCollect(this->context->tracyctx, cmd);
        VkResult res = vkEndCommandBuffer(this->cmd);
        ASSERT(res == VK_SUCCESS, "Failed to end Vulkan command buffer %d.\n", res);
    }

    VkShaderModule VulkanContext::createshadermodule(ORenderer::Shader shader) {
        VkShaderModuleCreateInfo shadercreate = { };
        shadercreate.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shadercreate.codeSize = shader.size;
        shadercreate.pCode = (const uint32_t *)shader.code;

        VkShaderModule module;
        VkResult res = vkCreateShaderModule(this->dev, &shadercreate, NULL, &module);
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan shader %d.\n", res);

        return module;
    }

    void VulkanContext::createswapchain(void) {
        VkResult res;

        res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->phy, this->surface, &this->surfacecaps);
        ASSERT(res == VK_SUCCESS, "Failed to get Vulkan surface capabilities %d.\n", res);
        VkSwapchainCreateInfoKHR swapcreate = { };
        swapcreate.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapcreate.pNext = NULL;
        swapcreate.surface = this->surface;
        swapcreate.minImageCount = this->surfacecaps.minImageCount + 1;
        swapcreate.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
        swapcreate.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        swapcreate.imageExtent = this->surfacecaps.currentExtent;
        swapcreate.imageArrayLayers = 1;
        swapcreate.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        uint32_t indices[2] = { this->initialfamily, this->presentfamily };
        if (this->initialfamily != this->presentfamily) {
            swapcreate.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapcreate.queueFamilyIndexCount = 2;
            swapcreate.pQueueFamilyIndices = indices;
        } else {
            swapcreate.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        swapcreate.preTransform = this->surfacecaps.currentTransform;
        swapcreate.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        // swapcreate.presentMode = VK_PRESENT_MODE_MAILBOX_KHR; // we want to use mailbox as much as possible in order to reduce stuttering, latency, and all other related issues.
        swapcreate.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        // MAILBOX keeps rendering until the display can handle a new frame (so latency is kept at a minimum as the next frame still gets rendered, no tearing)
        // IMMEDIATE ignores V-sync, all rendering requests are applied as fast as possible (can cause tearing)
        // FIFO wait until next vertical sync, queue is used so all requests are executed in order of first order (latency depends on refresh rate)
        // FIFO_RELAXED standard FIFO queue, does not always respect vertical sync (reduces visual stutter)
        swapcreate.clipped = VK_FALSE;
        swapcreate.oldSwapchain = VK_NULL_HANDLE;

        res = vkCreateSwapchainKHR(this->dev, &swapcreate, NULL, &this->swapchain);
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan swapchain %d.\n", res);

    }

    void VulkanContext::destroyswapchain(void) {
        for (size_t i = 0; i < this->swaptexturecount; i++) {
            this->destroyframebuffer(&this->swapfbs[i]);
            this->destroytextureview(&this->swaptextureviews[i]);
            this->destroytexture(&this->swaptextures[i]);
        }

        vkDestroySwapchainKHR(this->dev, this->swapchain, NULL);
    }

    void VulkanContext::createswapchainviews(void) {
        VkImage images[VULKAN_MAXBACKBUFFERS];
        vkGetSwapchainImagesKHR(this->dev, this->swapchain, &this->swaptexturecount, NULL);

        vkGetSwapchainImagesKHR(this->dev, this->swapchain, &this->swaptexturecount, images);
        for (size_t i = 0; i < this->swaptexturecount; i++) {
            // we use custom code to force the swapchain texture into the resources array as opposed to using our existing api
            if (this->swaptextures[i].handle == RENDERER_INVALIDHANDLE) {
                this->resourcemutex.lock();
                this->swaptextures[i].handle = this->resourcehandle++;
                this->textures[this->swaptextures[i].handle].vkresource = (struct texture) { };
                this->resourcemutex.unlock();
            }
            this->resourcemutex.lock();
            struct texture *vktexture = &this->textures[this->swaptextures[i].handle].vkresource;
            vktexture->state = VK_IMAGE_LAYOUT_UNDEFINED; // this is the state our swapchain textures start out as
            vktexture->image = images[i];
            vktexture->flags = VULKAN_TEXTURESWAPCHAIN; // so that we dont try to destroy a VMA allocation that doesn't exist
            char buf[64];
            snprintf(buf, 63, "Backbuffer Texture %lu", i);
            this->setdebugname(this->swaptextures[i], buf);
            this->resourcemutex.unlock();

            struct ORenderer::textureviewdesc viewdesc = { };
            viewdesc.format = ORenderer::FORMAT_BGRA8SRGB; // that's the format our swapchain comes in (XXX: Handle this properly)
            viewdesc.texture = this->swaptextures[i];
            viewdesc.type = ORenderer::IMAGETYPE_2D;
            viewdesc.aspect = ORenderer::ASPECT_COLOUR;
            viewdesc.mipcount = 1;
            viewdesc.baselayer = 0;
            viewdesc.layercount = 1;
            ASSERT(this->createtextureview(&viewdesc, &this->swaptextureviews[i]) == ORenderer::RESULT_SUCCESS, "Failed to create Vulkan swapchain texture view.\n");
            snprintf(buf, 63, "Backbuffer Texture View %lu", i);
            this->setdebugname(this->swaptextureviews[i], buf);
        }
    }

    uint8_t VulkanContext::createbackbuffer(struct ORenderer::renderpass pass, struct ORenderer::textureview *depth[RENDERER_MAXLATENCY]) {
        ASSERT(pass.handle != RENDERER_INVALIDHANDLE, "Invalid renderpass.\n");

        this->swapfbpass = pass;

        for (size_t i = 0; i < this->swaptexturecount; i++) {
            struct ORenderer::framebufferdesc fbdesc = { };
            fbdesc.width = this->surfacecaps.currentExtent.width;
            fbdesc.height = this->surfacecaps.currentExtent.height;
            fbdesc.layers = 1;
            fbdesc.attachmentcount = depth != NULL ? 2 : 1;
            fbdesc.pass = pass; // XXX: How do we setup the renderpass for the backbuffer?
            struct ORenderer::textureview attachments[] = {
                this->swaptextureviews[i],
                depth[i % RENDERER_MAXLATENCY] != NULL ? *depth[i % RENDERER_MAXLATENCY] :
                    (struct ORenderer::textureview) { RENDERER_INVALIDHANDLE }
            };
            fbdesc.attachments = attachments;
            ASSERT(this->createframebuffer(&fbdesc, &this->swapfbs[i]) == ORenderer::RESULT_SUCCESS, "Failed to create Vulkan swapchain framebuffer.\n");
            char buf[64];
            snprintf(buf, 63, "Backbuffer %lu", i);
            this->setdebugname(this->swapfbs[i], buf);
        }
        return ORenderer::RESULT_SUCCESS;
    }

    VulkanContext::~VulkanContext(void) {
        OUtils::print("Shutting down Vulkan context and destroying all active resources.\n");

        vkDeviceWaitIdle(this->dev); // required to allow us to do any work

        this->destroyswapchain();

        this->resourcemutex.lock();

        for (auto it = this->textures.begin(); it != this->textures.end(); it++) {
            struct texture vktexture = it->second.vkresource;
            if (vktexture.flags & VULKAN_TEXTURESWAPCHAIN) {
                continue;
            } else {
                vmaDestroyImage(this->allocator, vktexture.image, vktexture.allocation);
            }
        }

        for (auto it = this->textureviews.begin(); it != this->textureviews.end(); it++) {
            struct textureview vktextureview = it->second.vkresource;
            vkDestroyImageView(this->dev, vktextureview.imageview, NULL);
        }

        for (auto it = this->samplers.begin(); it != this->samplers.end(); it++) {
            struct sampler vksampler = it->second.vkresource;
            vkDestroySampler(this->dev, vksampler.sampler, NULL);
        }

        for (auto it = this->buffers.begin(); it != this->buffers.end(); it++) {
            struct buffer vkbuffer = it->second.vkresource;
            if (vkbuffer.flags & ORenderer::BUFFERFLAG_PERFRAME) {
                for (size_t i = 0; i < RENDERER_MAXLATENCY; i++) {
                    if (vkbuffer.mapped) {
                        vmaUnmapMemory(this->allocator, vkbuffer.allocation[i]);
                    }
                    vmaDestroyBuffer(this->allocator, vkbuffer.buffer[i], vkbuffer.allocation[i]);
                }
            } else {
                if (vkbuffer.mapped) {
                    vmaUnmapMemory(this->allocator, vkbuffer.allocation[0]);
                }
                vmaDestroyBuffer(this->allocator, vkbuffer.buffer[0], vkbuffer.allocation[0]);
            }
        }

        for (auto it = this->framebuffers.begin(); it != this->framebuffers.end(); it++) {
            struct framebuffer vkframebuffer = it->second.vkresource;
            vkDestroyFramebuffer(this->dev, vkframebuffer.framebuffer, NULL);
        }

        for (auto it = this->renderpasses.begin(); it != this->renderpasses.end(); it++) {
            struct renderpass vkrenderpass = it->second.vkresource;
            vkDestroyRenderPass(this->dev, vkrenderpass.renderpass, NULL);
        }

        for (auto it = this->pipelinestates.begin(); it != this->pipelinestates.end(); it++) {
            struct pipelinestate vkpipelinestate = it->second.vkresource;
            for (size_t i = 0; i < vkpipelinestate.shadercount; i++) {
                vkDestroyShaderModule(this->dev, vkpipelinestate.shaders[i], NULL);
            }
            vkDestroyPipelineLayout(this->dev, vkpipelinestate.pipelinelayout, NULL);
            // vkDestroyDescriptorSetLayout(this->dev, vkpipelinestate.descsetlayout, NULL);
            vkDestroyPipeline(this->dev, vkpipelinestate.pipeline, NULL);
        }

        for (auto it = this->layouts.begin(); it != this->layouts.end(); it++) {
            vkDestroyDescriptorSetLayout(this->dev, it->second.vkresource.layout, NULL);
        }

        vkDestroyDescriptorPool(this->dev, this->descpool, NULL);
        this->streampool.destroy();
        vkDestroyCommandPool(this->dev, this->transferpool, NULL);
        vkDestroyCommandPool(this->dev, this->computepool, NULL);
        vkDestroyCommandPool(this->dev, this->cmdpool, NULL);
        for (size_t i = 0; i < RENDERER_MAXLATENCY; i++) {
            vkDestroySemaphore(this->dev, this->imagepresent[i], NULL);
            vkDestroySemaphore(this->dev, this->renderdone[i], NULL);
            vkDestroyFence(this->dev, this->framesinflight[i], NULL);
        }
        vkDestroyFence(this->dev, this->imfence, NULL);

        vmaDestroyAllocator(this->allocator);

        // for (size_t i = 0; i < RENDERER_MAXLATENCY; i++) {
        TracyVkDestroy(this->tracyctx);
        // }

        vkDestroyDevice(this->dev, NULL);
        vkDestroySurfaceKHR(this->instance, this->surface, NULL);

        this->resourcemutex.unlock();
    }

    VulkanContext::VulkanContext(struct ORenderer::init *init) : ORenderer::RendererContext(init) {
        this->init = *init;

#ifdef __linux__
        this->vulkanlib = dlopen("libvulkan.so.1", RTLD_NOW);
#endif
        ASSERT(this->vulkanlib != NULL, "Failed to load Vulkan library! ('%s')\n", dlerror());

        VkResult res;

#define VK_IMPORT_FUNC(OPT, FUNC) \
        FUNC = (PFN_##FUNC)dlsym(this->vulkanlib, #FUNC); \
        ASSERT(OPT || FUNC != NULL, "Failed to import required function %s from Vulkan library!", #FUNC);
    VK_IMPORT

#undef VK_IMPORT_FUNC

        uint32_t apiversion;
        if (vkEnumerateInstanceVersion != NULL) {
            res = vkEnumerateInstanceVersion(&apiversion);
            ASSERT(res == VK_SUCCESS, "Failed to enumerate instance version with error %d.\n", res);
        } else {
            apiversion = VK_API_VERSION_1_0;
        }

        VkApplicationInfo info = { };
        info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        info.pNext = NULL;
        info.pApplicationName = "Don't know actually";
        info.applicationVersion = VK_MAKE_VERSION(1, 0, 0); // config
        info.pEngineName = "Omicron Engine";
        uint32_t major = 0;
        uint32_t minor = 0;
        uint32_t revision = 0;
        int sres = sscanf(OMICRON_VERSION, "%u.%u.%u", &major, &minor, &revision);
        ASSERT(sres == 3, "Invalid version string during compilation.\n"); // If the result is anything other than three, the version string did not match the format string.
        info.engineVersion = VK_MAKE_VERSION(major, minor, revision); // assemble version out of components.
        info.apiVersion = apiversion;

        const char *extensions[sizeof(OVulkan::extensions) / sizeof(OVulkan::extensions[0])]; // purely string representation of vulkan device extensions
        for (size_t i = 0; i < sizeof(OVulkan::extensions) / sizeof(OVulkan::extensions[0]); i++) {
           extensions[i] = OVulkan::extensions[i].name;
        }

        checkextensions(VK_NULL_HANDLE, OVulkan::extensions, sizeof(OVulkan::extensions) / sizeof(OVulkan::extensions[0]));

        for (size_t i = 0; i < sizeof(OVulkan::extensions) / sizeof(OVulkan::extensions[0]); i++) {
            ASSERT(OVulkan::extensions[i].optional || OVulkan::extensions[i].supported, "Required Vulkan extension %s is not present when needed for instance.\n", OVulkan::extensions[i].name);
        }

        VkValidationFeaturesEXT vinfo = { };
        vinfo.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
        static const VkValidationFeatureEnableEXT enable_features[3] = {
            VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
            VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
            VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
        };
        vinfo.enabledValidationFeatureCount = 3;
        vinfo.pEnabledValidationFeatures = enable_features;

        VkInstanceCreateInfo instancecreate = { };
        instancecreate.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instancecreate.pNext = &vinfo;
        instancecreate.flags = 0;
        instancecreate.pApplicationInfo = &info;
        instancecreate.enabledLayerCount = 1;
        const char *validation[] = { "VK_LAYER_KHRONOS_validation" };
        instancecreate.ppEnabledLayerNames = validation;
        instancecreate.enabledExtensionCount = sizeof(OVulkan::extensions) / sizeof(OVulkan::extensions[0]);
        instancecreate.ppEnabledExtensionNames = extensions;

        res = vkCreateInstance(&instancecreate, NULL, &this->instance);
        ASSERT(res == VK_SUCCESS, "Failed to create instance with error %d.\n", res);

#define VK_IMPORT_INSTANCE_FUNC(OPT, FUNC) \
        FUNC = (PFN_##FUNC)vkGetInstanceProcAddr(this->instance, #FUNC); \
        ASSERT(OPT || FUNC != NULL, "Failed to import required function %s from Vulkan instance.\n", #FUNC);
    VK_IMPORT_INSTANCE
#undef VK_IMPORT_INSTANCE_FUNC

        uint32_t phycount = 0;
        res = vkEnumeratePhysicalDevices(this->instance, &phycount, NULL);
        ASSERT(res == VK_SUCCESS, "Failed to enumerate physical devices with error %d.\n", res);

        VkPhysicalDevice phys[4];
        phycount = glm::min((size_t )phycount, sizeof(phys) / sizeof(phys[0])); // make sure we only ever hit the right amount
        res = vkEnumeratePhysicalDevices(this->instance, &phycount, phys);
        ASSERT(res == VK_SUCCESS, "Failed to enumerate physical devices with error %d.\n", res);

        uint32_t phyidx = UINT32_MAX;
        uint32_t fallbackphyidx = UINT32_MAX;

        for (size_t i = 0; i < phycount; i++) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(phys[i], &props);

            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                fallbackphyidx = i;
            } else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                phyidx = i;
            }
        }

        if (phyidx == UINT32_MAX) {
            phyidx = fallbackphyidx == UINT32_MAX ? 0 : fallbackphyidx;
        }

        this->phy = phys[phyidx];
        vkGetPhysicalDeviceProperties(this->phy, &this->phyprops);

        OUtils::print("Using Vulkan compatible device %s (Vulkan API: %d.%d.%d)\n", this->phyprops.deviceName, VK_API_VERSION_MAJOR(this->phyprops.apiVersion), VK_API_VERSION_MINOR(this->phyprops.apiVersion), VK_API_VERSION_PATCH(this->phyprops.apiVersion));

        vkGetPhysicalDeviceFeatures(this->phy, &this->phyfeatures); // Get all to enable all
        VkPhysicalDeviceShaderDrawParametersFeatures shaderdraw = { };
        shaderdraw.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETER_FEATURES;
        shaderdraw.pNext = NULL;
        // shaderdraw.shaderDrawParameters = VK_TRUE;

        VkPhysicalDeviceDescriptorIndexingFeatures indexing = { };
        indexing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
        indexing.pNext = &shaderdraw;

        VkPhysicalDeviceBufferDeviceAddressFeatures bda = { };
        bda.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
        bda.pNext = &indexing;
        // we need this.
        // bda.bufferDeviceAddress = VK_TRUE;
        // we don't need these.
        // bda.bufferDeviceAddressMultiDevice = VK_FALSE;
        // bda.bufferDeviceAddressCaptureReplay = VK_FALSE;


        VkPhysicalDeviceFeatures2 extrafeatures = { };
        extrafeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        extrafeatures.pNext = &bda;

        vkGetPhysicalDeviceFeatures2(this->phy, &extrafeatures);
        ASSERT(shaderdraw.shaderDrawParameters == VK_TRUE, "Vulkan physical device does not have the required shader draw parameter feature.\n");
        ASSERT(bda.bufferDeviceAddress == VK_TRUE, "Vulkan physical device does not have the required buffer device address feature.\n");
        ASSERT(indexing.runtimeDescriptorArray == VK_TRUE, "Vulkan physical device does not have the required descriptor indexing features.\n");
        ASSERT(indexing.descriptorBindingPartiallyBound == VK_TRUE, "Vulkan physical device does not have the required descriptor indexing features.\n");
        ASSERT(indexing.shaderSampledImageArrayNonUniformIndexing == VK_TRUE, "Vulkan physical device does not have the required descriptor indexing features.\n");
        ASSERT(indexing.shaderStorageBufferArrayNonUniformIndexing == VK_TRUE, "Vulkan physical device does not have the required descriptor indexing features.\n");

        checkextensions(this->phy, devextensions, sizeof(devextensions) / sizeof(devextensions[0]));

        // TODO: Score based system for picking GPU based on supported extensions?
        for (size_t i = 0; i < sizeof(devextensions) / sizeof(devextensions[0]); i++) {
            ASSERT(devextensions[i].optional || devextensions[i].supported, "Required Vulkan extension %s is not present when needed by the Omicron Vulkan backend.\n", devextensions[i].name);
        }

#ifdef __linux__
        res = VK_ERROR_INITIALIZATION_FAILED;
        if (vkCreateXlibSurfaceKHR != NULL) {
            VkXlibSurfaceCreateInfoKHR surfacecreate = { };
            surfacecreate.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
            surfacecreate.pNext = NULL;
            surfacecreate.flags = 0;
            surfacecreate.dpy = (Display *)init->platform.ndt;
            surfacecreate.window = (Window)init->platform.nwh;
            res = vkCreateXlibSurfaceKHR(this->instance, &surfacecreate, NULL, &this->surface);
        }

        if (res != VK_SUCCESS) {
            void *xcblib = dlopen("libX11-xcb.so.1", RTLD_NOW);

            if (xcblib != NULL && vkCreateXcbSurfaceKHR != NULL) {
                VkXcbSurfaceCreateInfoKHR surfacecreate = { };
                surfacecreate.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
                surfacecreate.pNext = NULL;
                surfacecreate.flags = 0;
                typedef xcb_connection_t *(*PFN_XGetXCBConnection)(Display *);
                PFN_XGetXCBConnection getconnection = (PFN_XGetXCBConnection)dlsym(xcblib, "XGetXCBConnection");
                surfacecreate.connection = getconnection((Display *)init->platform.ndt);
                union {
                    void *ptr;
                    xcb_window_t window;
                } cast = { init->platform.nwh };
                surfacecreate.window = cast.window;
                res = vkCreateXcbSurfaceKHR(this->instance, &surfacecreate, NULL, &this->surface);
                dlclose(xcblib);
            }
        }
#endif
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan surface %d.\n", res);

        uint32_t queuefamilycount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(this->phy, &queuefamilycount, NULL);

        VkQueueFamilyProperties *queueprops = (VkQueueFamilyProperties *)malloc(sizeof(VkQueueFamilyProperties) * queuefamilycount);
        ASSERT(queueprops != NULL, "Failed to allocate memory for queue family properties.\n");
        vkGetPhysicalDeviceQueueFamilyProperties(this->phy, &queuefamilycount, queueprops);

        bool asyncqueues = false; // A queue family exists separate from the main queue that can handle compute and transfer asynchronously
        bool separatecomputeasync = false; // Async queue family has enough queues to be able to dedicate a queue to compute and transfer seperately.
        bool presentseparate = false; // Present queue can be separated from the compute and transfer async queues.
        bool presentoncompute = false; // Present queue is on the compute queue.
        bool presentontransfer = false; // Present queue is on the transfer queue.
        bool uniquecompute = false;
        bool uniquetransfer = false;
        bool uniquepresent = false;

        bool foundinitial = false; // We know what our initial queue will be.
        bool presentoninitial = false; // Present queue must be shared with the initial queue.

        size_t initialfamily = SIZE_MAX;
        size_t transferfamily = SIZE_MAX;
        size_t computefamily = SIZE_MAX;
        size_t presentfamily = SIZE_MAX;

        for (size_t i = 0; i < queuefamilycount; i++) {
            VkQueueFamilyProperties *props = &queueprops[i];

            // Preliminary check
            VkBool32 presentsupported = VK_FALSE;
            res = vkGetPhysicalDeviceSurfaceSupportKHR(this->phy, i, this->surface, &presentsupported);
            ASSERT(res == VK_SUCCESS, "Failed to find support information for queue family %lu.\n", i);

            if ((props->queueFlags & VK_QUEUE_GRAPHICS_BIT) && (props->queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                if (!foundinitial) { // This will be used as the initial queue.
                    foundinitial = true;
                    initialfamily = i;

                    if (presentsupported && !uniquepresent && !presentseparate) {
                        presentoninitial = true; // Present *is* supported on initial.
                        presentfamily = i; // Figure out later if we should actually be doing this on this queue family.
                    }
                }
            } else if ((props->queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                if (props->queueFlags & VK_QUEUE_TRANSFER_BIT) { // Queue family supports transfer as well
                    asyncqueues = true; // Found both async queues.

                    if (props->queueCount >= 2) { // This family supports enough queues for both transfer and compute asynchronously.
                        separatecomputeasync = true;
                        transferfamily = i;
                        computefamily = i;
                        if (props->queueCount >= 3) {
                            if (presentsupported && !uniquepresent) { // This means we have enough extra space for the present queue to fit separate as well.
                                presentseparate = true; // Enough for a separate queue.
                                presentoninitial = false;
                                presentfamily = i;
                            }
                        }
                    } else {
                        // Set anyway but keep in mind that we'll need to share! (unless something better comes along later)
                        transferfamily = i;
                        computefamily = i;

                        // If present is supported, we open up the possibility of sharing the present queue with compute and transfer.
                        if (!uniquepresent) {
                            presentoncompute = presentsupported;
                            presentontransfer = presentsupported;
                        }
                    }
                } else {
                    // Open up the possibility of putting the compute queue on here.
                    if (!uniquepresent) {
                        presentoncompute = presentsupported;
                    }
                    uniquecompute = true;
                    computefamily = i; // Set up our async compute family anyway.
                }
            } else if ((props->queueFlags & VK_QUEUE_TRANSFER_BIT)) {
                if (separatecomputeasync) {
                    continue;
                }
                // We found a transfer queue.
                transferfamily = i;
                uniquetransfer = true;

                // Open up the possibility of putting the present queue on here.
                if (!uniquepresent) {
                    presentontransfer = presentsupported;
                }
            } else if (presentsupported) { // Exclusive queue just for presentation!
                uniquepresent = true;
                presentfamily = i; // prefer this above everything else
            }

        }
        free(queueprops);

        // TODO: More sophisticated! On some systems compute and graphics queues are set up differently or may require other special tricks. In that case we must introduce special cases to accomodate for correct queue creation. References to device queues themselves are not a problem as we can have a multiple references to the same queue for a number of different uses all at once. On the "user" end it all acts the same, but the GPU will end up bunching operations together sequentially or otherwise depending on the queue setup.
        // Ideally we'd have a number of queues:
        // - Typically queue 0 is Compute + Graphics + Transfer
        // - Present queue
        // - Async transfer queue
        // - Async compute queue
        // We are guaranteed the first queue to be as we expect it but other queues may share the same family. In an ideal scenario, the async transfer, async compute, and present queues would all be separate. However, it is not uncommon to find hardware that doesn't have the required number of possible queues available for allocation from the "async queue family"
        // The benefits of having separate async queues is pretty clear, we can keep operations as concurrent as possible and avoid stalls where we have to wait for (ie.) a combined transfer+compute queue to finish its work on texture uploads only for the compute work to be performed.

        std::vector<VkDeviceQueueCreateInfo> queuecreate;
        // Use a size of three here as that's most a single create struct will use.
        float prio[3] = { 1.0f, 1.0f, 1.0f };
        ASSERT(foundinitial, "Failed to find a suitable queue family for main compute+rasterise setup!\n");

        VkDeviceQueueCreateInfo queueinfo = { };
        queueinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueinfo.pNext = NULL;
        queueinfo.queueCount = 1;
        queueinfo.queueFamilyIndex = initialfamily;
        queueinfo.pQueuePriorities = prio;
        queueinfo.flags = 0;
        queuecreate.push_back(queueinfo);

        if (separatecomputeasync) { // compute and async are separated, and the are async
            if (computefamily == transferfamily) {
                queueinfo.queueCount = 2;
                queueinfo.queueFamilyIndex = computefamily;
                if (presentfamily == transferfamily) {
                    queueinfo.queueCount = 3;
                }
                queuecreate.push_back(queueinfo);
            }
        } else if (asyncqueues) { // async queues, but they share
            queueinfo.queueCount = 1;
            queueinfo.queueFamilyIndex = transferfamily;
            queuecreate.push_back(queueinfo);
        } else {
            if (uniquecompute) {
                queueinfo.queueCount = 1;
                queueinfo.queueFamilyIndex = computefamily;
                queuecreate.push_back(queueinfo);
            }

            if (uniquetransfer) {
                queueinfo.queueCount = 1;
                queueinfo.queueFamilyIndex = transferfamily;
                queuecreate.push_back(queueinfo);
            }

        }
        if (uniquepresent) {
            queueinfo.queueCount = 1;
            queueinfo.queueFamilyIndex = presentfamily;
            queuecreate.push_back(queueinfo);
        }


        // float queueprio[3] = { 1.0f, 1.0f, 1.0f };
        // VkDeviceQueueCreateInfo queuecreate[3];
        // queuecreate[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        // queuecreate[0].pNext = NULL;
        // queuecreate[0].queueCount = 1;
        // queuecreate[0].queueFamilyIndex = this->graphicscomputefamily;
        // queuecreate[0].pQueuePriorities = queueprio;
        // queuecreate[0].flags = 0;
        //
        // queuecreate[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        // queuecreate[1].pNext = NULL;
        // queuecreate[1].queueCount = 1;
        // queuecreate[1].queueFamilyIndex = this->presentfamily;
        // queuecreate[1].pQueuePriorities = queueprio;
        // queuecreate[1].flags = 0;
        //
        // queuecreate[2].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        // queuecreate[2].pNext = NULL;
        // queuecreate[2].queueCount = 1;
        // queuecreate[2].queueFamilyIndex = this->transferfamily;
        // queuecreate[2].pQueuePriorities = queueprio;
        // queuecreate[2].flags = 0;

        VkDeviceCreateInfo devicecreate = { };
        devicecreate.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        devicecreate.pNext = &bda; // Pass on extra enabled features.
        devicecreate.pQueueCreateInfos = queuecreate.data();
        devicecreate.queueCreateInfoCount = queuecreate.size();
        devicecreate.pEnabledFeatures = &this->phyfeatures;
        devicecreate.enabledExtensionCount = sizeof(devextensions) / sizeof(devextensions[0]);

        const char *devextensions[sizeof(OVulkan::devextensions) / sizeof(OVulkan::devextensions[0])]; // purely string representation of vulkan device extensions
        for (size_t i = 0; i < sizeof(OVulkan::devextensions) / sizeof(OVulkan::devextensions[0]); i++) {
           devextensions[i] = OVulkan::devextensions[i].name;
        }
        devicecreate.ppEnabledExtensionNames = devextensions;
        devicecreate.enabledLayerCount = 0;

        res = vkCreateDevice(this->phy, &devicecreate, NULL, &this->dev);
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan device %d.\n", res);

#define VK_IMPORT_DEVICE_FUNC(OPT, FUNC) \
        FUNC = (PFN_##FUNC)vkGetDeviceProcAddr(this->dev, #FUNC); \
        ASSERT(OPT || FUNC != NULL, "Failed to import required Vulkan device function %s.\n", #FUNC);
    VK_IMPORT_DEVICE
#undef VK_IMPORT_DEVICE_FUNC

        this->vmafunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
        this->vmafunctions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
        this->vmafunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
        this->vmafunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
        this->vmafunctions.vkAllocateMemory = vkAllocateMemory;
        this->vmafunctions.vkFreeMemory = vkFreeMemory;
        this->vmafunctions.vkCreateImage = vkCreateImage;
        this->vmafunctions.vkDestroyImage = vkDestroyImage;
        this->vmafunctions.vkCreateBuffer = vkCreateBuffer;
        this->vmafunctions.vkDestroyBuffer = vkDestroyBuffer;
        this->vmafunctions.vkCmdCopyBuffer = vkCmdCopyBuffer;
        this->vmafunctions.vkMapMemory = vkMapMemory;
        this->vmafunctions.vkUnmapMemory = vkUnmapMemory;
        this->vmafunctions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
        this->vmafunctions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
        this->vmafunctions.vkBindBufferMemory = vkBindBufferMemory;
        this->vmafunctions.vkBindImageMemory = vkBindImageMemory;

        VmaAllocatorCreateInfo alloccreate = { };
        // alloccreate.vulkanApiVersion = VK_VERSION_1_0;
        alloccreate.physicalDevice = this->phy;
        alloccreate.device = this->dev;
        alloccreate.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT; // We use this feature, so enable it.
        alloccreate.instance = this->instance;
        alloccreate.pVulkanFunctions = &this->vmafunctions;
        vmaCreateAllocator(&alloccreate, &this->allocator);

        this->initialfamily = initialfamily;
        this->transferfamily = transferfamily;
        this->computefamily = computefamily;
        this->presentfamily = presentfamily;
        vkGetDeviceQueue(this->dev, initialfamily, 0, &this->graphicsqueue);
        vkGetDeviceQueue(this->dev, initialfamily, 0, &this->computequeue);
        VkDebugUtilsObjectNameInfoEXT nameinfo = { };
        nameinfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameinfo.pNext = NULL;
        nameinfo.objectType = VK_OBJECT_TYPE_QUEUE;
        nameinfo.objectHandle = (uint64_t)this->graphicsqueue;
        nameinfo.pObjectName = "Graphics / Compute Queue";
        vkSetDebugUtilsObjectNameEXT(this->dev, &nameinfo);
        if (presentseparate) {
            // Present separate on the last one.
            vkGetDeviceQueue(this->dev, presentfamily, 2, &this->presentqueue);
        } else if (presentoninitial && !presentoncompute && !presentontransfer) {
            vkGetDeviceQueue(this->dev, presentfamily, 0, &this->presentqueue);
        } else if (presentontransfer) {
            vkGetDeviceQueue(this->dev, transferfamily, 0, &this->presentqueue);
        } else if (presentoncompute) {
            vkGetDeviceQueue(this->dev, computefamily, 0, &this->presentqueue);
        } else if (uniquepresent) { // One for present.
            vkGetDeviceQueue(this->dev, presentfamily, 0, &this->presentqueue);
        }

        if (!presentoninitial) {
            nameinfo.objectHandle = (uint64_t)this->presentqueue;
            nameinfo.pObjectName = "Present Queue";
          vkSetDebugUtilsObjectNameEXT(this->dev, &nameinfo);
        }

        if (separatecomputeasync) { // Separate queues.
            this->asyncqueues = true;
            vkGetDeviceQueue(this->dev, computefamily, 0, &this->asynccompute);
            vkGetDeviceQueue(this->dev, transferfamily, 1, &this->asynctransfer);
            nameinfo.objectHandle = (uint64_t)this->asynctransfer;
            nameinfo.pObjectName = "Transfer Queue";
            vkSetDebugUtilsObjectNameEXT(this->dev, &nameinfo);
        } else if (uniquecompute) { // One for compute.
            vkGetDeviceQueue(this->dev, computefamily, 0, &this->asynccompute);
            this->asyncqueues = true;
        } else if (uniquetransfer) { // One for transfer.
            this->asyncqueues = true;
            vkGetDeviceQueue(this->dev, transferfamily, 0, &this->asynctransfer);
        } else if (asyncqueues) {
            this->asyncqueues = true;
            // Only one queue shared for all async ops.
            vkGetDeviceQueue(this->dev, computefamily, 0, &this->asynccompute);
            vkGetDeviceQueue(this->dev, transferfamily, 0, &this->asynctransfer);
        } else { // All on initial
            vkGetDeviceQueue(this->dev, initialfamily, 0, &this->asynccompute);
            vkGetDeviceQueue(this->dev, initialfamily, 0, &this->asynctransfer);
            this->computefamily  = initialfamily;
            this->transferfamily = initialfamily;
        }

        printf("%u initial %u transfer\n", this->initialfamily, this->transferfamily);

        uint32_t formatcount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(this->phy, this->surface, &formatcount, NULL);
        VkSurfaceFormatKHR *formats = (VkSurfaceFormatKHR *)malloc(sizeof(VkSurfaceFormatKHR) * formatcount);
        ASSERT(formats != NULL, "Failed to allocate memory for Vulkan surface supported formats.\n");
        vkGetPhysicalDeviceSurfaceFormatsKHR(this->phy, this->surface, &formatcount, formats);
        free(formats); // XXX: Actually do something with these

        this->createswapchain();
        this->createswapchainviews();

        // XXX: Create pools for async queues!!!
        VkCommandPoolCreateInfo poolcreate = { };
        poolcreate.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolcreate.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolcreate.queueFamilyIndex = this->initialfamily;
        poolcreate.pNext = NULL;

        res = vkCreateCommandPool(this->dev, &poolcreate, NULL, &this->cmdpool);
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan command pool %d.\n", res);

        // Create pools for async transfer.
        poolcreate.queueFamilyIndex = this->computefamily;
        res = vkCreateCommandPool(this->dev, &poolcreate, NULL, &this->computepool);
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan command pool %d.\n", res);
        poolcreate.queueFamilyIndex = this->transferfamily;
        res = vkCreateCommandPool(this->dev, &poolcreate, NULL, &this->transferpool);
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan command pool %d.\n", res);

        // create all command buffers
        VkCommandBufferAllocateInfo cmdalloc = { };
        cmdalloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdalloc.commandPool = this->cmdpool;
        cmdalloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdalloc.commandBufferCount = RENDERER_MAXLATENCY + 1;
        res = vkAllocateCommandBuffers(this->dev, &cmdalloc, this->cmd);
        ASSERT(res == VK_SUCCESS, "Failed to allocate Vulkan command buffer %d.\n", res);

        nameinfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameinfo.pNext = NULL;
        nameinfo.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
        for (size_t i = 0; i < RENDERER_MAXLATENCY; i++) {
            char name[64];
            nameinfo.objectHandle = (uint64_t)this->cmd[i];
            snprintf(name, 64, "Frame %lu Command Buffer", i);
            nameinfo.pObjectName = name;
            vkSetDebugUtilsObjectNameEXT(this->dev, &nameinfo);
            this->stream[i].cmd = this->cmd[i]; // Give the streams a handle to our command buffer. We can now reference it both through the stream and through this->cmd.
            this->stream[i].type = ORenderer::STREAM_FRAME;
            this->stream[i].context = this;
        }

        VkCommandBufferAllocateInfo imalloc = { };
        imalloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        imalloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        imalloc.commandBufferCount = 1;
        imalloc.commandPool = this->cmdpool;
        res = vkAllocateCommandBuffers(this->dev, &imalloc, &this->imcmd);
        ASSERT(res == VK_SUCCESS, "Failed to allocate Vulkan immediate submission command buffer %d.\n", res);

        this->imstream.context = this;
        this->imstream.extra = true;
        this->imstream.type = ORenderer::STREAM_IMMEDIATE;
        this->imstream.cmd = this->imcmd;

        // Initialise the stream pool.
        this->streampool.init(this);

        VkSemaphoreCreateInfo semcreate = { };
        semcreate.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fencecreate = { };
        fencecreate.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fencecreate.flags = VK_FENCE_CREATE_SIGNALED_BIT; // initialise as signalled

        for (size_t i = 0; i < RENDERER_MAXLATENCY; i++) {
            ASSERT(vkCreateSemaphore(this->dev, &semcreate, NULL, &this->imagepresent[i]) == VK_SUCCESS && vkCreateSemaphore(this->dev, &semcreate, NULL, &this->renderdone[i]) == VK_SUCCESS, "Failed to create Vulkan semaphores!\n");
            ASSERT(vkCreateFence(this->dev, &fencecreate, NULL, &this->framesinflight[i]) == VK_SUCCESS, "Failed to create Vulkan fence!\n");
        }
        ASSERT(vkCreateFence(this->dev, &fencecreate, NULL, &this->imfence) == VK_SUCCESS, "Failed to create Vulkan fence!\n");

        VkDescriptorPoolSize poolsizes[] = {
            (VkDescriptorPoolSize) {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = VULKAN_MAXDESCRIPTORS * 2,
            },
            (VkDescriptorPoolSize) {
                .type = VK_DESCRIPTOR_TYPE_SAMPLER,
                .descriptorCount = VULKAN_MAXDESCRIPTORS * 16
            },
            (VkDescriptorPoolSize) {
                .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .descriptorCount = VULKAN_MAXDESCRIPTORS * 16
            },
            (VkDescriptorPoolSize) {
                .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = VULKAN_MAXDESCRIPTORS * 16
            },
            (VkDescriptorPoolSize) {
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = VULKAN_MAXDESCRIPTORS * 16
            }
        };

        VkDescriptorPoolCreateInfo descpoolcreate = { };
        descpoolcreate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descpoolcreate.pNext = NULL;
        descpoolcreate.poolSizeCount = sizeof(poolsizes) / sizeof(poolsizes[0]);
        descpoolcreate.pPoolSizes = poolsizes; // describe which pools are part of our pool
        descpoolcreate.maxSets = VULKAN_MAXDESCRIPTORS;
        descpoolcreate.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        res = vkCreateDescriptorPool(this->dev, &descpoolcreate, NULL, &this->descpool);
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan descriptor pool %d.\n", res);

        for (size_t i = 0; i < RENDERER_MAXLATENCY; i++) {
            this->scratchbuffers[i].create(this, 128, RENDERER_MAXDRAWCALLS, this->phyprops.limits.minUniformBufferOffsetAlignment);
        }

#ifdef OMICRON_DEBUG
        this->tracyctx = TracyVkContextCalibrated(this->instance, this->phy, this->dev, this->graphicsqueue, this->cmd[RENDERER_MAXLATENCY], vkGetInstanceProcAddr, vkGetDeviceProcAddr);
#endif
        // for (size_t i = 0; i < RENDERER_MAXLATENCY; i++) {
            // this->cmdctx[i] = TracyVkContextCalibrated(this->instance, this->phy, this->dev, this->graphicsqueue, this->cmd[i], vkGetInstanceProcAddr, vkGetDeviceProcAddr);
        // }

        ORenderer::setmanager.init(this);
    }

    void VulkanStreamPool::init(VulkanContext *ctx) {
        this->context = ctx;
        this->freecompute = &this->computepool[0];
        this->freetransfer = &this->transferpool[0];
        this->freeim = &this->impool[0];

        VkCommandBufferAllocateInfo info = { };
        info.pNext = NULL;
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool = ctx->computepool;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandBufferCount = VULKAN_POOLSIZE;
        VkResult res = vkAllocateCommandBuffers(ctx->dev, &info, this->computecmds);
        ASSERT(res == VK_SUCCESS, "Failed to allocate Vulkan stream command buffer pool.\n");
        info.commandPool = ctx->transferpool;
        res = vkAllocateCommandBuffers(ctx->dev, &info, this->transfercmds);
        ASSERT(res == VK_SUCCESS, "Failed to allocate Vulkan stream command buffer pool.\n");
        info.commandPool = ctx->cmdpool;
        res = vkAllocateCommandBuffers(ctx->dev, &info, this->imcmds);
        ASSERT(res == VK_SUCCESS, "Failed to allocate Vulkan stream command buffer pool.\n");

        VkFenceCreateInfo fenceinfo = { };
        fenceinfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceinfo.pNext = NULL;
        fenceinfo.flags = 0;

        VkSemaphoreCreateInfo seminfo = { };
        seminfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        seminfo.pNext = NULL;
        seminfo.flags = 0;

        for (size_t i = 0; i < VULKAN_POOLSIZE * 3; i++) {
            ASSERT(vkCreateFence(ctx->dev, &fenceinfo, NULL, &this->fences[i]) == VK_SUCCESS, "Failed to create Vulkan fence!\n");
            VkDebugUtilsObjectNameInfoEXT nameinfo = { };
            nameinfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameinfo.pNext = NULL;
            nameinfo.objectHandle = (uint64_t)this->fences[i];
            nameinfo.objectType = VK_OBJECT_TYPE_FENCE;
            char name[64];
            snprintf(name, 64, "Pool Fence %lu\n", i);
            nameinfo.pObjectName = name;
            vkSetDebugUtilsObjectNameEXT(this->context->dev, &nameinfo);
            ASSERT(vkCreateSemaphore(ctx->dev, &seminfo, NULL, &this->semaphores[i]) == VK_SUCCESS, "Failed to create Vulkan semaphore!\n");
            nameinfo.objectType = VK_OBJECT_TYPE_SEMAPHORE;
            nameinfo.objectHandle = (uint64_t)this->semaphores[i];
            snprintf(name, 64, "Pool Semaphore %lu", i);
            nameinfo.pObjectName = name;
            vkSetDebugUtilsObjectNameEXT(this->context->dev, &nameinfo);
        }

        VkDebugUtilsObjectNameInfoEXT nameinfo = { };
        nameinfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameinfo.pNext = NULL;
        nameinfo.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;

        for (size_t i = 0; i < VULKAN_POOLSIZE - 1; i++) {
            nameinfo.pObjectName = "Transfer Command Buffer";
            nameinfo.objectHandle = (uint64_t)this->transfercmds[i];
            vkSetDebugUtilsObjectNameEXT(this->context->dev, &nameinfo);
            this->transferpool[i].cmd = this->transfercmds[i];
            this->transferpool[i].context = ctx;
            this->transferpool[i].type = ORenderer::STREAM_TRANSFER;
            this->transferpool[i].next = &this->transferpool[i + 1];
            this->transferpool[i].fence = this->fences[i];
            this->transferpool[i].semaphore = this->semaphores[i];

            nameinfo.pObjectName = "Compute Command Buffer";
            nameinfo.objectHandle = (uint64_t)this->computecmds[i];
            vkSetDebugUtilsObjectNameEXT(this->context->dev, &nameinfo);
            this->computepool[i].cmd = this->computecmds[i];
            this->computepool[i].context = ctx;
            this->computepool[i].type = ORenderer::STREAM_COMPUTE;
            this->computepool[i].next = &this->computepool[i + 1];
            this->computepool[i].fence = this->fences[i + VULKAN_POOLSIZE];
            this->computepool[i].semaphore = this->semaphores[i + VULKAN_POOLSIZE];

            nameinfo.pObjectName = "Immediate Command Buffer";
            nameinfo.objectHandle = (uint64_t)this->imcmds[i];
            vkSetDebugUtilsObjectNameEXT(this->context->dev, &nameinfo);
            this->impool[i].cmd = this->imcmds[i];
            this->impool[i].context = ctx;
            this->impool[i].type = ORenderer::STREAM_IMMEDIATE;
            this->impool[i].next = &this->impool[i + 1];
            this->impool[i].fence = this->fences[i + (VULKAN_POOLSIZE * 2)];
            this->impool[i].semaphore = this->semaphores[i + (VULKAN_POOLSIZE * 2)];
        }

        this->transferpool[VULKAN_POOLSIZE - 1].next = NULL;
        this->computepool[VULKAN_POOLSIZE - 1].next = NULL;
        this->impool[VULKAN_POOLSIZE - 1].next = NULL;
    }

    void VulkanStreamPool::destroy(void) {
        vkFreeCommandBuffers(this->context->dev, this->context->computepool, VULKAN_POOLSIZE, this->computecmds);
        vkFreeCommandBuffers(this->context->dev, this->context->transferpool, VULKAN_POOLSIZE, this->transfercmds);
        vkFreeCommandBuffers(this->context->dev, this->context->cmdpool, VULKAN_POOLSIZE, this->imcmds);
        for (size_t i = 0; i < VULKAN_POOLSIZE * 3; i++) {
            vkDestroyFence(this->context->dev, this->fences[i], NULL);
            vkDestroySemaphore(this->context->dev, this->semaphores[i], NULL);
        }
    }

    uint64_t VulkanContext::getbufferref(struct ORenderer::buffer buffer, uint8_t latency) {
        // XXX: Do this only at creation to avoid potential costs of the query if we do it regularly.
        VkBufferDeviceAddressInfo info = { };
        this->resourcemutex.lock();
        info.buffer = this->buffers[buffer.handle].vkresource.buffer[latency == UINT8_MAX ? this->frame : latency];
        info.pNext = NULL;
        info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        VkDeviceAddress addr = vkGetBufferDeviceAddressKHR(this->dev, &info);
        this->resourcemutex.unlock();
        return addr;
    }

    VulkanStream *VulkanStreamPool::alloc(uint8_t type) {
        ASSERT(
            type == ORenderer::STREAM_TRANSFER ||
            type == ORenderer::STREAM_COMPUTE ||
            type == ORenderer::STREAM_IMMEDIATE,
            "Invalid stream type for pool allocation %u.\n", type);
        VulkanStream **freestream = // Figure out what stream is being requested, this way prevents code duplication.
            type == ORenderer::STREAM_TRANSFER ? &this->freetransfer :
            type == ORenderer::STREAM_COMPUTE ? &this->freecompute : &this->freeim;
        ASSERT(*freestream != NULL, "Empty Vulkan stream pool.\n");

        this->spin.lock();

        VulkanStream *stream = *freestream;
        *freestream = stream->next;

        this->spin.unlock();

        return stream;
    }

    void VulkanStreamPool::free(VulkanStream *stream) {
        ASSERT(stream != NULL, "Expected active stream, not NULL.\n");

        VulkanStream **freestream = // Figure out what stream is being requested, this way prevents code duplication.
            stream->type == ORenderer::STREAM_TRANSFER ? &this->freetransfer :
            stream->type == ORenderer::STREAM_COMPUTE ? &this->freecompute : &this->freeim;

        this->spin.lock();
        stream->next = *freestream;
        *freestream = stream;
        this->spin.unlock();
    }

    void VulkanContext::execute(GraphicsPipeline *pipeline, void *cam) {
        ASSERT(pipeline != NULL, "Invalid pipeline.\n");
        ASSERT(cam != NULL, "Invalid camera.\n");
        ASSERT(this->frame < RENDERER_MAXLATENCY, "Invalid current frame!\n");
        vkWaitForFences(this->dev, 1, &this->framesinflight[this->frame], VK_TRUE, UINT64_MAX); // await frame completion (of previous version, if we recurse over our allowed latency we'll end up waiting for the original to complete)

        uint32_t image;
        VkResult res = vkAcquireNextImageKHR(this->dev, this->swapchain, UINT64_MAX, this->imagepresent[this->frame], VK_NULL_HANDLE, &image); // get the next image from our swapchain in order to render it
        if (res == VK_ERROR_OUT_OF_DATE_KHR) { // swapchain was invalidated (resized, etc.)
            // vulkan_recreateswap();
            vkDeviceWaitIdle(this->dev); // wait until all work is done!

            this->destroyswapchain();
            this->createswapchain();
            this->createswapchainviews();
            this->createbackbuffer(this->swapfbpass);
            return;
        } else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) { // suboptimal is still usable
            ASSERT(false, "Failed to acquire next Vulkan swapchain image %d.\n", res);
        }
        vkResetFences(this->dev, 1, &this->framesinflight[this->frame]); // reset fence
        this->swapimage = image;


        // vkResetCommandBuffer(this->cmd[this->frame], 0);
        this->stream[this->frame].flushcmd();
        this->scratchbuffers[this->frame].reset(); // Reset scratchbuffers.

        // printf("pipeline begin.\n");
        ASSERT(pipeline != NULL, "It's so over!\n");
        pipeline->execute(&this->stream[this->frame], cam);
        // printf("pipeline end.\n");

        // this->recordcmd(this->cmd[this->frame], image, &stream[this->frame]);

        VkSubmitInfo submit = { };
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore *waitsems = (VkSemaphore *)malloc(sizeof(VkSemaphore) * (1 + this->stream[this->frame].waiton.size()));
        VkPipelineStageFlags *waitstages = (VkPipelineStageFlags *)malloc(sizeof(VkPipelineStageFlags) * (1 + this->stream[this->frame].waiton.size()));
        ASSERT(waitsems != NULL, "Failed to allocate memory for wait semaphores.\n");
        ASSERT(waitstages != NULL, "Failed to allocate memory for wait stages.\n");

        waitsems[0] = this->imagepresent[this->frame];
        waitstages[0] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        for (size_t i = 0; i < this->stream[this->frame].waiton.size(); i++) {
            waitsems[i + 1] = this->stream[this->frame].waiton[i]->semaphore;
            waitstages[i + 1] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        }

        submit.waitSemaphoreCount = 1 + this->stream[this->frame].waiton.size();
        submit.pWaitSemaphores = waitsems;
        submit.pWaitDstStageMask = waitstages;
        submit.commandBufferCount = 1;
        // submit.pCommandBuffers = &this->cmd[this->frame];
        submit.pCommandBuffers = &this->stream[this->frame].cmd;

        this->stream[this->frame].waiton.clear();

        VkSemaphore signalsems[] = { this->renderdone[this->frame] };
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = signalsems; // signal render done when we're done

        res = vkQueueSubmit(this->graphicsqueue, 1, &submit, this->framesinflight[this->frame]); // do all this and trigger the frameinflight fence to we know we can go ahead and render the next frame when we're done with the command buffers
        ASSERT(res == VK_SUCCESS, "Failed to submit Vulkan queue %d.\n", res);

        free(waitsems);
        free(waitstages);

        VkPresentInfoKHR presentinfo = { };
        presentinfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentinfo.pNext = NULL;
        presentinfo.waitSemaphoreCount = 1;
        presentinfo.pWaitSemaphores = signalsems; // wait on these signals before presenting (we require all our work to be done before we're allowed to blit to the screen)

        VkSwapchainKHR swapchains[] = { this->swapchain };
        presentinfo.swapchainCount = 1;
        presentinfo.pSwapchains = swapchains; // present to these swapchains
        presentinfo.pImageIndices = &image; // with this image
        presentinfo.pResults = NULL; // we don't want to check every swapchain suceeded
        {
            ZoneScopedN("Vulkan Queue Present");
            res = vkQueuePresentKHR(this->presentqueue, &presentinfo); // blit queue to swapchains
        }
        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
            // vulkan_recreateswap();
            vkDeviceWaitIdle(this->dev); // wait until all work is done!

            this->destroyswapchain();
            this->createswapchain();
            this->createswapchainviews();
            this->createbackbuffer(this->swapfbpass);
        } else if (res != VK_SUCCESS) { // suboptimal is also considered a success code
            ASSERT(false, "Failed to acquire present Vulkan swapchain queue %d.\n", res);
        }

        this->frame = (this->frame + 1) % RENDERER_MAXLATENCY;
        this->frameid.fetch_add(1);
    }

    ORenderer::RendererContext *createcontext(void) {
        return NULL;
    }

}
