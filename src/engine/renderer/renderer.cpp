#include <engine/renderer/renderer.hpp>
#if defined (OMICRON_RENDERVULKAN)
#include <engine/renderer/backend/vulkan.hpp>
#endif

namespace ORenderer {

    // struct renderer_capabilities renderer_capabilities = { 0 };
    struct platformdata platformdata = { 0 };

    RendererContext *context = NULL;

    RendererContext *createcontext(void) {
#if defined(OMICRON_RENDERVULKAN)
        return OVulkan::createcontext();
#endif
    }

}
