#ifndef _ENGINE__RENDERER__BACKEND__VULKAN_HPP
#define _ENGINE__RENDERER__BACKEND__VULKAN_HPP

#include <dlfcn.h>
#include <GLFW/glfw3.h>
#include <engine/renderer/pipeline.hpp>
#include <engine/renderer/renderer.hpp>
#include <unordered_map>

#ifdef __linux__

#define VK_USE_PLATFORM_XLIB_KHR
#define VK_USE_PLATFORM_XCB_KHR
#define KHR_SURFACE_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME
#define VK_IMPORT_INSTANCE_PLATFORM VK_IMPORT_INSTANCE_LINUX

#endif

#define VK_NO_STDINT_H
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <engine/ext/vk_mem_alloc.hpp>

#define VK_IMPORT \
    VK_IMPORT_FUNC(false, vkCreateInstance); \
    VK_IMPORT_FUNC(false, vkGetInstanceProcAddr); \
    VK_IMPORT_FUNC(false, vkGetDeviceProcAddr); \
    VK_IMPORT_FUNC(false, vkEnumerateInstanceExtensionProperties); \
    VK_IMPORT_FUNC(false, vkEnumerateInstanceLayerProperties); \
    VK_IMPORT_FUNC(true, vkEnumerateInstanceVersion);
#define VK_IMPORT_INSTANCE_LINUX \
    VK_IMPORT_INSTANCE_FUNC(true, vkCreateXlibSurfaceKHR); \
    VK_IMPORT_INSTANCE_FUNC(true, vkGetPhysicalDeviceXlibPresentationSupportKHR); \
    VK_IMPORT_INSTANCE_FUNC(true, vkCreateXcbSurfaceKHR); \
    VK_IMPORT_INSTANCE_FUNC(true, vkGetPhysicalDeviceXcbPresentationSupportKHR);

#define VK_IMPORT_INSTANCE \
    VK_IMPORT_INSTANCE_FUNC(false, vkDestroyInstance); \
    VK_IMPORT_INSTANCE_FUNC(false, vkEnumeratePhysicalDevices); \
    VK_IMPORT_INSTANCE_FUNC(false, vkEnumerateDeviceExtensionProperties); \
    VK_IMPORT_INSTANCE_FUNC(false, vkEnumerateDeviceLayerProperties); \
    VK_IMPORT_INSTANCE_FUNC(false, vkGetPhysicalDeviceProperties); \
    VK_IMPORT_INSTANCE_FUNC(false, vkGetPhysicalDeviceFormatProperties); \
    VK_IMPORT_INSTANCE_FUNC(false, vkGetPhysicalDeviceFeatures); \
    VK_IMPORT_INSTANCE_FUNC(false, vkGetPhysicalDeviceImageFormatProperties); \
    VK_IMPORT_INSTANCE_FUNC(false, vkGetPhysicalDeviceMemoryProperties); \
    VK_IMPORT_INSTANCE_FUNC(false, vkGetPhysicalDeviceQueueFamilyProperties); \
    VK_IMPORT_INSTANCE_FUNC(false, vkCreateDevice); \
    VK_IMPORT_INSTANCE_FUNC(false, vkDestroyDevice); \
    VK_IMPORT_INSTANCE_FUNC(true, vkGetPhysicalDeviceSurfaceCapabilitiesKHR); \
    VK_IMPORT_INSTANCE_FUNC(true, vkGetPhysicalDeviceSurfaceFormatsKHR); \
    VK_IMPORT_INSTANCE_FUNC(true, vkGetPhysicalDeviceSurfacePresentModesKHR); \
    VK_IMPORT_INSTANCE_FUNC(true, vkGetPhysicalDeviceSurfaceSupportKHR); \
    VK_IMPORT_INSTANCE_FUNC(true, vkDestroySurfaceKHR); \
    VK_IMPORT_INSTANCE_FUNC(false, vkGetPhysicalDeviceFeatures2); \
    VK_IMPORT_INSTANCE_FUNC(true, vkGetPhysicalDeviceMemoryProperties2KHR); \
    VK_IMPORT_INSTANCE_FUNC(true, vkCreateDebugReportCallbackEXT); \
    VK_IMPORT_INSTANCE_FUNC(true, vkDestroyDebugReportCallbackEXT); \
    VK_IMPORT_INSTANCE_FUNC(true, vkCmdSetFragmentShadingRateKHR); \
    VK_IMPORT_INSTANCE_FUNC(true, vkGetPhysicalDeviceFragmentShadingRatesKHR); \
    VK_IMPORT_INSTANCE_PLATFORM

