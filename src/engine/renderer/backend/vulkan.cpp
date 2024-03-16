#include <engine/renderer/backend/vulkan.hpp>
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
        if (res == VK_SUCCESS && propcount > 0) {
            VkExtensionProperties *props = (VkExtensionProperties *)malloc(sizeof(VkExtensionProperties) * propcount);
            ASSERT(props != NULL, "Failed to allocate memory for Vulkan extension properties.\n");
            res = enumextensions(phy, NULL, &propcount, props);
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
        // { VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, false, false, false },
        { VK_KHR_SWAPCHAIN_EXTENSION_NAME, false, false, false },
        { VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME, false, false, false }
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
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // RENDERER_RESOURCESAMPLER
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, // RENDERER_RESOURCESTORAGEIMAGE
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // RENDERER_RESOURCEUNIFORM
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER // RENDERER_RESOURCESTORAGE
    };

    static VkVertexInputRate vulkan_inputratetable[] = {
        VK_VERTEX_INPUT_RATE_VERTEX, // RENDERER_RATEVERTEX
        VK_VERTEX_INPUT_RATE_INSTANCE // RENDERER_RATEINSTANCE
    };

    void ScratchBuffer::create(VulkanContext *ctx, size_t size, size_t count) {
        const size_t entsize = utils_stridealignment(size, ctx->phyprops.limits.minUniformBufferOffsetAlignment);

        // VkBufferCreateInfo buffercreate = { };
        // buffercreate.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        // buffercreate.pNext = NULL;
        // buffercreate.size = entsize * count;
        // buffercreate.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        // buffercreate.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        //
        // VmaAllocationCreateInfo allocinfo = { };
        // allocinfo.usage = VMA_MEMORY_USAGE_AUTO;
        // allocinfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        // allocinfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        //
        // VmaAllocation alloc;
        // VkResult res = vmaCreateBuffer(ctx->allocator, &buffercreate, &allocinfo, &this->buffer.buffer[0], &alloc, NULL);
        // 
        // if (res != VK_SUCCESS) { // if we fail to have unified memory, try again with ram instead
        //     allocinfo.requiredFlags &= ~ VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        //     res = vmaCreateBuffer(ctx->allocator, &buffercreate, &allocinfo, &this->buffer.buffer[0], &alloc, NULL);
        // }
        //
        // ASSERT(res == VK_SUCCESS, "Failed to allocate Vulkan buffer for scratchbuffer.\n");
        // VmaAllocationInfo info;
        // vmaGetAllocationInfo(ctx->allocator, alloc, &info);
        // this->buffer.memory[0] = info.deviceMemory;
        // this->buffer.offset[0] = info.offset;
        // this->buffer.allocation[0] = alloc;
        struct ORenderer::bufferdesc desc = { };
        desc.size = entsize * count;
        desc.usage = ORenderer::BUFFER_UNIFORM;
        // exists on the GPU itself but we allow the CPU to see that memory and pass data to it when flushing the ranges, we also need random access to the memory as we rarely are writing from start to finish in its entirety.
        desc.memprops = ORenderer::MEMPROP_GPULOCAL | ORenderer::MEMPROP_CPUVISIBLE | ORenderer::MEMPROP_CPURANDOMACCESS;
        ASSERT(ctx->createbuffer(&desc, &this->buffer) == ORenderer::RESULT_SUCCESS, "Failed to create scratch buffer.\n");

        this->size = entsize * count;
        this->pos = 0;

        struct ORenderer::buffermapdesc mapdesc = { };
        mapdesc.buffer = this->buffer;
        mapdesc.size = entsize * count;
        mapdesc.offset = 0;
        ASSERT(ctx->mapbuffer(&mapdesc, &this->map) == ORenderer::RESULT_SUCCESS, "Failed to map scratch buffer.\n");

    }

    void ScratchBuffer::destroy(void) {
        // vmaUnmapMemory(((VulkanContext *)ORenderer::context)->allocator, this->buffer.allocation[0]);
        // vmaDestroyBuffer(((VulkanContext *)ORenderer::context)->allocator, this->buffer.buffer[0], this->buffer.allocation[0]);
    }

    size_t ScratchBuffer::write(const void *data, size_t size) {
        const VulkanContext *ctx = (VulkanContext *)ORenderer::context;

        ASSERT(this->pos + size < this->size, "Not enough space for scratchbuffer write of %lu bytes.\n", size);
        size_t off = this->pos;
        
        memcpy(&((uint8_t *)this->map.mapped[0])[this->pos], data, size);

        this->pos += utils_stridealignment(size, ctx->phyprops.limits.minUniformBufferOffsetAlignment);

        return off;
    }

    void ScratchBuffer::flush(void) {
        VulkanContext *ctx = (VulkanContext *)ORenderer::context; 
        const size_t size = glm::min((size_t)utils_stridealignment(this->pos, ctx->phyprops.limits.minUniformBufferOffsetAlignment), this->size);

        VkMappedMemoryRange range = { };
        range.size = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.pNext = NULL;
        range.memory = ctx->buffers[this->buffer.handle].vkresource.memory[0];
        range.offset = 0;
        range.size = size;
        VkResult res = vkFlushMappedMemoryRanges(ctx->dev, 1, &range);
        ASSERT(res == VK_SUCCESS, "Failed to flush Vulkan memory range.\n");
    }

    static void vulkan_genmips(VkImage image, uint32_t width, uint32_t height, VkImageAspectFlags aspectmask, uint32_t mips, VkPipelineStageFlags stages) {
        VkCommandBuffer cmd = ((VulkanContext *)ORenderer::context)->beginonetimecmd();

        VkImageMemoryBarrier barrier = { };
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = aspectmask;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        uint32_t mwidth = width;
        uint32_t mheight = height;

        for (size_t i = 1; i < mips; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

            VkImageBlit blit = { };
            blit.srcOffsets[0] = { };
            blit.srcOffsets[1].x = mwidth;
            blit.srcOffsets[1].y = mheight;
            blit.srcOffsets[1].z = 1;
            blit.srcSubresource.aspectMask = aspectmask;
            blit.srcSubresource.mipLevel = i - 1; // use previous mip level as a basis for our current one
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {  };
            blit.dstOffsets[1].x = mwidth > 1 ? mwidth / 2 : 1;
            blit.dstOffsets[1].y = mheight > 1 ? mheight / 2 : 1;
            blit.dstOffsets[1].z = 1; // destination should be one size smaller
            blit.dstSubresource.aspectMask = aspectmask;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR); // blit self (with smaller size)
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, stages, 0, 0, NULL, 0, NULL, 1, &barrier);

            if (mwidth > 1) {
                mwidth /= 2;
            }
            if (mheight > 1) {
                mheight /= 2;
            }
        }

        barrier.subresourceRange.baseMipLevel = mips - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        // barrier transition for last mip level
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, stages, 0, 0, NULL, 0, NULL, 1, &barrier);

        ((VulkanContext *)ORenderer::context)->endonetimecmd(cmd);
    }

    VkCommandBuffer VulkanContext::beginonetimecmd(void) {
        VkCommandBufferAllocateInfo allocinfo = { };
        allocinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocinfo.pNext = NULL;
        allocinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocinfo.commandPool = this->cmdpool;
        allocinfo.commandBufferCount = 1;

        VkCommandBuffer cmd;
        VkResult res = vkAllocateCommandBuffers(this->dev, &allocinfo, &cmd);
        ASSERT(res == VK_SUCCESS, "Failed to allocate Vulkan command buffers %d.\n", res);

        VkCommandBufferBeginInfo begininfo = { };
        begininfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begininfo.pNext = NULL;
        begininfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(cmd, &begininfo);

        return cmd;
    }

    void VulkanContext::endonetimecmd(VkCommandBuffer cmd) {
        vkEndCommandBuffer(cmd);

        VkSubmitInfo submitinfo = { };
        submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitinfo.pNext = NULL;
        submitinfo.commandBufferCount = 1;
        submitinfo.pCommandBuffers = &cmd;

        vkQueueSubmit(this->graphicsqueue, 1, &submitinfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(this->graphicsqueue);

        vkFreeCommandBuffers(this->dev, this->cmdpool, 1, &cmd);
    }

    static void transitionlayout(VkCommandBuffer cmd, struct texture *texture, size_t format, size_t state) {
        VkImageMemoryBarrier barrier = { };
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = NULL;
        barrier.oldLayout = texture->state;
        barrier.newLayout = layouttable[state];
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = texture->image;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
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

        vkCmdPipelineBarrier(cmd, srcstagemask, dststagemask, 0, 0, NULL, 0, NULL, 1, &barrier);
        texture->state = layouttable[state];
    }

    uint8_t VulkanContext::transitionlayout(struct ORenderer::texture texture, size_t format, size_t state) {
        VkCommandBuffer cmd = this->beginonetimecmd();

        resourcemutex.lock();
        struct texture *vktexture = &textures[texture.handle].vkresource;
        OVulkan::transitionlayout(cmd, vktexture, format, state);
        resourcemutex.unlock();

        this->endonetimecmd(cmd);
        return ORenderer::RESULT_SUCCESS;
    }

    static void vulkan_copybuffertoimage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer cmd = ((VulkanContext *)ORenderer::context)->beginonetimecmd();

        VkBufferImageCopy region = { 0 };
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = (VkOffset3D) { 0 };
        region.imageExtent = (VkExtent3D) { width, height, 1 };

        vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        ((VulkanContext *)ORenderer::context)->endonetimecmd(cmd);
    }

    uint8_t VulkanContext::createtexture(struct ORenderer::texturedesc *desc, struct ORenderer::texture *texture) {
        if (desc->format >= ORenderer::FORMAT_COUNT) {
            return ORenderer::RESULT_INVALIDARG;
        } else if (desc->type >= ORenderer::IMAGETYPE_COUNT) {
            return ORenderer::RESULT_INVALIDARG;
        }

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
        VkResult res;

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

        VmaAllocation alloc;
        vmaCreateImage(this->allocator, &imagecreate, &allocinfo, &vktexture->image, &alloc, NULL);

        VmaAllocationInfo info;
        vmaGetAllocationInfo(this->allocator, alloc, &info);

        vktexture->offset = info.offset;
        vktexture->memory = info.deviceMemory;
        vktexture->allocation = alloc;

        this->resourcemutex.unlock();

        return ORenderer::RESULT_SUCCESS;
    }

    uint8_t VulkanContext::createbuffer(struct ORenderer::bufferdesc *desc, struct ORenderer::buffer *buffer) {
        if (desc->size <= 0) {
            return ORenderer::RESULT_INVALIDARG;
        }

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
            0;
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
        if (desc->attachmentcount <= 0) {
            return ORenderer::RESULT_INVALIDARG;
        } else if (desc->attachments == NULL) {
            return ORenderer::RESULT_INVALIDARG;
        } else if (desc->width <= 0) {
            return ORenderer::RESULT_INVALIDARG;
        } else if (desc->height <= 0) {
            return ORenderer::RESULT_INVALIDARG;
        } else if (desc->layers <= 0) {
            return ORenderer::RESULT_INVALIDARG;
        }

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

        if (desc->attachmentcount <= 0) {
            return ORenderer::RESULT_INVALIDARG;
        } else if (desc->colourrefcount <= 0 && desc->depthref == NULL) { // no colour references and no depth references!!!!!
            return ORenderer::RESULT_INVALIDARG;
        } else if (desc->attachments == NULL) {
            return ORenderer::RESULT_INVALIDARG;
        } else if (desc->colourrefs == NULL && desc->colourrefcount > 0) {
            return ORenderer::RESULT_INVALIDARG;
        }

        size_t attachmentid = 0;
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

        VkPipelineShaderStageCreateInfo stage = { };
        stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage.pNext = NULL;
        if (desc->stage.type != ORenderer::SHADER_COMPUTE) {
            return ORenderer::RESULT_INVALIDARG;
        }

        if (state->handle == RENDERER_INVALIDHANDLE) {
            this->resourcemutex.lock();
            state->handle = this->resourcehandle++;
            this->pipelinestates[state->handle].vkresource = (struct pipelinestate) { };
            this->resourcemutex.unlock();
        }


        stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stage.module = this->createshadermodule(desc->stage);
        stage.pName = "main";

        VkDescriptorSetLayoutBinding *layoutbindings = (VkDescriptorSetLayoutBinding *)malloc(sizeof(VkDescriptorSetLayoutBinding) * desc->resourcecount);
        ASSERT(layoutbindings != NULL, "Failed to allocate memory for Vulkan descriptor set layout binding.\n");
        for (size_t i = 0; i < desc->resourcecount; i++) {
            struct ORenderer::pipelinestateresourcedesc *resource = &desc->resources[i];
            layoutbindings[i].binding = resource->binding;
            layoutbindings[i].descriptorCount = 1;
            layoutbindings[i].stageFlags =
                (resource->stages & ORenderer::STAGE_VERTEX ? VK_SHADER_STAGE_VERTEX_BIT : 0) |
                (resource->stages & ORenderer::STAGE_TESSCONTROL ? VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT : 0) |
                (resource->stages & ORenderer::STAGE_TESSEVALUATION ? VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT : 0) |
                (resource->stages & ORenderer::STAGE_GEOMETRY ? VK_SHADER_STAGE_GEOMETRY_BIT : 0) |
                (resource->stages & ORenderer::STAGE_FRAGMENT ? VK_SHADER_STAGE_FRAGMENT_BIT : 0);
            layoutbindings[i].pImmutableSamplers = NULL;
            layoutbindings[i].descriptorType = descriptortypetable[resource->type];
        }

        struct pipelinestate *vkstate = &this->pipelinestates[state->handle].vkresource;

        this->resourcemutex.lock();
        VkDescriptorSetLayoutCreateInfo layoutcreate = { };
        layoutcreate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutcreate.pNext = NULL;
        layoutcreate.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
        layoutcreate.bindingCount = desc->resourcecount;
        layoutcreate.pBindings = layoutbindings;
        VkResult res = vkCreateDescriptorSetLayout(this->dev, &layoutcreate, NULL, &vkstate->descsetlayout);
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan descriptor set layout %d.\n", res);
        free(layoutbindings);

        VkDescriptorSetAllocateInfo allocinfo = { };
        allocinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocinfo.pNext = NULL;
        allocinfo.descriptorPool = this->descpool;
        allocinfo.descriptorSetCount = RENDERER_MAXLATENCY;
        VkDescriptorSetLayout alloclayouts[RENDERER_MAXLATENCY] = { };
        for (size_t i = 0; i < RENDERER_MAXLATENCY; i++) {
            alloclayouts[i] = vkstate->descsetlayout;
        }
        allocinfo.pSetLayouts = alloclayouts;
        res = vkAllocateDescriptorSets(this->dev, &allocinfo, vkstate->descsets);
        ASSERT(res == VK_SUCCESS, "Failed to allocate Vulkan descriptor sets %d.\n", res);


        VkPipelineLayoutCreateInfo pipelayoutcreate = { };
        pipelayoutcreate.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelayoutcreate.pNext = NULL;
        pipelayoutcreate.setLayoutCount = 1;
        pipelayoutcreate.pSetLayouts = &vkstate->descsetlayout;
        pipelayoutcreate.pushConstantRangeCount = 0;
        pipelayoutcreate.pPushConstantRanges = NULL;
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
        if (desc->stagecount <= 0) {
            return ORenderer::RESULT_INVALIDARG;
        }

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
                return ORenderer::RESULT_INVALIDARG;
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
        VkVertexInputAttributeDescription *attribdescs = (VkVertexInputAttributeDescription *)malloc(sizeof(VkVertexInputBindingDescription));
        ASSERT(attribdescs != NULL, "Failed to allocate memory for Vulkan input attributes.\n");
        size_t attribs = 0;

        for (size_t i = 0; i < desc->vtxinput->bindcount; i++) {
            struct ORenderer::vtxbinddesc bind = desc->vtxinput->binddescs[i];
            binddescs[i].binding = bind.layout.binding;
            binddescs[i].stride = bind.layout.stride;
            binddescs[i].inputRate = vulkan_inputratetable[bind.rate];
            attribs += bind.layout.attribcount;
            attribdescs = (VkVertexInputAttributeDescription *)realloc(attribdescs, sizeof(VkVertexInputAttributeDescription) * attribs);
            ASSERT(attribdescs, "Failed to reallocate memory for Vulkan vertex input attributes.\n");

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

        VkDescriptorSetLayoutBinding *layoutbindings = (VkDescriptorSetLayoutBinding *)malloc(sizeof(VkDescriptorSetLayoutBinding) * desc->resourcecount);
        ASSERT(layoutbindings != NULL, "Failed to allocate memory for Vulkan descriptor set layout bindings.\n");
        for (size_t i = 0; i < desc->resourcecount; i++) {
            struct ORenderer::pipelinestateresourcedesc *resource = &desc->resources[i];
            layoutbindings[i].binding = resource->binding;
            layoutbindings[i].descriptorCount = 1;
            layoutbindings[i].stageFlags =
                (resource->stages & ORenderer::STAGE_VERTEX ? VK_SHADER_STAGE_VERTEX_BIT : 0) |
                (resource->stages & ORenderer::STAGE_TESSCONTROL ? VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT : 0) |
                (resource->stages & ORenderer::STAGE_TESSEVALUATION ? VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT : 0) |
                (resource->stages & ORenderer::STAGE_GEOMETRY ? VK_SHADER_STAGE_GEOMETRY_BIT : 0) |
                (resource->stages & ORenderer::STAGE_FRAGMENT ? VK_SHADER_STAGE_FRAGMENT_BIT : 0);
            layoutbindings[i].pImmutableSamplers = NULL;
            layoutbindings[i].descriptorType = descriptortypetable[resource->type];
        }

        this->resourcemutex.lock();

        struct pipelinestate *vkstate = &this->pipelinestates[state->handle].vkresource;

        VkDescriptorSetLayoutCreateInfo layoutcreate = { };
        layoutcreate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutcreate.pNext = NULL;
        layoutcreate.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
        layoutcreate.bindingCount = desc->resourcecount;
        layoutcreate.pBindings = layoutbindings;
        VkResult res = vkCreateDescriptorSetLayout(this->dev, &layoutcreate, NULL, &vkstate->descsetlayout);
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan descriptor set layout %d.\n", res);
        free(layoutbindings);

        // VkDescriptorSetAllocateInfo allocinfo = { };
        // allocinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        // allocinfo.pNext = NULL;
        // allocinfo.descriptorPool = this->descpool;
        // allocinfo.descriptorSetCount = RENDERER_MAXLATENCY;
        // VkDescriptorSetLayout alloclayouts[RENDERER_MAXLATENCY] = { };
        // for (size_t i = 0; i < RENDERER_MAXLATENCY; i++) {
        //     alloclayouts[i] = vkstate->descsetlayout;
        // }
        // allocinfo.pSetLayouts = alloclayouts;
        // res = vkAllocateDescriptorSets(this->dev, &allocinfo, vkstate->descsets);
        // ASSERT(res == VK_SUCCESS, "Failed to allocate Vulkan descriptor sets %d.\n", res);


        VkPipelineLayoutCreateInfo pipelayoutcreate = { };
        pipelayoutcreate.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelayoutcreate.pNext = NULL;
        pipelayoutcreate.setLayoutCount = 1;
        pipelayoutcreate.pSetLayouts = &vkstate->descsetlayout;
        pipelayoutcreate.pushConstantRangeCount = 0;
        pipelayoutcreate.pPushConstantRanges = NULL;
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
        if (desc->layercount <= 0) {
            return ORenderer::RESULT_INVALIDARG;
        } else if (desc->mipcount <= 0) {
            return ORenderer::RESULT_INVALIDARG;
        } else if (desc->basemiplevel > desc->mipcount) {
            return ORenderer::RESULT_INVALIDARG;
        } else if (desc->texture.handle == RENDERER_INVALIDHANDLE) {
            return ORenderer::RESULT_INVALIDARG;
        }

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

    uint8_t VulkanContext::createsampler(struct ORenderer::samplerdesc *desc, struct ORenderer::sampler *sampler) {
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
        if (desc == NULL) {
            return ORenderer::RESULT_INVALIDARG;
        } else if (map == NULL) {
            return ORenderer::RESULT_INVALIDARG;
        } else if (desc->buffer.handle == RENDERER_INVALIDHANDLE) {
            return ORenderer::RESULT_INVALIDARG;
        }

        this->resourcemutex.lock();
        struct buffer *buffer = &this->buffers[desc->buffer.handle].vkresource;
        map->buffer = desc->buffer;
        if (buffer->flags & ORenderer::BUFFERFLAG_PERFRAME) {
            for (size_t i = 0; i < RENDERER_MAXLATENCY; i++) {
                VkResult res = vmaMapMemory(allocator, buffer->allocation[i], &map->mapped[i]);
                if (res != VK_SUCCESS) {
                    resourcemutex.unlock();
                    return ORenderer::RESULT_FAILED;
                }
            }
            resourcemutex.unlock();
            buffer->mapped = true;
        } else {
            VkResult res = vmaMapMemory(allocator, buffer->allocation[0], &map->mapped[0]);
            resourcemutex.unlock();
            if (res != VK_SUCCESS) {
                return ORenderer::RESULT_FAILED;
            }
            buffer->mapped = true;
        }
        return ORenderer::RESULT_SUCCESS;
    }

    uint8_t VulkanContext::unmapbuffer(struct ORenderer::buffermap map) {
        if (map.buffer.handle == RENDERER_INVALIDHANDLE) {
            return ORenderer::RESULT_INVALIDARG;
        }
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
        return this->frame;
    }

    uint8_t VulkanContext::copybuffer(struct ORenderer::buffercopydesc *desc) {
        if (desc == NULL) {
            return ORenderer::RESULT_INVALIDARG;
        } else if (desc->src.handle == RENDERER_INVALIDHANDLE) {
            return ORenderer::RESULT_INVALIDARG;
        } else if (desc->dst.handle == RENDERER_INVALIDHANDLE) {
            return ORenderer::RESULT_INVALIDARG;
        }

        VkCommandBuffer cmd = this->beginonetimecmd();

        VkBufferCopy copy = { };
        copy.srcOffset = desc->srcoffset;
        copy.dstOffset = desc->dstoffset;
        copy.size = desc->size;

        this->resourcemutex.lock();
        vkCmdCopyBuffer(cmd, this->buffers[desc->src.handle].vkresource.buffer[0], this->buffers[desc->dst.handle].vkresource.buffer[0], 1, &copy);
        this->resourcemutex.unlock();

        this->endonetimecmd(cmd);
        return ORenderer::RESULT_SUCCESS;
    }

    void VulkanContext::destroytexture(struct ORenderer::texture *texture) {
        if (texture->handle == RENDERER_INVALIDHANDLE) {
            return;
        }

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
        if (textureview->handle == RENDERER_INVALIDHANDLE) {
            return;
        }
        this->resourcemutex.lock();
        struct textureview *vktextureview = &this->textureviews[textureview->handle].vkresource;
        vkDestroyImageView(this->dev, vktextureview->imageview, NULL);
        this->textureviews.erase(textureview->handle);
        textureview->handle = RENDERER_INVALIDHANDLE;
        this->resourcemutex.unlock();
    }

    void VulkanContext::destroybuffer(struct ORenderer::buffer *buffer) {
        if (buffer->handle == RENDERER_INVALIDHANDLE) {
            return;
        }

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
        if (framebuffer->handle == RENDERER_INVALIDHANDLE) {
            return;
        }

        this->resourcemutex.lock();
        struct framebuffer *vkframebuffer = &this->framebuffers[framebuffer->handle].vkresource;
        vkDestroyFramebuffer(this->dev, vkframebuffer->framebuffer, NULL);
        this->framebuffers.erase(framebuffer->handle);
        framebuffer->handle = RENDERER_INVALIDHANDLE;
        this->resourcemutex.unlock();
    }

    void VulkanContext::destroyrenderpass(struct ORenderer::renderpass *pass) {
        if (pass->handle == RENDERER_INVALIDHANDLE) {
            return;
        }

        this->resourcemutex.lock();
        struct renderpass *vkrenderpass = &this->renderpasses[pass->handle].vkresource;
        vkDestroyRenderPass(this->dev, vkrenderpass->renderpass, NULL);
        this->renderpasses.erase(pass->handle);
        pass->handle = RENDERER_INVALIDHANDLE;
        this->resourcemutex.unlock();
    }

    void VulkanContext::destroypipelinestate(struct ORenderer::pipelinestate *state) {
        if (state->handle == RENDERER_INVALIDHANDLE) {
            return;
        }

        this->resourcemutex.lock();
        struct pipelinestate *vkpipelinestate = &this->pipelinestates[state->handle].vkresource;

        for (size_t i = 0; i < vkpipelinestate->shadercount; i++) {
            vkDestroyShaderModule(this->dev, vkpipelinestate->shaders[i], NULL);
        }
        vkDestroyPipelineLayout(this->dev, vkpipelinestate->pipelinelayout, NULL);
        vkFreeDescriptorSets(this->dev, this->descpool, RENDERER_MAXLATENCY, vkpipelinestate->descsets);
        vkDestroyDescriptorSetLayout(this->dev, vkpipelinestate->descsetlayout, NULL);
        vkDestroyPipeline(this->dev, vkpipelinestate->pipeline, NULL);
        this->pipelinestates.erase(state->handle);
        state->handle = RENDERER_INVALIDHANDLE;
        this->resourcemutex.unlock();
    }

    void VulkanContext::setdebugname(struct ORenderer::texture texture, const char *name) {
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

    uint8_t VulkanContext::requestbackbuffer(struct ORenderer::framebuffer *framebuffer) {
        if (framebuffer == NULL) {
            return ORenderer::RESULT_INVALIDARG;
        }
        *framebuffer = this->swapfbs[this->swapimage];
        return ORenderer::RESULT_SUCCESS;
    }

    uint8_t VulkanContext::requestbackbufferinfo(struct ORenderer::backbufferinfo *info) {
        if (info == NULL) {
            return ORenderer::RESULT_INVALIDARG;
        }

        info->format = ORenderer::FORMAT_BGRA8SRGB; // XXX: Return selected swapchain format (based on support)
        info->width = this->surfacecaps.currentExtent.width;
        info->height = this->surfacecaps.currentExtent.height;

        return ORenderer::RESULT_SUCCESS;
    }

    void VulkanContext::interpretstream(VkCommandBuffer cmd, ORenderer::Stream *stream) {
        stream->claim();
        // stream->mutex.lock();
        this->resourcemutex.lock();
        for (auto it = stream->cmd.begin(); it != stream->cmd.end(); it++) {
            switch (it->type) {
                case ORenderer::Stream::OP_BEGINRENDERPASS: {
                    size_t marker = stream->tempmem.getmarker();

                    VkRenderPass rpass = this->renderpasses[it->renderpass.rpass.handle].vkresource.renderpass;
                    VkFramebuffer fb = this->framebuffers[it->renderpass.fb.handle].vkresource.framebuffer;
                    struct ORenderer::rect area = it->renderpass.area;
                    struct ORenderer::clearcolourdesc clear = it->renderpass.clear;
                    VkRenderPassBeginInfo passbegin = { };
                    passbegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    passbegin.pNext = NULL;
                    passbegin.renderPass = rpass;
                    passbegin.framebuffer = fb;
                    passbegin.renderArea.offset = (VkOffset2D) { .x = area.x, .y = area.y };
                    passbegin.renderArea.extent = (VkExtent2D) { .width = area.width, .height = area.height };
                    passbegin.clearValueCount = clear.count;
                    VkClearValue *clearvalues = (VkClearValue *)stream->tempmem.alloc(sizeof(VkClearValue) * clear.count);
                    for (size_t i = 0; i < clear.count; i++) {
                        if (clear.clear[i].isdepth) {
                            clearvalues[i].depthStencil = { .depth = clear.clear[i].depth, .stencil = clear.clear[i].stencil };
                        } else {
                            clearvalues[i].color = { clear.clear[i].colour.r, clear.clear[i].colour.g, clear.clear[i].colour.b, clear.clear[i].colour.a };
                        }
                    }
                    passbegin.pClearValues = clearvalues;
                    vkCmdBeginRenderPass(cmd, &passbegin, VK_SUBPASS_CONTENTS_INLINE);
                    stream->tempmem.freeto(marker);
                    break;
                }
                case ORenderer::Stream::OP_ENDRENDERPASS: {
                    vkCmdEndRenderPass(cmd);
                    break;
                }
                case ORenderer::Stream::OP_SETVIEWPORT: {
                    struct ORenderer::viewport viewport = it->viewport;
                    VkViewport vkviewport;
                    vkviewport.x = viewport.x;
                    vkviewport.y = viewport.y;
                    vkviewport.width = viewport.width;
                    vkviewport.height = viewport.height;
                    vkviewport.minDepth = viewport.mindepth;
                    vkviewport.maxDepth = viewport.maxdepth;
                    vkCmdSetViewport(cmd, 0, 1, &vkviewport);
                    break;
                }
                case ORenderer::Stream::OP_SETSCISSOR: {
                    struct ORenderer::rect scissor = it->scissor;
                    VkRect2D vkscissor;
                    vkscissor.extent = (VkExtent2D) { .width = scissor.width, .height = scissor.height };
                    vkscissor.offset = (VkOffset2D) { .x = scissor.x, .y = scissor.y };
                    vkCmdSetScissor(cmd, 0, 1, &vkscissor);
                    break;
                }
                case ORenderer::Stream::OP_SETPIPELINESTATE: {
                    struct ORenderer::pipelinestate state = it->pipeline;
                    stream->pipelinestate = state;
                    struct pipelinestate *vkstate = &this->pipelinestates[state.handle].vkresource;
                    vkCmdBindPipeline(cmd, vkstate->type == ORenderer::GRAPHICSPIPELINE ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE, vkstate->pipeline);
                    stream->mappings.clear(); // reset mappings so we're only using our current pipeline's descriptor set mappings
                    break;
                }
                case ORenderer::Stream::OP_DRAW: {
                    vkCmdDraw(cmd, it->draw.vtxcount, it->draw.instancecount, it->draw.firstvtx, it->draw.firstinstance);
                    break;
                }
                case ORenderer::Stream::OP_DRAWINDEXED: {
                    vkCmdDrawIndexed(cmd, it->drawindexed.idxcount, it->drawindexed.instancecount, it->drawindexed.firstidx, it->drawindexed.vtxoffset, it->drawindexed.firstinstance);
                    break;
                }
                case ORenderer::Stream::OP_SETVTXBUFFERS: {
                    struct ORenderer::buffer *buffers = it->vtxbuffers.buffers;

                    size_t marker = stream->tempmem.getmarker();

                    VkBuffer *vkbuffers = (VkBuffer *)stream->tempmem.alloc(sizeof(VkBuffer) * it->vtxbuffers.bindcount);
                    for (size_t i = 0; i < it->vtxbuffers.bindcount; i++) {
                        struct buffer *vkbuffer = &this->buffers[buffers[i].handle].vkresource;
                        vkbuffers[i] = vkbuffer->buffer[0];
                    }
                    vkCmdBindVertexBuffers(cmd, 0, 1, vkbuffers, it->vtxbuffers.offsets);
                    stream->tempmem.freeto(marker);
                    break;
                }
                case ORenderer::Stream::OP_SETIDXBUFFER: {
                    struct buffer *buffer = &this->buffers[it->idxbuffer.buffer.handle].vkresource;
                    vkCmdBindIndexBuffer(cmd, buffer->buffer[0], it->idxbuffer.offset, it->idxbuffer.index32 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
                    break;
                }
                case ORenderer::Stream::OP_BINDRESOURCE: {
                    switch (it->resource.type) {
                        case ORenderer::RESOURCE_UNIFORM:
                        case ORenderer::RESOURCE_STORAGE:
                            stream->mappings.push_back((struct ORenderer::pipelinestateresourcemap) { .binding = it->resource.binding, .type = it->resource.type, .bufferbind = it->resource.bufferbind });
                        case ORenderer::RESOURCE_SAMPLER:
                        case ORenderer::RESOURCE_STORAGEIMAGE:
                            stream->mappings.push_back((struct ORenderer::pipelinestateresourcemap) { .binding = it->resource.binding, .type = it->resource.type, .sampledbind = it->resource.sampledbind });
                            break;
                    }
                    break;
                }
                case ORenderer::Stream::OP_TRANSITIONLAYOUT: {
                    struct texture *vktexture = &this->textures[it->layout.texture.handle].vkresource;
                    OVulkan::transitionlayout(cmd, vktexture, it->layout.format, it->layout.state);
                    break;
                }
                case ORenderer::Stream::OP_COMMITRESOURCES: {
                    ASSERT(stream->pipelinestate.handle != RENDERER_INVALIDHANDLE, "Attempted to commit resources before deciding pipeline state.\n");
                    size_t marker = stream->tempmem.getmarker();

                    VkWriteDescriptorSet *wds = (VkWriteDescriptorSet *)stream->tempmem.alloc(sizeof(VkWriteDescriptorSet) * stream->mappings.size());
                    // XXX: Always be careful about scope! Higher optimisation levels seem to treat anything even one level out of scope as invalid (so we should make sure not to let that happen!)
                    VkDescriptorBufferInfo *buffinfos = (VkDescriptorBufferInfo *)stream->tempmem.alloc(sizeof(VkDescriptorBufferInfo) * stream->mappings.size());
                    VkDescriptorImageInfo *imginfos = (VkDescriptorImageInfo *)stream->tempmem.alloc(sizeof(VkDescriptorImageInfo) * stream->mappings.size()); // we're allocating with worst case in mind
                    for (size_t i = 0; i < stream->mappings.size(); i++) {
                        wds[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        wds[i].pNext = NULL;
                        wds[i].dstBinding = stream->mappings[i].binding;
                        wds[i].dstSet = this->pipelinestates[stream->pipelinestate.handle].vkresource.descsets[this->frame];
                        wds[i].dstArrayElement = 0;
                        wds[i].descriptorCount = 1;
                        if (stream->mappings[i].type == ORenderer::RESOURCE_UNIFORM || stream->mappings[i].type == ORenderer::RESOURCE_STORAGE) {
                            struct ORenderer::bufferbind bind = stream->mappings[i].bufferbind;
                            struct buffer *buffer = &this->buffers[bind.buffer.handle].vkresource;
                            wds[i].descriptorType = descriptortypetable[stream->mappings[i].type];
                            buffinfos[i].buffer = buffer->flags & ORenderer::BUFFERFLAG_PERFRAME ? buffer->buffer[this->frame] : buffer->buffer[0];
                            buffinfos[i].offset = bind.offset;
                            buffinfos[i].range = bind.range == SIZE_MAX ? VK_WHOLE_SIZE : bind.range;
                            wds[i].pBufferInfo = &buffinfos[i];
                        } else if (stream->mappings[i].type == ORenderer::RESOURCE_SAMPLER || stream->mappings[i].type == ORenderer::RESOURCE_STORAGEIMAGE) {
                            struct ORenderer::sampledbind bind = stream->mappings[i].sampledbind;
                            struct sampler *sampler = &this->samplers[bind.sampler.handle].vkresource;
                            struct textureview *view = &this->textureviews[bind.view.handle].vkresource;
                            wds[i].descriptorType = descriptortypetable[stream->mappings[i].type];
                            imginfos[i].sampler = sampler->sampler;
                            imginfos[i].imageView = view->imageview;
                            imginfos[i].imageLayout = layouttable[bind.layout];
                            wds[i].pImageInfo = &imginfos[i];
                        }
                    }

                    struct pipelinestate *pipelinestate = &this->pipelinestates[stream->pipelinestate.handle].vkresource;
                    vkCmdPushDescriptorSetKHR(cmd, pipelinestate->type == ORenderer::GRAPHICSPIPELINE ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE, pipelinestate->pipelinelayout, 0, stream->mappings.size(), wds);


                    stream->mappings.clear();
                    stream->tempmem.freeto(marker);
                    break;
                }
                case ORenderer::Stream::OP_COPYBUFFERIMAGE: {
                    struct ORenderer::bufferimagecopy region = it->copybufferimage.region;
                    struct buffer *buffer = &this->buffers[it->copybufferimage.buffer.handle].vkresource;
                    struct texture *texture = &this->textures[it->copybufferimage.texture.handle].vkresource;
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
                    vkCmdCopyBufferToImage(cmd, buffer->flags & ORenderer::BUFFERFLAG_PERFRAME ? buffer->buffer[this->frame] : buffer->buffer[0], texture->image, layouttable[texture->state], 1, &vkregion);
                    break;
                }
                case ORenderer::Stream::OP_STAGEDMEMCOPY: {


                    // memcpy(it->stagedmemcopy.dst, it->stagedmemcopy.src, it->stagedmemcopy.size);
                    break;
                }
            }
        }

        this->resourcemutex.unlock();
        // stream->mutex.unlock();
        stream->release();
    }

    uint8_t VulkanContext::submitstream(ORenderer::Stream *stream) {
        if (stream == NULL) {
            return ORenderer::RESULT_INVALIDARG;
        }
        VkCommandBuffer cmd = this->beginonetimecmd();

        this->interpretstream(cmd, stream);

        this->endonetimecmd(cmd);
        return ORenderer::RESULT_SUCCESS;
    }

    void VulkanContext::recordcmd(VkCommandBuffer cmd, uint32_t image, ORenderer::Stream *stream) {
        VkCommandBufferBeginInfo cmdbegin = { };
        cmdbegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdbegin.pNext = NULL;
        cmdbegin.flags = 0;
        cmdbegin.pInheritanceInfo = NULL;

        VkResult res = vkBeginCommandBuffer(cmd, &cmdbegin);
        ASSERT(res == VK_SUCCESS, "Failed to begin Vulkan command buffer %d.\n", res);

        this->scratchbuffers[this->frame].reset();

        // do stuff
        this->interpretstream(cmd, stream);

        res = vkEndCommandBuffer(cmd);
        ASSERT(res == VK_SUCCESS, "Failed to end Vulkan command buffer recording %d.\n", res);
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
        uint32_t indices[2] = { this->graphicscomputefamily, this->presentfamily };
        if (this->graphicscomputefamily != this->presentfamily) {
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
        VkResult res;
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
            sprintf(buf, "Backbuffer Texture %lu", i);
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
            sprintf(buf, "Backbuffer Texture View %lu", i);
            this->setdebugname(this->swaptextureviews[i], buf);
        }
    }

    uint8_t VulkanContext::createbackbuffer(struct ORenderer::renderpass pass) {
        if (pass.handle == RENDERER_INVALIDHANDLE) {
            return ORenderer::RESULT_INVALIDARG;
        }

        this->swapfbpass = pass;

        for (size_t i = 0; i < this->swaptexturecount; i++) {
            struct ORenderer::framebufferdesc fbdesc = { };
            fbdesc.width = this->surfacecaps.currentExtent.width;
            fbdesc.height = this->surfacecaps.currentExtent.height;
            fbdesc.layers = 1;
            fbdesc.attachmentcount = 1;
            fbdesc.pass = pass; // XXX: How do we setup the renderpass for the backbuffer?
            fbdesc.attachments = &this->swaptextureviews[i];
            ASSERT(this->createframebuffer(&fbdesc, &this->swapfbs[i]) == ORenderer::RESULT_SUCCESS, "Failed to create Vulkan swapchain framebuffer.\n");
            char buf[64];
            sprintf(buf, "Backbuffer %lu", i);
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

        for (size_t i = 0; i < RENDERER_MAXLATENCY; i++) {
            this->scratchbuffers[i].destroy();
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
            vkDestroyDescriptorSetLayout(this->dev, vkpipelinestate.descsetlayout, NULL);
            vkDestroyPipeline(this->dev, vkpipelinestate.pipeline, NULL);
        }

        vkDestroyDescriptorPool(this->dev, this->descpool, NULL);
        vkDestroyCommandPool(this->dev, this->cmdpool, NULL);
        for (size_t i = 0; i < RENDERER_MAXLATENCY; i++) {
            vkDestroySemaphore(this->dev, this->imagepresent[i], NULL);
            vkDestroySemaphore(this->dev, this->renderdone[i], NULL);
            vkDestroyFence(this->dev, this->framesinflight[i], NULL);
        }

        vmaDestroyAllocator(this->allocator);

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
        info.engineVersion = VK_MAKE_VERSION(OMICRON_VERSION, 0, 0);
        info.apiVersion = apiversion;

        const char *extensions[sizeof(OVulkan::extensions) / sizeof(OVulkan::extensions[0])]; // purely string representation of vulkan device extensions
        for (size_t i = 0; i < sizeof(OVulkan::extensions) / sizeof(OVulkan::extensions[0]); i++) {
           extensions[i] = OVulkan::extensions[i].name;
        }

        checkextensions(VK_NULL_HANDLE, OVulkan::extensions, sizeof(OVulkan::extensions) / sizeof(OVulkan::extensions[0]));

        for (size_t i = 0; i < sizeof(OVulkan::extensions) / sizeof(OVulkan::extensions[0]); i++) {
            ASSERT(OVulkan::extensions[i].optional || OVulkan::extensions[i].supported, "Required Vulkan extension %s is not present when needed for instance.\n", OVulkan::extensions[i].name);
        }

        VkInstanceCreateInfo instancecreate = { };
        instancecreate.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instancecreate.pNext = NULL;
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

        vkGetPhysicalDeviceFeatures(this->phy, &this->phyfeatures);
        // memset(&this->enabledphyfeatures, 0, sizeof(VkPhysicalDeviceProperties));
        this->enabledphyfeatures = this->phyfeatures;
        // this->phyfeatures.samplerAnisotropy = VK_TRUE;

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

        for (size_t i = 0; i < queuefamilycount; i++) {
            VkQueueFamilyProperties *props = &queueprops[i];
            if ((props->queueFlags & VK_QUEUE_GRAPHICS_BIT) && (props->queueFlags & VK_QUEUE_COMPUTE_BIT)) { // find a queue that offers what we want
                // this->globalqueuefamily = i;
                this->graphicscomputefamily = i;
            }

            VkBool32 presentsupported = VK_FALSE;
            res = vkGetPhysicalDeviceSurfaceSupportKHR(this->phy, i, this->surface, &presentsupported);
            ASSERT(res == VK_SUCCESS, "Failed to gather support information on queue family %lu\n", i);

            if (presentsupported) {
                this->presentfamily = i;
            }

        }
        free(queueprops);

        float queueprio = 1.0f;
        VkDeviceQueueCreateInfo queuecreate[2];
        queuecreate[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queuecreate[0].pNext = NULL;
        queuecreate[0].queueCount = 1;
        queuecreate[0].queueFamilyIndex = this->graphicscomputefamily;
        queuecreate[0].pQueuePriorities = &queueprio;
        queuecreate[0].flags = 0;

        queuecreate[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queuecreate[1].pNext = NULL;
        queuecreate[1].queueCount = 1;
        queuecreate[1].queueFamilyIndex = this->presentfamily;
        queuecreate[1].pQueuePriorities = &queueprio;
        queuecreate[1].flags = 0;

        VkDeviceCreateInfo devicecreate = { };
        devicecreate.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        devicecreate.pQueueCreateInfos = queuecreate;
        devicecreate.queueCreateInfoCount = 2;
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
        alloccreate.instance = this->instance;
        alloccreate.pVulkanFunctions = &this->vmafunctions;
        vmaCreateAllocator(&alloccreate, &this->allocator);

        vkGetDeviceQueue(this->dev, this->graphicscomputefamily, 0, &this->graphicsqueue);
        vkGetDeviceQueue(this->dev, this->graphicscomputefamily, 0, &this->computequeue);
        vkGetDeviceQueue(this->dev, this->presentfamily, 0, &this->presentqueue);

        uint32_t formatcount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(this->phy, this->surface, &formatcount, NULL);
        VkSurfaceFormatKHR *formats = (VkSurfaceFormatKHR *)malloc(sizeof(VkSurfaceFormatKHR) * formatcount);
        ASSERT(formats != NULL, "Failed to allocate memory for Vulkan surface supported formats.\n");
        vkGetPhysicalDeviceSurfaceFormatsKHR(this->phy, this->surface, &formatcount, formats);
        free(formats); // XXX: Actually do something with these

        this->createswapchain();
        this->createswapchainviews();

        VkCommandPoolCreateInfo poolcreate = { };
        poolcreate.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolcreate.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolcreate.queueFamilyIndex = this->graphicscomputefamily;

        res = vkCreateCommandPool(this->dev, &poolcreate, NULL, &this->cmdpool);
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan command pool %d.\n", res);

        // create all command buffers
        VkCommandBufferAllocateInfo cmdalloc = { };
        cmdalloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdalloc.commandPool = this->cmdpool;
        cmdalloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdalloc.commandBufferCount = RENDERER_MAXLATENCY;
        res = vkAllocateCommandBuffers(this->dev, &cmdalloc, this->cmd);
        ASSERT(res == VK_SUCCESS, "Failed to allocate Vulkan command buffer %d.\n", res);

        VkSemaphoreCreateInfo semcreate = { };
        semcreate.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fencecreate = { };
        fencecreate.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fencecreate.flags = VK_FENCE_CREATE_SIGNALED_BIT; // initialise as signalled

        for (size_t i = 0; i < RENDERER_MAXLATENCY; i++) {
            ASSERT(vkCreateSemaphore(this->dev, &semcreate, NULL, &this->imagepresent[i]) == VK_SUCCESS && vkCreateSemaphore(this->dev, &semcreate, NULL, &this->renderdone[i]) == VK_SUCCESS, "Failed to create Vulkan semaphores!\n");
            ASSERT(vkCreateFence(this->dev, &fencecreate, NULL, &this->framesinflight[i]) == VK_SUCCESS, "Failed to create Vulkan fence!\n");
        }

        VkDescriptorPoolSize poolsizes[] = {
            (VkDescriptorPoolSize) {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = VULKAN_MAXDESCRIPTORS * 2,
            },
            (VkDescriptorPoolSize) {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
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
        res = vkCreateDescriptorPool(this->dev, &descpoolcreate, NULL, &this->descpool);
        ASSERT(res == VK_SUCCESS, "Failed to create Vulkan descriptor pool %d.\n", res);

        for (size_t i = 0; i < RENDERER_MAXLATENCY; i++) {
            this->scratchbuffers[i].create(this, 128, RENDERER_MAXDRAWCALLS);
        }
    }

    ORenderer::Stream stream[RENDERER_MAXLATENCY];

    void VulkanContext::execute(GraphicsPipeline *pipeline) {
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


        vkResetCommandBuffer(this->cmd[this->frame], 0);
        stream[this->frame].flushcmd();

        pipeline->execute(&stream[this->frame]);

        this->recordcmd(this->cmd[this->frame], image, &stream[this->frame]);

        VkSubmitInfo submit = { };
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitsems[] = { this->imagepresent[this->frame] };
        VkPipelineStageFlags waitstages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // wait for output stage by waiting on imagepresent semaphore, execute the stage when triggered
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = waitsems;
        submit.pWaitDstStageMask = waitstages;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &this->cmd[this->frame];

        VkSemaphore signalsems[] = { this->renderdone[this->frame] };
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = signalsems; // signal render done when we're done

        res = vkQueueSubmit(this->graphicsqueue, 1, &submit, this->framesinflight[this->frame]); // do all this and trigger the frameinflight fence to we know we can go ahead and render the next frame when we're done with the command buffers
        ASSERT(res == VK_SUCCESS, "Failed to submit Vulkan queue %d.\n", res);

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
        VkCommandBuffer cmd = this->beginonetimecmd();
        // vulkan_resourcemutex.lock();
        // vulkan_transitionlayout(cmd, &vulkan_textures[this->swaptextures[image].handle].vkresource, RENDERER_FORMATBGRA8, RENDERER_LAYOUTBACKBUFFER);
        // vulkan_resourcemutex.unlock();
        this->endonetimecmd(cmd);
        res = vkQueuePresentKHR(this->presentqueue, &presentinfo); // blit queue to swapchains
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
    }

    ORenderer::RendererContext *createcontext(void) {
        return NULL;
    }

}
