#ifndef _RENDERER__PIPELINE__PBRPIPELINE_HPP
#define _RENDERER__PIPELINE__PBRPIPELINE_HPP

#include <renderer/pipeline.hpp>

class PBRPipeline : public GraphicsPipeline {
    public:
        // all pipeline global resources must be defined in here
        void init(void);
        void resize(struct ORenderer::rect rendersize);
        void update(uint64_t flags);
        void execute(ORenderer::Stream *stream);
};

class PBRPrimitivePass : public GraphicsPipelinePass {

};

class MainView : public GraphicsPipelineView {
    public:

        void init(void);
        void resize(struct ORenderer::rect rendersize);
        void update(uint64_t flags);
        void execute(void);
};

#endif
