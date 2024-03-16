#ifndef _ENGINE__RENDERER__PIPELINE_HPP
#define _ENGINE__RENDERER__PIPELINE_HPP

#include <engine/renderer/renderer.hpp>

// hierachy:
// pipeline
//      - pipeline resources
//      - views
//          - view resources
//      - stages
//          - view resources
//          - passes
//              - pass resources

class GraphicsPipeline;

class GraphicsPipelineView {
    private:
        GraphicsPipeline *pipeline;
    public:

        struct camera *source;
        size_t id;

        // every view has to have its own render target (because every view is a camera)
        struct ORenderer::texture rt;
        struct ORenderer::texture depth;

        void init(GraphicsPipeline *pipeline); // when view is initialised: call this
        void resize(struct ORenderer::rect rendersize); // when source is resized: call this
        bool update(void); // when settings are updated: call this
        void execute(ORenderer::Stream *stream); // execute under view
};

class GraphicsPipeline {
    public:
        std::vector<GraphicsPipelineView> views; // list of views bound into the pipeline

        // virtual void init(void); // global pipeline resource initialisation, fall through into stage initialisation and view initialisation
        // virtual void resize(struct renderer_rect rendersize); // renderering resolution has changed, fall through into stage and view resize
        // virtual void update(uint64_t flags); // graphics settings have changed, fall through into stage and view update

        // void execute(size_t id); // execute pipeline on view (id based)
        virtual void execute(ORenderer::Stream *stream) { };
};

// pass contains resources per-stage
class GraphicsPipelinePass {
    public:
        struct ORenderer::pipelinestate state;
        struct ORenderer::framebuffer fb;
        struct ORenderer::renderpass pass;

        void createpipelinestate(void); // should be called on any recreation of resources or initial creation
};

class GraphicsPipelineStage {
    public:
        void init(void); // per-stage resource initialisation
        void resize(struct ORenderer::rect rendersize); // renderering resolution has changed
        bool update(void); // settings changed

        bool isactive(uint64_t flags); // should this stage be enabled?
};


#endif