#define VK_IMPORT_DEVICE \
    VK_IMPORT_DEVICE_FUNC(false, vkGetDeviceQueue); \
    VK_IMPORT_DEVICE_FUNC(false, vkCreateFence); \
    VK_IMPORT_DEVICE_FUNC(false, vkDestroyFence); \
    VK_IMPORT_DEVICE_FUNC(false, vkCreateSemaphore); \
    VK_IMPORT_DEVICE_FUNC(false, vkDestroySemaphore); \
    VK_IMPORT_DEVICE_FUNC(false, vkResetFences); \
    VK_IMPORT_DEVICE_FUNC(false, vkCreateCommandPool); \
    VK_IMPORT_DEVICE_FUNC(false, vkDestroyCommandPool); \
    VK_IMPORT_DEVICE_FUNC(false, vkResetCommandPool); \
    VK_IMPORT_DEVICE_FUNC(false, vkAllocateCommandBuffers); \
    VK_IMPORT_DEVICE_FUNC(false, vkFreeCommandBuffers); \
    VK_IMPORT_DEVICE_FUNC(false, vkGetBufferMemoryRequirements); \
    VK_IMPORT_DEVICE_FUNC(false, vkGetImageMemoryRequirements); \
    VK_IMPORT_DEVICE_FUNC(false, vkGetImageSubresourceLayout); \
    VK_IMPORT_DEVICE_FUNC(false, vkAllocateMemory); \
    VK_IMPORT_DEVICE_FUNC(false, vkFreeMemory); \
    VK_IMPORT_DEVICE_FUNC(false, vkCreateImage); \
    VK_IMPORT_DEVICE_FUNC(false, vkDestroyImage); \
    VK_IMPORT_DEVICE_FUNC(false, vkCreateImageView); \
    VK_IMPORT_DEVICE_FUNC(false, vkDestroyImageView); \
    VK_IMPORT_DEVICE_FUNC(false, vkCreateBuffer); \
    VK_IMPORT_DEVICE_FUNC(false, vkDestroyBuffer); \
    VK_IMPORT_DEVICE_FUNC(false, vkCreateFramebuffer); \
    VK_IMPORT_DEVICE_FUNC(false, vkDestroyFramebuffer); \
    VK_IMPORT_DEVICE_FUNC(false, vkCreateRenderPass); \
    VK_IMPORT_DEVICE_FUNC(false, vkDestroyRenderPass); \
    VK_IMPORT_DEVICE_FUNC(false, vkCreateShaderModule); \
    VK_IMPORT_DEVICE_FUNC(false, vkDestroyShaderModule); \
    VK_IMPORT_DEVICE_FUNC(false, vkCreatePipelineCache); \
    VK_IMPORT_DEVICE_FUNC(false, vkDestroyPipelineCache); \
    VK_IMPORT_DEVICE_FUNC(false, vkGetPipelineCacheData); \
    VK_IMPORT_DEVICE_FUNC(false, vkMergePipelineCaches); \
    VK_IMPORT_DEVICE_FUNC(false, vkCreateGraphicsPipelines); \
    VK_IMPORT_DEVICE_FUNC(false, vkCreateComputePipelines); \
    VK_IMPORT_DEVICE_FUNC(false, vkDestroyPipeline); \
    VK_IMPORT_DEVICE_FUNC(false, vkCreatePipelineLayout); \
    VK_IMPORT_DEVICE_FUNC(false, vkDestroyPipelineLayout); \
    VK_IMPORT_DEVICE_FUNC(false, vkCreateSampler); \
    VK_IMPORT_DEVICE_FUNC(false, vkDestroySampler); \
    VK_IMPORT_DEVICE_FUNC(false, vkCreateDescriptorSetLayout); \
    VK_IMPORT_DEVICE_FUNC(false, vkDestroyDescriptorSetLayout); \
    VK_IMPORT_DEVICE_FUNC(false, vkCreateDescriptorPool); \
    VK_IMPORT_DEVICE_FUNC(false, vkDestroyDescriptorPool); \
    VK_IMPORT_DEVICE_FUNC(false, vkResetDescriptorPool); \
    VK_IMPORT_DEVICE_FUNC(false, vkAllocateDescriptorSets); \
    VK_IMPORT_DEVICE_FUNC(false, vkFreeDescriptorSets); \
    VK_IMPORT_DEVICE_FUNC(false, vkUpdateDescriptorSets); \
    VK_IMPORT_DEVICE_FUNC(false, vkCreateQueryPool); \
    VK_IMPORT_DEVICE_FUNC(false, vkDestroyQueryPool); \
    VK_IMPORT_DEVICE_FUNC(false, vkQueueSubmit); \
    VK_IMPORT_DEVICE_FUNC(false, vkQueueWaitIdle); \
    VK_IMPORT_DEVICE_FUNC(false, vkDeviceWaitIdle); \
    VK_IMPORT_DEVICE_FUNC(false, vkWaitForFences); \
    VK_IMPORT_DEVICE_FUNC(false, vkBeginCommandBuffer); \
    VK_IMPORT_DEVICE_FUNC(false, vkEndCommandBuffer); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdPipelineBarrier); \
    VK_IMPORT_DEVICE_FUNC(false, vkResetCommandBuffer); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdBeginRenderPass); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdEndRenderPass); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdSetViewport); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdDraw); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdDrawIndexed); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdDrawIndirect); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdDrawIndexedIndirect); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdDispatch); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdDispatchIndirect); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdBindPipeline); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdSetStencilReference); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdSetBlendConstants); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdSetScissor); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdBindDescriptorSets); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdBindIndexBuffer); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdBindVertexBuffers); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdClearColorImage); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdClearDepthStencilImage); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdClearAttachments); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdResolveImage); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdCopyBuffer); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdCopyBufferToImage); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdCopyImage); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdCopyImageToBuffer); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdBlitImage); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdResetQueryPool); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdWriteTimestamp); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdBeginQuery); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdEndQuery); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdCopyQueryPoolResults); \
    VK_IMPORT_DEVICE_FUNC(false, vkMapMemory); \
    VK_IMPORT_DEVICE_FUNC(false, vkUnmapMemory); \
    VK_IMPORT_DEVICE_FUNC(false, vkFlushMappedMemoryRanges); \
    VK_IMPORT_DEVICE_FUNC(false, vkInvalidateMappedMemoryRanges); \
    VK_IMPORT_DEVICE_FUNC(false, vkBindBufferMemory); \
    VK_IMPORT_DEVICE_FUNC(false, vkBindImageMemory); \
    VK_IMPORT_DEVICE_FUNC(false, vkCmdPushDescriptorSetKHR); \
    VK_IMPORT_DEVICE_FUNC(true,  vkCreateSwapchainKHR); \
    VK_IMPORT_DEVICE_FUNC(true,  vkDestroySwapchainKHR); \
    VK_IMPORT_DEVICE_FUNC(true,  vkGetSwapchainImagesKHR); \
    VK_IMPORT_DEVICE_FUNC(true,  vkAcquireNextImageKHR); \
    VK_IMPORT_DEVICE_FUNC(true,  vkQueuePresentKHR); \
    VK_IMPORT_DEVICE_FUNC(true,  vkSetDebugUtilsObjectNameEXT); \
    VK_IMPORT_DEVICE_FUNC(true,  vkCmdBeginDebugUtilsLabelEXT); \
    VK_IMPORT_DEVICE_FUNC(true,  vkCmdEndDebugUtilsLabelEXT); \
    VK_IMPORT_DEVICE_FUNC(true,  vkCmdInsertDebugUtilsLabelEXT); \
    VK_IMPORT_DEVICE_FUNC(true,  vkCmdDrawIndirectCountKHR); \
    VK_IMPORT_DEVICE_FUNC(true,  vkCmdDrawIndexedIndirectCountKHR);


