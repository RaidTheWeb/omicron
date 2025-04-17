#include <engine/renderer/camera.hpp>
#include <engine/concurrency/job.hpp>
#include <engine/math/math.hpp>
#include <GLFW/glfw3.h>
#ifdef __linux__
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#endif
#include <engine/light.hpp>
#include <engine/matrix.hpp>
#include <engine/renderer/renderer.hpp>
#include <engine/resources/resource.hpp>
#include <stdio.h>
#include <string.h>
#include <engine/renderer/backend/vulkan.hpp>
#include <engine/renderer/pipeline/pbrpipeline.hpp>
#include <engine/renderer/texture.hpp>

#include <engine/resources/rpak.hpp>
#include <engine/resources/texture.hpp>

#include <engine/scene/partition.hpp>
#include <engine/scene/scene.hpp>
#include <engine/utils/containers.hpp>
#include <engine/utils/pointers.hpp>

#include <engine/resources/model.hpp>
#include <engine/resources/asyncio.hpp>
#include <engine/resources/serialise.hpp>

#include <engine/utils/reflection.hpp>

#include <tracy/Tracy.hpp>

extern OScene::Scene scene;
GLFWwindow *window;
glm::vec2 winsize;

bool keys[348] = { 0 };

glm::vec2 mousepos = glm::vec3(0.0f);

static void keycallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    keys[key] = action;
}

int main(int argc, const char **argv) {
    memset(keys, 0, sizeof(keys));
    glfwInit();
    window = glfwCreateWindow(1280, 720, "Omicron Engine", NULL, NULL);
    int32_t w, h;
    glfwGetWindowSize(window, &w, &h);
    winsize = glm::vec2(w, h);

    glfwSetKeyCallback(window, keycallback);

    OJob::init();

    struct ORenderer::init init = { 0 };
    init.platform.ndt = glfwGetX11Display();
    init.platform.nwh = (void *)glfwGetX11Window(window);
    init.window = window;
    ORenderer::context = new OVulkan::VulkanContext(&init);

    OResource::RPak rpak = OResource::RPak("test.rpak");
    OResource::manager.loadrpak(&rpak);
    OResource::RPak shaders = OResource::RPak("shaders.rpak");
    OResource::manager.loadrpak(&shaders);

    PBRPipeline pipeline = PBRPipeline();
    pipeline.init();

    OResource::Model::fromassimp("misc/spray_paint_bottles_4k.gltf", "misc/test2.omod");
    OResource::manager.create("out.otex");

    OUtils::Handle<OResource::Resource> tex = ORenderer::texturemanager.create("misc/test.otex");
    struct ORenderer::Texture::updateinfo info = { };
    info.timestamp = utils_getcounter();
    info.resolution = tex->as<ORenderer::Texture>()->headers.header.levelcount - 1;

    // XXX:
    OScene::Scene scene2 = OScene::Scene();

    OScene::setupreflection();

    srand(time(NULL));

    OScene::Test *m = OScene::GameObject::create<OScene::Test>();
    m->scene = &scene2;
    // test->flags |= OScene::GameObject::IS_INVISIBLE;
    OResource::manager.create("misc/test2.omod*", new ORenderer::Model("misc/test2.omod"));
    m->model->model = OResource::manager.get("misc/test2.omod*");
    m->model->modelpath = "misc/test2.omod";
    printf("bounds.\n");
    m->bounds = OMath::AABB(m->model->model->as<ORenderer::Model>()->bounds.min, m->model->model->as<ORenderer::Model>()->bounds.max);
    printf("done.\n");
    m->translate(glm::vec3(1.0f, 0.0f, 3.0f));
    m->scaleby(glm::vec3(8.0));
    m->setrotation(glm::vec3(glm::radians(0.0f), glm::radians(0.0f), glm::radians(0.0f)));
    m->silly();
    scene2.objects.push_back(m->gethandle());

    {
        ZoneScopedN("Object Creation");
        for (size_t i = 0; i < 4096; i++) {
            // break;
            OScene::Test *e = OScene::GameObject::create<OScene::Test>();
            e->scene = &scene2;
            // e->flags |= OScene::GameObject::IS_INVISIBLE;
            e->model->model = m->model->model;
            e->model->modelpath = "misc/test2.omod";
            e->bounds = m->bounds;
            e->translate(glm::vec3(rand() % 40000, rand() % 5, rand() % 40000));
            e->setrotation(glm::vec3(glm::radians((float)(rand() % 40)), 0.0f, glm::radians((float)(rand() % 40))));
            e->silly();
            scene2.objects.push_back(e->gethandle());
        }
    }

    scene2.save("saved.osce");
    scene.load("saved.osce");

    int width, height;
    int oldwidth, oldheight;
    glfwGetFramebufferSize(window, &width, &height);
    oldwidth = width;
    oldheight = height;

    glm::mat4 viewmtx = glm::lookAt(glm::vec3(2.0f, 2.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 _0; // Scale
    glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 _1; // Position
    glm::vec3 _2; // Skew
    glm::vec4 _3; // Perspective

    glm::decompose(viewmtx, _0, orientation, _1, _2, _3);

    ORenderer::PerspectiveCamera camera = ORenderer::PerspectiveCamera(glm::vec3(2.0f, 2.0f, 5.0f), glm::rotate(orientation, glm::radians(-10.0f), glm::vec3(0.0f, 1.0f, 0.0f)), 45.0f, 1280 / (float)720, 0.1f, 1000.0f);

    int64_t last = utils_getcounter();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int64_t now = utils_getcounter();
        const float delta = (now - last) / 1000000.0f;
        last = now;

        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        mousepos.x = mx;
        mousepos.y = my;

        glm::vec3 current = camera.pos;
        if (keys[GLFW_KEY_W]) {
            current.z -= 1 * delta;
        } else if (keys[GLFW_KEY_S]) {
            current.z += 1 * delta;
        }

        glm::quat curr = camera.orientation;
        if (keys[GLFW_KEY_LEFT]) {
            curr = glm::rotate(curr, glm::radians(-30.0f) * delta, glm::vec3(0.0f, 1.0f, 0.0f));
        } else if (keys[GLFW_KEY_RIGHT]) {
            curr = glm::rotate(curr, glm::radians(30.0f) * delta, glm::vec3(0.0f, 1.0f, 0.0f));
        }
        camera.setorientation(curr);

        if (keys[GLFW_KEY_A]) {
            current.x -= 1 * delta;
        } else if (keys[GLFW_KEY_D]) {
            current.x += 1 * delta;
        }
        camera.setpos(current);

        oldwidth = width;
        oldheight = height;
        glfwGetFramebufferSize(window, &width, &height);
        if (oldwidth != width || oldheight != height) {
            pipeline.resize((struct ORenderer::rect) { .x = 0, .y = 0, .width = (uint16_t)width, .height = (uint16_t)height });
        }

        ((OVulkan::VulkanContext *)ORenderer::context)->execute(&pipeline, &camera);
        FrameMark; // Tracy frame mark.
    }

    OJob::destroy();
    delete ORenderer::context;

    return 0;
}
