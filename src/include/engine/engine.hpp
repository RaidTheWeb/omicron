#ifndef _ENGINE__ENGINE_HPP
#define _ENGINE__ENGINE_HPP

#include <GLFW/glfw3.h>
#ifdef __linux__
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#endif
#include <engine/math/math.hpp>
#include <engine/renderer/renderer.hpp>

struct engine {
    glm::vec2 winsize;
    GLFWwindow *window;

    struct camera *activecam; // current camera
};

extern struct engine *engine_engine;

void engine_init(void);

#endif
