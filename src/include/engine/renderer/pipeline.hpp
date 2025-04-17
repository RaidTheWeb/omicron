#ifndef _ENGINE__RENDERER__PIPELINE_HPP
#define _ENGINE__RENDERER__PIPELINE_HPP

#include <engine/renderer/renderer.hpp>
#include <GLFW/glfw3.h>

extern GLFWwindow *window;
extern glm::vec2 winsize;

class GraphicsPipeline {
    public:
        virtual void execute(ORenderer::Stream *stream, void *cam) { };
        virtual void postexecute(void) { };
};


#endif