#define TRACY_VK_USE_SYMBOL_TABLE
#include <tracy/TracyVulkan.hpp>

namespace OVulkan {

#define VULKAN_MAXBACKBUFFERS 10
#define VULKAN_MAXDESCRIPTORS (1024 * RENDERER_MAXLATENCY)

    struct extension {
        const char *name;
        bool supported;
        bool instance;
        bool optional;
    };

    struct shader {
        VkShaderModule module;
        ORenderer::Shader shader;
    };

    enum {
        VULKAN_TEXTURESWAPCHAIN = (1 << 0)
    };

    struct texture {
        VkImage image;
        VkDeviceMemory memory;
        size_t offset;
        VkImageLayout state;
        VmaAllocation allocation;
        size_t flags = 0;
    };

    struct textureview {
        VkImageView imageview;
    };

    struct buffer {
        VkBuffer buffer[RENDERER_MAXLATENCY];
        VkDeviceMemory memory[RENDERER_MAXLATENCY];
        size_t offset[RENDERER_MAXLATENCY];
        VmaAllocation allocation[RENDERER_MAXLATENCY];
        bool mapped = false;
        size_t flags = 0;
    };

    struct framebuffer {
        VkFramebuffer framebuffer;
    };

    struct renderpass {
        VkRenderPass renderpass;
    };

