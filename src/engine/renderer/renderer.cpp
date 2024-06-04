#include <engine/renderer/renderer.hpp>
#if defined (OMICRON_RENDERVULKAN)
#include <engine/renderer/backend/vulkan.hpp>
#endif

namespace ORenderer {

    // struct renderer_capabilities renderer_capabilities = { 0 };
    struct platformdata platformdata = { 0 };

    std::map<uint32_t, struct pipelinestate> pipelinestatecache;

    uint32_t serialisepipeline(struct pipelinestatedesc *desc) {
        return 0;
    }

    RendererContext *context = NULL;

    RendererContext *createcontext(void) {
#if defined(OMICRON_RENDERVULKAN)
        return OVulkan::createcontext();
#endif
    }

    struct game_object **visible(struct renderer *renderer) {
        return NULL;
    }

    struct renderer *initrenderer(void) {
        struct renderer *renderer = (struct renderer *)malloc(sizeof(struct renderer));

        renderer->local = (struct localstorage *)malloc(sizeof(struct localstorage));
        renderer->local->head = NULL;
        renderer->local->tail = NULL;
        renderer->local->size.store(0);
        renderer->local->capacity = RENDERER_MAXRESOURCES;
        // job_initmutex(&renderer->local->mutex);
        return renderer;
    }

}
