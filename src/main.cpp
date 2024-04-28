#include <engine/renderer/camera.hpp>
#include <engine/concurrency/job.hpp>
#include <engine/engine.hpp>
#include <engine/math/math.hpp>
#include <engine/renderer/envmap.hpp>
#include <engine/game.hpp>
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

void testjob(OJob::Job *job) {
    uint64_t *args = (uint64_t *)job->param;
    ((OScene::Scene *)args[0])->partitionmanager.cull(*((ORenderer::PerspectiveCamera *)args[1]));
}

void callback(struct OResource::AsyncIO::work *work) {
    printf("work size %lu\n", work->size);
    for (size_t i = 0; i < work->size; i++) {
        printf("%c", ((char *)work->buffer)[i]);
    }
    free(work->buffer);
    free(work);
}

int main(int argc, const char **argv) {
    engine_engine = (struct engine *)malloc(sizeof(struct engine));
    glfwInit();
    engine_engine->window = glfwCreateWindow(1280, 720, "Omicron Engine", NULL, NULL);
    int32_t w, h;
    glfwGetWindowSize(engine_engine->window, &w, &h);
    engine_engine->winsize = glm::vec2(w, h);

    OJob::init();

    // renderer_context = renderer_createcontext();

    // glfwSetWindowSizeCallback(engine_engine->window, engine_winsizecallback);

    // input_init();
    // renderer_initbackend();
    struct ORenderer::init init = { 0 };
    init.platform.ndt = glfwGetX11Display();
    init.platform.nwh = (void *)glfwGetX11Window(engine_engine->window);
    init.window = engine_engine->window;
    // vulkan_init(&init);
    ORenderer::context = new OVulkan::VulkanContext(&init);

    struct ORenderer::renderer *renderer = ORenderer::initrenderer();
    // camera_init(renderer);

    OResource::RPak rpak = OResource::RPak("test.rpak");
    OResource::manager.loadrpak(&rpak);
    OResource::RPak shaders = OResource::RPak("shaders.rpak");
    OResource::manager.loadrpak(&shaders);

    // OResource::manager.create("misc/model.glb");

    // OResource::manager.create("misc/out.ktx2");

    PBRPipeline pipeline = PBRPipeline();
    pipeline.init();

    // OResource::Model::fromassimp("misc/Scene.glb", "misc/testscene.omod");

    // XXX:
    OScene::Scene scene2 = OScene::Scene();
    // OScene::GameObject *obj = OScene::GameObject::create<OScene::GameObject>();
    // obj->scene = &scene2;
    // obj->translate(glm::vec3(0.0f, 0.0f, 0.0f));
    // scene2.objects.push_back(obj->gethandle());
    //
    // OScene::GameObject *model = OScene::GameObject::create<OScene::ModelInstance>();
    // model->scene = &scene2;
    // model->parent = obj->gethandle();
    // obj->children.push_back(model->gethandle());
    // ((OScene::ModelInstance *)model)->model = ORenderer::Model("misc/testscene.omod");
    // ((OScene::ModelInstance *)model)->modelpath = "misc/testscene.omod";
    // model->translate(glm::vec3(2.0f, 1.0f, 0.0f));
    // scene2.objects.push_back(model->gethandle());



    // ((OScene::ModelInstance *)model)->model.meshes[0].material.base.texture = OResource::Texture::load("misc/out.ktx2");
    // struct ORenderer::textureviewdesc viewdesc = { };
    // viewdesc.texture = ((OScene::ModelInstance *)model)->model.meshes[0].material.base.texture;
    // viewdesc.type = ORenderer::IMAGETYPE_2D;
    // viewdesc.aspect = ORenderer::ASPECT_COLOUR;
    // viewdesc.format = ORenderer::FORMAT_RGBA8SRGB;
    // viewdesc.mipcount = 1;
    // viewdesc.baselayer = 0;
    // viewdesc.layercount = 1;
    // viewdesc.basemiplevel = 0;
    // ORenderer::context->createtextureview(&viewdesc, &((OScene::ModelInstance *)model)->model.meshes[0].material.base.view);
    // scene.objects.push_back(model);

    // XXX:
    // model = OScene::GameObject::create<OScene::ModelInstance>();
    // model->scene = &scene2;
    // model->parent = obj->gethandle();
    // obj->children.push_back(model->gethandle());
    // ((OScene::ModelInstance *)model)->model = ORenderer::Model("misc/testscene.omod");
    // ((OScene::ModelInstance *)model)->modelpath = "misc/testscene.omod";
    // model->translate(glm::vec3(2.0f, 1.0f, 1.0f));
    // scene2.objects.push_back(model->gethandle());


    // ((OScene::ModelInstance *)model)->model.meshes[0].material.base.texture = OResource::Texture::load("misc/out2.ktx2");
    // viewdesc.texture = ((OScene::ModelInstance *)model)->model.meshes[0].material.base.texture;
    // viewdesc.type = ORenderer::IMAGETYPE_2D;
    // viewdesc.aspect = ORenderer::ASPECT_COLOUR;
    // viewdesc.format = ORenderer::FORMAT_RGBA8SRGB;
    // viewdesc.mipcount = 1;
    // viewdesc.baselayer = 0;
    // viewdesc.layercount = 1;
    // viewdesc.basemiplevel = 0;
    // ORenderer::context->createtextureview(&viewdesc, &((OScene::ModelInstance *)model)->model.meshes[0].material.base.view);
    // scene.objects.push_back(model);

    OScene::setupreflection();

    // scene2.save("saved.osce");
    // scene.load("saved.osce");


    srand(time(NULL));

    OScene::Test *test = OScene::GameObject::create<OScene::Test>();
    test->scene = &scene;
    // test->flags |= OScene::GameObject::IS_INVISIBLE;
    test->model->model = new ORenderer::Model("misc/testscene.omod");
    test->model->modelpath = "misc/testscene.omod";
    test->bounds = OMath::AABB(test->model->model->bounds.min, test->model->model->bounds.max);
    // test->translate(glm::vec3(1.0f, 3.0f, 2.0f));
    // test->setrotation(glm::vec3(glm::radians((float)(rand() % 40)), 0.0f, glm::radians((float)(rand() % 40))));
    // test->silly();
    // scene.objects.push_back(test->gethandle());

    OScene::Test *m = OScene::GameObject::create<OScene::Test>();
    m->scene = &scene;
    // test->flags |= OScene::GameObject::IS_INVISIBLE;
    m->model->model = new ORenderer::Model("misc/testscene.omod");
    m->model->modelpath = "misc/testscene.omod";
    m->bounds = OMath::AABB(test->model->model->bounds.min, test->model->model->bounds.max);
    m->translate(glm::vec3(1.0f, 3.0f, 2.0f));
    m->setrotation(glm::vec3(glm::radians((float)(rand() % 40)), 0.0f, glm::radians((float)(rand() % 40))));
    m->silly();
    scene.objects.push_back(m->gethandle());

    scene2.save("saved.osce");
    scene.load("saved.osce");

    {
        ZoneScopedN("Object Creation");
        for (size_t i = 0; i < 2048; i++) {
            OScene::Test *e = OScene::GameObject::create<OScene::Test>();
            e->scene = &scene;
            // e->flags |= OScene::GameObject::IS_INVISIBLE;
            e->model->model = test->model->model;
            e->model->modelpath = "misc/testscene.omod";
            e->bounds = OMath::AABB(e->model->model->bounds.min, e->model->model->bounds.max);
            e->translate(glm::vec3(rand() % 400, rand() % 5, rand() % 400));
            e->setrotation(glm::vec3(glm::radians((float)(rand() % 40)), 0.0f, glm::radians((float)(rand() % 40))));
            e->silly();
            scene.objects.push_back(e->gethandle());
        }
    }

    // Quick hack to get an orientation quaternion that does this
    glm::mat4 viewmtx = glm::lookAt(glm::vec3(2.0f, 2.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 _0; // Scale
    glm::quat orientation;
    glm::vec3 _1; // Position
    glm::vec3 _2; // Skew
    glm::vec4 _3; // Perspective

    glm::decompose(viewmtx, _0, orientation, _1, _2, _3);


    ORenderer::PerspectiveCamera camera = ORenderer::PerspectiveCamera(glm::vec3(2.0f, 2.0f, 5.0f), orientation, 45.0f, 1280 / 720, 0.1f, 10.0f);
    glm::ivec3 loc = camera.pos * (1 / 300.0f);
    printf("would be in cell %d %d %d\n", loc.x, loc.y, loc.z);

    OResource::manager.create("openworldnotes");

    OResource::AsyncIO::loadcall("openworldnotes", 0, SIZE_MAX, callback);
    // void *buffer = NULL;
    // OResource::AsyncIO::loadwait("openworldnotes", &buffer, 0, SIZE_MAX, NULL);
    // printf("%s\n", buffer);

    // OScene::CullResult *res = scene2.partitionmanager.cull(camera);


    // OScene::GameObject::destroy(test->gethandle());

    // OResource::manager.create("misc/test.omod");
    // OResource::Model m = OResource::Model("misc/testscene.omod");

    // OUtils::HandlePointer<OResource::Resource> res = manager.create(OResource::Resource::TYPE_UNKNOWN, "misc/test.res");

    // manager.destroy(res);

    // OResource::RPak rpak = OResource::RPak("test.rpak");
    // for (size_t i = 0; i < rpak.header.num; i++) {
        // printf("%s\n", rpak.entries[i].path);
    // }


    // struct script *script = script_load("./pipelineproto.so", "testscript");
    // engine_engine->activecam = renderer->camera;
    // struct pipeline *pipeline = pipeline_init(script);

    // struct pipeline_view *view = pipeline_createview(pipeline, pipeline->configs[0]);
    // view->viewport.width = 1280;
    // view->viewport.height = 720;
    // view->viewport.x = 0;
    // view->viewport.y = 0;

    int width, height;
    int oldwidth, oldheight;
    glfwGetFramebufferSize(engine_engine->window, &width, &height);
    oldwidth = width;
    oldheight = height;

    while (!glfwWindowShouldClose(engine_engine->window)) {
        glfwPollEvents();
        oldwidth = width;
        oldheight = height;
        glfwGetFramebufferSize(engine_engine->window, &width, &height);
        if (oldwidth != width || oldheight != height) {
            pipeline.resize((struct ORenderer::rect) { .x = 0, .y = 0, .width = (uint16_t)width, .height = (uint16_t)height });
        }

        // vulkan_frame(&pipeline);
        ((OVulkan::VulkanContext *)ORenderer::context)->execute(&pipeline);
        FrameMark;
        // break;
        // return 0;
    }

    OJob::destroy();
    delete ORenderer::context;

    return 0;
    // engine_init();
    //
    // // model2 = model_init("pbrplane.glb", false, true);
    // // struct model *model3 = model_init("untitled.glb", false, true);
    //
    // bgfx_vertex_layout_begin(&layout, bgfx_get_renderer_type());
    // bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_POSITION, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    // bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_TEXCOORD0, 2, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    // bgfx_vertex_layout_end(&layout);
    // vb = bgfx_create_vertex_buffer(bgfx_make_ref(bgfx_get_caps()->originBottomLeft ? flipquadvertices : quadvertices, sizeof(quadvertices)), &layout, BGFX_BUFFER_NONE);
    // ib = bgfx_create_index_buffer(bgfx_make_ref(quadindices, sizeof(quadindices)), BGFX_BUFFER_NONE);
    //
    // // vec2s lastmousepos = (vec2s) { 0.0, 0.0 };
    // glm::vec2 lastmousepos = glm::vec2(0.0, 0.0);
    // glm::vec2 deltamousepos = glm::vec2(0.0, 0.0);
    // glm::vec2 deltamouseposn = glm::vec2(0.0, 0.0);
    //
    // // vec2s deltamousepos = (vec2s) { 0.0, 0.0 };
    // // vec2s deltamouseposn = (vec2s) { 0.0, 0.0 };
    //
    // struct envmap *env = NULL;
    //
    // // bgfx_texture_handle_t skybox = utils_loadcubemapfromktx("/home/rtw/Desktop/output_skybox.ktx", false);
    //
    // // game_manager.globalrenderer->camera->pos.y = 1.5f;
    //
    // size_t oldw = 0;
    // size_t oldh = 0;
    //
    // #include <scripting/script.hpp>
    //
    // // struct renderer *renderer = renderer_initrenderer();
    // // camera_init(renderer);
    // //
    // // struct script *script = script_load("./pipelineproto.so", "testscript");
    // // engine_engine->activecam = renderer->camera;
    // // struct pipeline *pipeline = pipeline_init(script);
    // // pipeline->views[1]->renderer = renderer;
    //
    // // struct game_object *obj = game_initobj(&game_manager);
    // // game_objbindscript(obj, script);
    // // obj->ready(obj);
    // // obj->update(obj);
    // // return 0;
    //
    // int64_t lasttick = 0; // last tick (for delta calculations)
    // uint32_t frame = 0;
    // glm::vec4 luminescence = glm::vec4(0.0);
    // float average = 0.0f;
    // while (!glfwWindowShouldClose(engine_engine->window)) {
    //     input_update(); // per input tick (preferrably a constant rate of 60hz or something high like 128hz, game tickrate exact)
    //     input_updateframe(); // per frame (for stuff that depends on framerate for updates and would be expected to update faster as framerate climbs)
    //
    //     int64_t now = utils_getcounter();
    //     int64_t delta = now - lasttick;
    //     float deltaseconds = delta / 1000000.0f;
    //     lasttick = now;
    //
    //     if (frame == 0) {
    //         env = envmap_initfromfile("/home/rtw/Desktop/output_skybox2.ktx2");
    //     }
    //
    //     glm::vec3 movedirection = glm::vec3(sinf(engine_engine->activecam->horizontal), 0.0f, cosf(engine_engine->activecam->horizontal));
    //     glm::vec3 movevector = glm::vec3(0.0);
    //
    //     // if (input_controlleraxis(&game_manager.inputmanager, 0, INPUT_GAMEPADAXISLEFTY) < 0.0f) {
    //     if (input_manager.keys[GLFW_KEY_W]) {
    //         movevector = (movedirection * glm::vec3(1.0f, 0.0f, 1.0f)) + movevector;
    //     // } else if (input_controlleraxis(&game_manager.inputmanager, 0, INPUT_GAMEPADAXISLEFTY) > 0.0f) {
    //     } else if (input_manager.keys[GLFW_KEY_S]) {
    //         // movevector = glms_vec3_add(glms_vec3_mul(movedirection, (vec3s) { -1.0f, -0.0f, -1.0f }), movevector);
    //         movevector = (movedirection * glm::vec3(-1.0f, 0.0f, -1.0f)) + movevector;
    //     }
    //
    //     // if (input_controlleraxis(&game_manager.inputmanager, 0, INPUT_GAMEPADAXISLEFTX) < 0.0f) {
    //     if (input_manager.keys[GLFW_KEY_A]) {
    //         // movevector = glms_vec3_add(glms_vec3_mul(engine_engine->activecam->right, (vec3s) { 1.0f, 0.0f, 1.0f }), movevector);
    //         (engine_engine->activecam->right * glm::vec3(1.0f, 0.0f, 1.0f)) + movevector;
    //     // } else if (input_controlleraxis(&game_manager.inputmanager, 0, INPUT_GAMEPADAXISLEFTX) > 0.0f) {
    //     } else if (input_manager.keys[GLFW_KEY_D]) {
    //         (engine_engine->activecam->right * glm::vec3(-1.0f, 0.0f, -1.0f)) + movevector;
    //         // movevector = glms_vec3_add(glms_vec3_mul(engine_engine->activecam->right, (vec3s) { -1.0f, 0.0f, -1.0f }), movevector);
    //     }
    //
    //     // normalise movement vector so we don't end up moving faster on a diagonal
    //     // movevector = glms_vec3_normalize(movevector);
    //     // movevector = glms_vec3_muladds(movevector, 10.0f, (vec3s) { 0 }); // 10.0f is the movement speed :)
    //     // movevector = glms_vec3_muladds(movevector, deltaseconds, (vec3s) { 0 }); // multiply by delta to get tick length independant movement (this should be in the physics update)
    //     // engine_engine->activecam->pos = glms_vec3_add(engine_engine->activecam->pos, movevector);
    //     movevector = glm::normalize(movevector);
    //     movevector *= 10.0f * deltaseconds;
    //
    //     deltamousepos = input_manager.mousepos - lastmousepos;
    //     // deltamousepos = glms_vec2_sub(input_manager.mousepos, lastmousepos);
    //     deltamousepos = deltamousepos / engine_engine->winsize;
    //     // deltamouseposn = glms_vec2_div(deltamousepos, engine_engine->winsize);
    //     lastmousepos = input_manager.mousepos;
    //
    //     engine_engine->activecam->vertical -= input_manager.mousesensitivity.y * 2.5f * deltamouseposn.y;
    //     engine_engine->activecam->horizontal += input_manager.mousesensitivity.x * 2.5f * deltamouseposn.x;
    //     engine_engine->activecam->rect = engine_engine->winsize;
    //     // printf("main: %llx\n", engine_engine->activecam);
    //
    //     camera_update(engine_engine->activecam); // camera updates should always occur AFTER the camera is modified (or else it doesn't properly align the camera position (used for lighting calculations) with the projection matrix)
    //
    //     bool sizechanged = false;
    //     if (oldw != engine_engine->winsize.x || oldh != engine_engine->winsize.y) {
    //         // render_updatescene(game_manager.globalrenderer->window.winsize, game_manager.globalrenderer->camera);
    //         sizechanged = true;
    //         bgfx_set_view_rect(0, 0, 0, engine_engine->winsize.x, engine_engine->winsize.y);
    //         bgfx_reset(engine_engine->winsize.x, engine_engine->winsize.y, BGFX_RESET_VSYNC, BGFX_TEXTURE_FORMAT_COUNT);
    //     }
    //     oldw = engine_engine->winsize.x;
    //     oldh = engine_engine->winsize.y;
    //
    //     // bgfx_touch(pipeline_vmain);
    //     // if (sizechanged) {
    //     //     for (ssize_t i = pipeline->viewidx - 1; i >= 0; i--) {
    //     //         if (pipeline->views[i]->renderer != NULL) {
    //     //             pipeline->views[i]->resize(pipeline->views[i], engine_engine->winsize);
    //     //         }
    //     //     }
    //     // }
    //     // for (ssize_t i = pipeline->viewidx - 1; i >= 0; i--) {
    //     //     if (pipeline->views[i]->renderer != NULL) {
    //     //         pipeline->views[i]->render(pipeline->views[i], 0);
    //     //     }
    //     // }
    //     // pipeline.render(&pipeline, renderer, (frame == 0 ? RENDERER_RENDERFLAGFIRST : 0) | (sizechanged && frame != 0 ? RENDERER_RENDERFLAGRECTUPDATED : 0));
    //
    //     // checkerboard
    //     // if (frame % 2 != 0) { // render on every even frame
    //         // bgfx_set_view_frame_buffer(VIEW_LIGHT, f_cbr[0]);
    //         // bgfx_set_view_frame_buffer(VIEW_ZPASS, f_cbr[0]);
    //     // } else { // render on every odd frame
    //         // proj = glms_mat4_mul(glms_translate_make((vec3s) { 2.0f / (cam.rect.x), 0.0f, 0.0f }), proj); // projection is intentionally skewed off to the side by a quarter of a pixel (to fill in checkerboard gaps)
    //         // bgfx_set_view_frame_buffer(VIEW_LIGHT, f_cbr[1]);
    //         // bgfx_set_view_frame_buffer(VIEW_ZPASS, f_cbr[1]);
    //     // }
    //     // bgfx_set_view_frame_buffer(v_light, f_hdr->framebuffer);
    //     // bgfx_set_view_frame_buffer(v_zpass, f_hdr->framebuffer);
    //     //
    //     // bgfx_set_view_transform(v_zpass, &game_manager.globalrenderer->camera->view, &game_manager.globalrenderer->camera->proj);
    //     // bgfx_set_view_transform(v_light, &game_manager.globalrenderer->camera->view, &game_manager.globalrenderer->camera->proj);
    //
    //     // ensure clear so we don't have weird "freezing"
    //     // bgfx_touch(v_zpass);
    //     // bgfx_touch(v_light);
    //     //
    //     // mat4s mtx = glms_mat4_identity();
    //     // mtx = glms_translate(mtx, (vec3s) { 0.0f, 0.0f, 0.0f });
    //     // mtx = glms_scale(mtx, (vec3s) { 1.0f, 1.0f, 1.0f });
    //     // model_draw(model3, p_depthprepass, v_zpass, BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CCW | BGFX_STATE_MSAA, mtx, false, game_manager.globalrenderer->camera, true, NULL);
    //     //
    //     // mtx = glms_mat4_identity();
    //     // mtx = glms_translate(mtx, (vec3s) { 5.0f, 1.0f, 0.0f });
    //     // mtx = glms_scale(mtx, (vec3s) { 1.0f, 1.0f, 1.0f });
    //     // model_draw(model3, p_depthprepass, v_zpass, BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CCW | BGFX_STATE_MSAA, mtx, false, game_manager.globalrenderer->camera, true, NULL);
    //     //
    //     // mtx = glms_mat4_identity();
    //     // mtx = glms_translate(mtx, (vec3s) { 0.0f, 0.0f, 0.0f });
    //     // mtx = glms_scale(mtx, (vec3s) { 1.0f, 1.0f, 1.0f });
    //     // model_draw(model3, p_pbrclustered, v_light, BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_LEQUAL | BGFX_STATE_CULL_CCW | BGFX_STATE_BLEND_ALPHA | BGFX_STATE_MSAA, mtx, false, game_manager.globalrenderer->camera, false, env);
    //     //
    //     // mtx = glms_mat4_identity();
    //     // mtx = glms_translate(mtx, (vec3s) { 5.0f, 1.0f, 0.0f });
    //     // mtx = glms_scale(mtx, (vec3s) { 1.0f, 1.0f, 1.0f });
    //     // model_draw(model3, p_pbrclustered, v_light, BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_LEQUAL | BGFX_STATE_CULL_CCW | BGFX_STATE_BLEND_ALPHA | BGFX_STATE_MSAA, mtx, false, game_manager.globalrenderer->camera, false, env);
    //     //
    //     //
    //     // mat4s vc;
    //     // memcpy(&vc, &game_manager.globalrenderer->camera->view, sizeof(mat4s));
    //     // ((float *)&vc)[12] = 0.0f;
    //     // ((float *)&vc)[13] = 0.0f;
    //     // ((float *)&vc)[14] = 0.0f;
    //     //
    //     // mat4s vmtx = glms_mat4_inv(vc);
    //     // bgfx_set_uniform(u_params->uniform, ((vec4s[]) { (vec4s) { game_manager.globalrenderer->camera->fov, 0.0f, 0.0f, 0.0f }, vmtx.col[0], vmtx.col[1], vmtx.col[2], vmtx.col[3] }), 5);
    //     // bgfx_set_texture(0, s_quadtexture->uniform, skybox, UINT32_MAX);
    //     // mat4s ortho;
    //     // mtx_ortho((float *)&ortho, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 2.0f, 0.0f, bgfx_get_caps()->homogeneousDepth);
    //     // renderscreenquad(v_skybox, p_skybox->program, true);
    //     //
    //     // // tonemapping luminescence calculations
    //     // float minlum = -5.0f;
    //     // float rangelum = 10.0f;
    //     // vec4s histparams = (vec4s) { minlum, 1.0f / (rangelum - minlum), game_manager.globalrenderer->window.winsize.x, game_manager.globalrenderer->window.winsize.y };
    //     // bgfx_set_image(0, t_hdr[0]->texture, 0, BGFX_ACCESS_READ, BGFX_TEXTURE_FORMAT_RGBA16F);
    //     // bgfx_set_compute_dynamic_index_buffer(1, b_histogram->dindex, BGFX_ACCESS_WRITE);
    //     // bgfx_set_uniform(u_histparams->uniform, &histparams, 1);
    //     // bgfx_dispatch(v_histogram, p_histogram->program, ceilf(game_manager.globalrenderer->window.winsize.x / 16.0f), ceilf(game_manager.globalrenderer->window.winsize.y / 16.0f), 1, BGFX_DISCARD_ALL);
    //     //
    //     // float tau = 1.1f;
    //     // float timecoeff = glm_clamp(1.0f - exp(-deltaseconds * tau), 0.0f, 1.0f);
    //     // vec4s avgparams = (vec4s) { minlum, rangelum - minlum, timecoeff, game_manager.globalrenderer->window.winsize.x * game_manager.globalrenderer->window.winsize.y };
    //     // bgfx_set_image(0, t_lumavg->texture, 0, BGFX_ACCESS_READWRITE, BGFX_TEXTURE_FORMAT_R16F);
    //     // bgfx_set_compute_dynamic_index_buffer(1, b_histogram->dindex, BGFX_ACCESS_READWRITE);
    //     // bgfx_set_uniform(u_avgparams->uniform, &avgparams, 1);
    //     // bgfx_dispatch(v_average, p_average->program, 1, 1, 1, BGFX_DISCARD_ALL);
    //     //
    //     // bgfx_set_texture(0, s_quadtexture->uniform, t_hdr[0]->texture, UINT32_MAX);
    //     // bgfx_set_texture(1, s_avglum->uniform, t_lumavg->texture, BGFX_SAMPLER_POINT | BGFX_SAMPLER_UVW_CLAMP);
    //     // bgfx_set_view_transform(v_post, &game_manager.globalrenderer->camera->view, &ortho);
    //     // renderscreenquad(v_post, p_tonemap->program, false);
    //
    //     // fxaa + blit to display (FXAA occurs as last post processing step so we blit (saves us from blitting twice))
    //     // XXX: SIGABRT for no reason?
    //     // vec4s fxaaparams = (vec4s) { bgfx_get_renderer_type() == BGFX_RENDERER_TYPE_DIRECT3D9 ? 0.5f : 0.0f, 0.0f, 0.0f, 0.0f };
    //     // bgfx_set_texture(0, s_quadtexture->uniform, t_hdr[0]->texture, BGFX_SAMPLER_POINT | BGFX_SAMPLER_UVW_CLAMP);
    //     // bgfx_set_texture(0, s_quadtexture->uniform, t_post->texture, BGFX_SAMPLER_POINT | BGFX_SAMPLER_UVW_CLAMP);
    //     // bgfx_set_uniform(u_params->uniform, &fxaaparams, 1);
    //     // bgfx_set_view_transform(v_main, &game_manager.globalrenderer->camera->view, &ortho);
    //     // renderscreenquad(v_main, p_fxaa->program, false); // FINAL RENDERING STEP
    //
    //     // XXX: hang on pthread_cond_wait() randomly (issue with a long duration shader? bgfx breaks? possible memory leak or otherwise? it's definitely something to do with frame latency)
    //     frame = bgfx_frame(false);
    //
    //     // resource_kick();
    //     input_resetpoll();
    // }
    //
    // // resource_cpufreeall();
    // // resource_gpufreeall();
    // bgfx_shutdown();

    // return 0;
}