    struct pipelinestate {
        size_t type;
        VkPipeline pipeline;
        VkPipelineLayout pipelinelayout;
        VkDescriptorSetLayout descsetlayout;
        VkDescriptorSet descsets[RENDERER_MAXLATENCY];
        VkShaderModule shaders[ORenderer::SHADER_COUNT];
        size_t shadercount;
    };

    struct sampler {
        VkSampler sampler;
    };

    template <typename T>
    class VulkanResource {
        public:
            T vkresource;
    };

    class VulkanContext;
    class VulkanStream : public ORenderer::Stream {
        private:
            OJob::Mutex mutex;
        public:
            VkCommandBuffer cmd = VK_NULL_HANDLE;
            VulkanContext *context;
            uint8_t primary = 0;

            void flushcmd(void);

            void setviewport(struct ORenderer::viewport viewport);
            void setscissor(struct ORenderer::rect scissor);
            void beginrenderpass(struct ORenderer::renderpass renderpass, struct ORenderer::framebuffer framebuffer, struct ORenderer::rect area, struct ORenderer::clearcolourdesc clear);

            void endrenderpass(void);
            void setpipelinestate(struct ORenderer::pipelinestate pipeline);

            // Set the current index buffer for use before a draw call.
            void setidxbuffer(struct ORenderer::buffer buffer, size_t offset, bool index32);

            // Set the current vertex buffers for use before a draw call.
            void setvtxbuffers(struct ORenderer::buffer *buffers, size_t *offsets, size_t firstbind, size_t bindcount);

            void setvtxbuffer(struct ORenderer::buffer buffer, size_t offset);

            // Draw unindexed vertices.
            void draw(size_t vtxcount, size_t instancecount, size_t firstvtx, size_t firstinstance);

            // Draw indexed vertices.
            void drawindexed(size_t idxcount, size_t instancecount, size_t firstidx, size_t vtxoffset, size_t firstinstance);

            // Commit currently set pipeline resources.
            void commitresources(void);

            // Bind a pipeline resource to a binding.
            void bindresource(size_t binding, struct ORenderer::bufferbind bind, size_t type);

            void bindresource(size_t binding, struct ORenderer::sampledbind bind, size_t type);

            // Transition a texture between layouts.
            void transitionlayout(struct ORenderer::texture texture, size_t format, size_t state);

            void copybufferimage(struct ORenderer::bufferimagecopy region, struct ORenderer::buffer buffer, struct ORenderer::texture texture);

            // Submit another stream's command list to the current command list (the result will be as if the commands were submitted to this current stream)
            void submitstream(ORenderer::Stream *stream);


            void begin(void);
            void end(void);
    };

    struct resource {
        public:
            union {
                struct texture texture;
                struct textureview textureview;
                struct buffer buffer;
                struct framebuffer framebuffer;
                struct renderpass renderpass;
                struct pipelinestate pipelinestate;
            };
    };

    class VulkanContext;

    class VulkanContext : public ORenderer::RendererContext {
        public:
            TracyVkCtx tracyctx;

