#include <assertion.hpp>
#include <engine/engine.hpp>
#include <engine/event.hpp>
#include <engine/input.hpp>

struct engine *engine_engine = NULL;

static void engine_winsizecallback(GLFWwindow *window, int32_t width, int32_t height) {
    engine_engine->winsize = glm::vec2(width, height);
    struct event event = { 0 };
    event.type = utils_stringid("omicron_winsize");
    event.numargs = 2;
    event.args[0] = width;
    event.args[1] = height;
    event.deliverytime = utils_getcounter(); // immediate dispatch
    event_queue(event);
}

void engine_init(void) {
    engine_engine = (struct engine *)malloc(sizeof(struct engine));

    OJob::init();
    event_init();
    // resource_init();

    glfwInit();
    engine_engine->window = glfwCreateWindow(1280, 720, "Omicron Engine", NULL, NULL);
    int32_t w, h;
    glfwGetWindowSize(engine_engine->window, &w, &h);
    engine_engine->winsize = glm::vec2(w, h);

    glfwSetWindowSizeCallback(engine_engine->window, engine_winsizecallback);

    input_init();

    bgfx_platform_data_t platform = { 0 };
#ifdef __linux__
    platform.ndt = glfwGetX11Display();
    platform.nwh = (void *)glfwGetX11Window(engine_engine->window);
#endif

    bgfx_render_frame(-1);
    bgfx_init_t init = { };
    bgfx_init_ctor(&init);
    init.platformData = platform;
#ifdef OMICRON_RENDEROPENGL
    init.type = BGFX_RENDERER_TYPE_OPENGL;
#elif OMICRON_RENDERVULKAN
    init.type = BGFX_RENDERER_TYPE_VULKAN;
#endif
    // init.debug = true;
    init.resolution.width = engine_engine->winsize.x;
    init.resolution.height = engine_engine->winsize.y;
    ASSERT(bgfx_init(&init), "Failed to initialise BGFX.\n");

    bgfx_set_debug(BGFX_DEBUG_STATS);

    bgfx_reset(engine_engine->winsize.x, engine_engine->winsize.y, BGFX_RESET_VSYNC, init.resolution.format);
}