            VmaVulkanFunctions vmafunctions = { };
            VmaAllocator allocator = { };
            OJob::Mutex resourcemutex;
            // XXX: Dynamic allocations *are* expensive, if we ever create new render resources it'll be pretty expensive; however, thankfully, recreating resources (something we'd do more often) would be less expensive (memory alloc + write as opposed to just a lookup and write)
            // Preallocate anything maybe?
            std::unordered_map<size_t, VulkanResource<struct texture>> textures;
            std::unordered_map<size_t, VulkanResource<struct textureview>> textureviews;
            std::unordered_map<size_t, VulkanResource<struct buffer>> buffers;
            std::unordered_map<size_t, VulkanResource<struct framebuffer>> framebuffers;
            std::unordered_map<size_t, VulkanResource<struct renderpass>> renderpasses;
            std::unordered_map<size_t, VulkanResource<struct pipelinestate>> pipelinestates;
            std::unordered_map<size_t, VulkanResource<struct sampler>> samplers;
            size_t resourcehandle = 0;

            ORenderer::ScratchBuffer scratchbuffers[RENDERER_MAXLATENCY];

            VkPhysicalDevice phy;
            VkPhysicalDeviceProperties phyprops;
            VkPhysicalDeviceFeatures phyfeatures;
            VkPhysicalDeviceFeatures enabledphyfeatures;
            VkDevice dev;
            VkInstance instance;
            void *vulkanlib;

            uint32_t globalqueuefamily;
            VkQueue globalqueue;
            uint32_t graphicscomputefamily;
            VkQueue graphicsqueue;
            VkQueue computequeue;
            uint32_t presentfamily;
            VkQueue presentqueue;

            VkSurfaceKHR surface;
            VkSurfaceCapabilitiesKHR surfacecaps;

            VkSwapchainKHR swapchain;
            struct ORenderer::texture swaptextures[VULKAN_MAXBACKBUFFERS];
            struct ORenderer::textureview swaptextureviews[VULKAN_MAXBACKBUFFERS];
            struct ORenderer::framebuffer swapfbs[VULKAN_MAXBACKBUFFERS];
            struct ORenderer::renderpass swapfbpass; // pass swapchain framebuffers are attached to
            uint32_t swapimage;
            // struct vulkan_texture swapimages[VULKAN_MAXBACKBUFFERS];
            // VkImageView swapimageviews[VULKAN_MAXBACKBUFFERS];
            // VkFramebuffer swapchainfbs[VULKAN_MAXBACKBUFFERS];
            uint32_t swaptexturecount;
            int width, height;
            struct ORenderer::init init;

            VkDescriptorPool descpool;

            // pipelines are individual to passes in the renderer, each pipeline has a single renderpass which dictates everything associated with it, much like a view in our renderer
            // compute pipelines are done for compute views
            // so should this just be for passes instead, each "pass" does indeed work a certain way with each individual set of stages working on its own thing
            // yes, that makes sense (and the vulkan render passes just gets the view resources shoved at them)

            VkCommandPool cmdpool;
            VkCommandBuffer cmd[RENDERER_MAXLATENCY + 1]; // With additional space for tracy cmd buffer
            VulkanStream stream[RENDERER_MAXLATENCY];

            VkSemaphore imagepresent[RENDERER_MAXLATENCY];
            VkSemaphore renderdone[RENDERER_MAXLATENCY];
            VkFence framesinflight[RENDERER_MAXLATENCY];
            uint32_t frame = 0; // 0-VULKAN_MAXLATENCY

            VkFence imfence; // Immediate submission fence
            VkCommandBuffer imcmd; // Immediate command buffer
            VulkanStream imstream; // Immediate stream

            VulkanContext(struct ORenderer::init *init);
            ~VulkanContext(void);

            // on any of these creates, if the handle is not already SIZE_MAX (invalid handle), we reuse the slot (to allow us to keep any reference to a resource valid)
            uint8_t createtexture(struct ORenderer::texturedesc *desc, struct ORenderer::texture *texture);
            uint8_t createtextureview(struct ORenderer::textureviewdesc *desc, struct ORenderer::textureview *view);
            uint8_t createbuffer(struct ORenderer::bufferdesc *desc, struct ORenderer::buffer *buffer);
            uint8_t createframebuffer(struct ORenderer::framebufferdesc *desc, struct ORenderer::framebuffer *framebuffer);
            uint8_t createrenderpass(struct ORenderer::renderpassdesc *desc, struct ORenderer::renderpass *pass);
            uint8_t createpipelinestate(struct ORenderer::pipelinestatedesc *desc, struct ORenderer::pipelinestate *state);
            uint8_t createcomputepipelinestate(struct ORenderer::computepipelinestatedesc *desc, struct ORenderer::pipelinestate *state);
            uint8_t createsampler(struct ORenderer::samplerdesc *desc, struct ORenderer::sampler *sampler);

            // XXX: Think real hard about everything, how do I want to go about it?
            // Probably anything synchronous (on demand and not related to a command "stream") should be put here as we want our changes to be reflected immediately rather than have them appear at a later date.
            uint8_t mapbuffer(struct ORenderer::buffermapdesc *desc, struct ORenderer::buffermap *map);
            uint8_t unmapbuffer(struct ORenderer::buffermap map);
            uint8_t getlatency(void);
            uint8_t copybuffer(struct ORenderer::buffercopydesc *desc);
            // Request a copy of the backbuffer (in a Vulkan setting, this'll be a framebuffer pointing to the current swapchain image)
            uint8_t requestbackbuffer(struct ORenderer::framebuffer *framebuffer);
            uint8_t requestbackbufferinfo(struct ORenderer::backbufferinfo *info);
            uint8_t requestscratchbuffer(ORenderer::ScratchBuffer **scratchbuffer) {
                *scratchbuffer = &this->scratchbuffers[this->frame];
                return ORenderer::RESULT_SUCCESS;
            }
            uint8_t requestbackbuffertexture(struct ORenderer::texture *texture) {
                *texture = this->swaptextures[this->frame];
                return ORenderer::RESULT_SUCCESS;
            }
            ORenderer::Stream *getimmediate(void);
            uint8_t transitionlayout(struct ORenderer::texture texture, size_t format, size_t state);

            uint8_t submitstream(ORenderer::Stream *stream);

            void destroytexture(struct ORenderer::texture *texture);
            void destroytextureview(struct ORenderer::textureview *textureview);
            void destroybuffer(struct ORenderer::buffer *buffer);
            void destroyframebuffer(struct ORenderer::framebuffer *framebuffer);
            void destroyrenderpass(struct ORenderer::renderpass *pass);
            void destroypipelinestate(struct ORenderer::pipelinestate *state);

            void setdebugname(struct ORenderer::texture texture, const char *name);
            void setdebugname(struct ORenderer::textureview view, const char *name);
            void setdebugname(struct ORenderer::buffer buffer, const char *name);
            void setdebugname(struct ORenderer::framebuffer framebuffer, const char *name);
            void setdebugname(struct ORenderer::renderpass renderpass, const char *name);
            void setdebugname(struct ORenderer::pipelinestate state, const char *name);

            void adjustprojection(glm::mat4 *mtx) {
                (*mtx)[1][1] *= -1; // Vulkan has an inverted Y
            }

            void adjustorthoprojection(float *top, float *bottom) {
                // Swap around top and bottom as it Y is flipped in Vulkan.
                float tmp0 = *top;
                float tmp1 = *bottom;
                *top = tmp1;
                *bottom = tmp0;
            }

            void flushrange(struct ORenderer::buffer buffer, size_t size);

            VkCommandBuffer beginimmediate(void);
            void endimmediate(void);


            void execute(GraphicsPipeline *pipeline, void *cam);
            uint8_t createbackbuffer(struct ORenderer::renderpass pass, struct ORenderer::textureview *depth = NULL);
            void interpretstream(VkCommandBuffer cmd, ORenderer::Stream *stream);
            VkShaderModule createshadermodule(ORenderer::Shader shader);
            void createswapchainviews(void);
            void createswapchain(void); // XXX: Flags (presentation mode, etc.)
            void destroyswapchain(void);
            void recordcmd(VkCommandBuffer cmd, uint32_t image, VulkanStream *stream);
    };

    ORenderer::RendererContext *createcontext(void);
}

#endif
