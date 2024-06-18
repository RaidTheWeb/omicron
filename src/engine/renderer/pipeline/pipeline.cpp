#include <engine/engine.hpp>

#include <engine/renderer/bindless.hpp>
#include <engine/renderer/pipeline/pbrpipeline.hpp>
#include <engine/renderer/renderer.hpp>
#include <engine/renderer/im3d.hpp>
#include <engine/renderer/mesh.hpp>
#include <engine/resources/texture.hpp>

struct ORenderer::pipelinestate state;
struct ORenderer::renderpass rpass;
struct ORenderer::framebuffer fb = { };
struct ORenderer::buffer buffer;
struct ORenderer::buffer idxbuffer;
struct ORenderer::buffer ubo;
struct ORenderer::texture tex = { };
struct ORenderer::textureview texview = { };
struct ORenderer::texture depthtex = { };
struct ORenderer::textureview depthtexview = { };
struct ORenderer::buffermap ubomap = { };
struct ORenderer::texture outtex = { };
struct ORenderer::textureview outtexview = { };
struct ORenderer::sampler outsampler = { };
uint32_t samplerid;
struct ORenderer::sampler outsampler2 = { };
struct ORenderer::sampler outsampler3 = { };
ORenderer::Model model;

struct ubo {
    uint32_t sampler;
    uint32_t base;
    uint32_t normal;
    uint32_t mrid;
    glm::mat4 model;
    glm::mat4 viewproj;
    glm::vec3 campos;
};

struct vertex {
    glm::vec3 pos;
    glm::vec3 colour;
    glm::vec2 texcoord;
};

struct ORenderer::rect renderrect = { 0, 0, 1280, 720 };

glm::vec4 im3dvtx[] = {
    glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f),
    glm::vec4(1.0f, -1.0f, 0.0f, 1.0f),
    glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f),
    glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)
};

struct im3dubo {
    glm::mat4 viewproj;
    glm::vec2 viewport;
};

ORenderer::Shader im3dpt[2] = { };
struct ORenderer::pipelinestate im3dptstate;
struct ORenderer::buffer im3dubo;
struct ORenderer::buffermap im3dubomap;
struct ORenderer::buffer im3dvtxbuffer;
struct ORenderer::buffer im3dvtxdata;
struct ORenderer::buffermap im3dvtxdatamap;
struct ORenderer::vertexlayout im3dlayout;
int64_t lastframetimestamp = 0;

#include <engine/renderer/im.hpp>
ORenderer::ImCanvas canvas;

#include <engine/renderer/backend/vulkan.hpp>

ORenderer::ImGuiRenderer imr;

void PBRPipeline::init(void) {
    ORenderer::Shader stages[2] = { ORenderer::Shader("shaders/test.vert.spv", ORenderer::SHADER_VERTEX), ORenderer::Shader("shaders/test.frag.spv", ORenderer::SHADER_FRAGMENT) };
    im3dpt[0] = ORenderer::Shader("shaders/im3dpt.vert.spv", ORenderer::SHADER_VERTEX);
    im3dpt[1] = ORenderer::Shader("shaders/im3dpt.frag.spv", ORenderer::SHADER_FRAGMENT);
    // bindless = ORenderer::BindlessManager(ORenderer::context);

    struct ORenderer::blendattachmentdesc attachment = { };
    attachment.writemask = ORenderer::WRITE_RGBA;
    attachment.blend = false;

    struct ORenderer::blendstatedesc blendstate = { };
    blendstate.logicopenable = false;
    blendstate.logicop = ORenderer::LOGICOP_COPY;
    blendstate.attachmentcount = 1;
    blendstate.attachments = &attachment;

    struct ORenderer::vtxbinddesc bind = { };
    bind.rate = ORenderer::RATE_VERTEX;
    bind.layout = ORenderer::Mesh::getlayout(0);

    struct ORenderer::vtxinputdesc vtxinput = { };
    vtxinput.bindcount = 1;
    vtxinput.binddescs = &bind;

    struct ORenderer::rasteriserdesc rasteriser = { };
    rasteriser.depthclamp = false;
    rasteriser.rasteriserdiscard = false;
    rasteriser.polygonmode = ORenderer::POLYGON_FILL;
    rasteriser.cullmode = ORenderer::CULL_BACK;
    rasteriser.frontface = ORenderer::FRONT_CCW;
    rasteriser.depthbias = false;
    rasteriser.depthbiasconstant = 0.0f;
    rasteriser.depthclamp = 0.0f;
    rasteriser.depthbiasslope = 0.0f;

    struct ORenderer::multisampledesc multisample = { };
    multisample.sampleshading = false;
    multisample.samples = ORenderer::SAMPLE_X1;
    multisample.minsampleshading = 1.0f;
    multisample.samplemask = NULL;
    multisample.alphatocoverage = false;
    multisample.alphatoone = false;

    struct ORenderer::backbufferinfo backbufferinfo = { };
    ASSERT(ORenderer::context->requestbackbufferinfo(&backbufferinfo) == ORenderer::RESULT_SUCCESS, "Failed to request backbuffer information.\n");

    struct ORenderer::rtattachmentdesc rtattachment = { };
    rtattachment.format = backbufferinfo.format; // since we're going to be using the backbuffer as our render attachment we have to use the format we specified
    rtattachment.samples = ORenderer::SAMPLE_X1;
    rtattachment.loadop = ORenderer::LOADOP_CLEAR;
    rtattachment.storeop = ORenderer::STOREOP_STORE;
    rtattachment.stencilloadop = ORenderer::LOADOP_DONTCARE;
    rtattachment.stencilstoreop = ORenderer::STOREOP_DONTCARE;
    rtattachment.initiallayout = ORenderer::LAYOUT_UNDEFINED;
    rtattachment.finallayout = ORenderer::LAYOUT_COLOURATTACHMENT; // Shift to colour attachment for our next pass.

    struct ORenderer::rtattachmentdesc depthattachment = { };
    depthattachment.format = ORenderer::FORMAT_D32F;
    depthattachment.samples = ORenderer::SAMPLE_X1;
    depthattachment.loadop = ORenderer::LOADOP_CLEAR;
    depthattachment.storeop = ORenderer::STOREOP_STORE; // We'd like the depth information to be made available at a later date
    depthattachment.stencilloadop = ORenderer::LOADOP_DONTCARE;
    depthattachment.stencilstoreop = ORenderer::STOREOP_DONTCARE;
    depthattachment.initiallayout = ORenderer::LAYOUT_UNDEFINED;
    depthattachment.finallayout = ORenderer::LAYOUT_DEPTHSTENCILATTACHMENT;

    struct ORenderer::rtattachmentrefdesc colourref = { };
    colourref.attachment = 0;

    struct ORenderer::rtattachmentrefdesc depthref = { };
    depthref.attachment = 1;

    struct ORenderer::rtattachmentdesc attachments[2] = { rtattachment, depthattachment };

    ASSERT(ORenderer::context->createrenderpass(&rpass, 2, attachments, 1, &colourref, &depthref) == ORenderer::RESULT_SUCCESS, "Failed to create renderpass.\n");

    // struct ORenderer::pipelinestateresourcedesc resource = { };
    // resource.binding = 0;
    // resource.stages = ORenderer::STAGE_VERTEX | ORenderer::STAGE_FRAGMENT;
    // resource.type = ORenderer::RESOURCE_UNIFORM;
    //
    // struct ORenderer::pipelinestateresourcedesc samplerresource = { };
    // samplerresource.binding = 1;
    // samplerresource.stages = ORenderer::STAGE_FRAGMENT;
    // samplerresource.type = ORenderer::RESOURCE_SAMPLER;
    //
    // struct ORenderer::pipelinestateresourcedesc texresource = { };
    // samplerresource.binding = 1;
    // samplerresource.stages = ORenderer::STAGE_FRAGMENT;
    // samplerresource.type = ORenderer::RESOURCE_SAMPLER;
    //
    // struct ORenderer::pipelinestateresourcedesc samplerresource2 = { };
    // samplerresource2.binding = 3;
    // samplerresource2.stages = ORenderer::STAGE_FRAGMENT;
    // samplerresource2.type = ORenderer::RESOURCE_SAMPLER;
    //
    // struct ORenderer::pipelinestateresourcedesc samplerresource3 = { };
    // samplerresource3.binding = 5;
    // samplerresource3.stages = ORenderer::STAGE_FRAGMENT;
    // samplerresource3.type = ORenderer::RESOURCE_SAMPLER;

    struct ORenderer::pipelinestateresourcedesc resources[] = {
        (struct ORenderer::pipelinestateresourcedesc) {
            .binding = 0,
            .stages = ORenderer::STAGE_ALL,
            .type = ORenderer::RESOURCE_UNIFORM
        },
        (struct ORenderer::pipelinestateresourcedesc) {
            .binding = 1,
            .stages = ORenderer::STAGE_ALL,
            .type = ORenderer::RESOURCE_SAMPLER
        },
        (struct ORenderer::pipelinestateresourcedesc) {
            .binding = 2,
            .stages = ORenderer::STAGE_ALL,
            .type = ORenderer::RESOURCE_TEXTURE
        },
        (struct ORenderer::pipelinestateresourcedesc) {
            .binding = 3,
            .stages = ORenderer::STAGE_ALL,
            .type = ORenderer::RESOURCE_SAMPLER
        },
        (struct ORenderer::pipelinestateresourcedesc) {
            .binding = 4,
            .stages = ORenderer::STAGE_ALL,
            .type = ORenderer::RESOURCE_TEXTURE
        },
        (struct ORenderer::pipelinestateresourcedesc) {
            .binding = 5,
            .stages = ORenderer::STAGE_ALL,
            .type = ORenderer::RESOURCE_SAMPLER
        },
        (struct ORenderer::pipelinestateresourcedesc) {
            .binding = 6,
            .stages = ORenderer::STAGE_ALL,
            .type = ORenderer::RESOURCE_TEXTURE
        }
    };

    struct ORenderer::depthstencilstatedesc depthstencildesc = { };
    depthstencildesc.stencil = false;
    depthstencildesc.depthtest = true;
    depthstencildesc.maxdepth = 1.0f;
    depthstencildesc.mindepth = 0.0f;
    depthstencildesc.depthwrite = true;
    depthstencildesc.depthboundstest = false;
    depthstencildesc.depthcompareop = ORenderer::CMPOP_LESS;

    // struct ORenderer::pipelinestateresourcedesc resources[4];
    // resources[0] = resource;
    // resources[1] = samplerresource;
    // resources[2] = samplerresource2;
    // resources[3] = samplerresource3;

    struct ORenderer::pipelinestatedesc desc = { };
    desc.stagecount = 2;
    desc.stages = stages;
    desc.tesspoints = 4;
    desc.scissorcount = 1;
    desc.viewportcount = 1;
    desc.primtopology = ORenderer::PRIMTOPOLOGY_TRILIST;
    desc.renderpass = &rpass;
    desc.blendstate = &blendstate;
    desc.rasteriser = &rasteriser;
    desc.vtxinput = &vtxinput;
    desc.depthstencil = &depthstencildesc;
    desc.multisample = &multisample;
    desc.constantssize = sizeof(struct ubo);
    desc.reslayout = &ORenderer::setmanager.layout;
    // desc.resourcecount = sizeof(resources) / sizeof(resources[0]);

    ASSERT(ORenderer::context->createpipelinestate(&desc, &state) == ORenderer::RESULT_SUCCESS, "Failed to create pipeline state.\n");

    ASSERT(ORenderer::context->createtexture(
        &depthtex, ORenderer::IMAGETYPE_2D, 1280, 720, 1, 1, 1,
        ORenderer::FORMAT_D32F, ORenderer::MEMLAYOUT_OPTIMAL,
        ORenderer::USAGE_DEPTHSTENCIL, ORenderer::SAMPLE_X1
    ) == ORenderer::RESULT_SUCCESS, "Failed to create depth texture.\n");

    ASSERT(ORenderer::context->createtextureview(
        &depthtexview, ORenderer::FORMAT_D32F, depthtex,
        ORenderer::IMAGETYPE_2D, ORenderer::ASPECT_DEPTH,
        0, 1, 0, 1
    ) == ORenderer::RESULT_SUCCESS, "Failed to create depth texture view.\n");

    ASSERT(ORenderer::context->createbuffer(
        &ubo, sizeof(struct ubo), ORenderer::BUFFER_UNIFORM,
        ORenderer::MEMPROP_CPUVISIBLE | ORenderer::MEMPROP_CPUCOHERENT | ORenderer::MEMPROP_CPUSEQUENTIALWRITE,
        ORenderer::BUFFERFLAG_PERFRAME
    ) == ORenderer::RESULT_SUCCESS, "Failed to create buffer.\n");

    ASSERT(ORenderer::context->mapbuffer(&ubomap, ubo, 0, sizeof(struct ubo)) == ORenderer::RESULT_SUCCESS, "Failed to map uniform buffer.\n");

    ORenderer::context->createbackbuffer(rpass, &depthtexview);
    renderrect.width = engine_engine->winsize.x;
    renderrect.height = engine_engine->winsize.y;

    ASSERT(ORenderer::context->createsampler(&outsampler, ORenderer::FILTER_NEAREST, ORenderer::FILTER_NEAREST, ORenderer::ADDR_CLAMPEDGE, 0.0f, false, 1.0f) == ORenderer::RESULT_SUCCESS, "Failed to create sampler.\n");

    stages[0].destroy();
    stages[1].destroy();

    samplerid = ORenderer::setmanager.registersampler(outsampler);
    imr.init();
}

void PBRPipeline::resize(struct ORenderer::rect rendersize) {
    renderrect = rendersize;
}

void PBRPipeline::update(uint64_t flags) {

}


#include <engine/scene/scene.hpp>

OScene::Scene scene;

void PBRPipeline::execute(ORenderer::Stream *stream, void *cam) {
    ZoneScopedN("Pipeline Execute");
    int64_t now = utils_getcounter();
    if (!lastframetimestamp) {
        lastframetimestamp = now;
    }
    float delta = (now - lastframetimestamp) / 1000000.0f;
    lastframetimestamp = now;

    struct ORenderer::clearcolourdesc colourdesc = { };
    colourdesc.count = 2;
    colourdesc.clear[0].isdepth = false;
    colourdesc.clear[0].colour = glm::vec4(0.0, 0.0, 0.0, 1.0);
    colourdesc.clear[1].isdepth = true;
    colourdesc.clear[1].depth = 1.0f;
    colourdesc.clear[1].stencil = 0;

    struct ubo data = { };

    struct ORenderer::framebuffer fb = { };
    ORenderer::context->requestbackbuffer(&fb);

    stream->claim();
    stream->begin();
    size_t zone = stream->zonebegin("Object Render");
    stream->beginrenderpass(rpass, fb, (struct ORenderer::rect) { .x = 0, .y = 0, .width = renderrect.width, .height = renderrect.height }, colourdesc);
    struct ORenderer::viewport viewport = { .x = 0, .y = 0, .width = (float)renderrect.width, .height = (float)renderrect.height, .mindepth = 0.0f, .maxdepth = 1.0f };


    stream->setpipelinestate(state); // Only ever set the state when we need to. Ideally an UBER shader should be used as we can afford the branching costs in exchange for not having to worry about materials.
    stream->bindset(ORenderer::setmanager.set);

    stream->setviewport(viewport);
    stream->setscissor((struct ORenderer::rect) { .x = 0, .y = 0, .width = renderrect.width, .height = renderrect.height });

    glm::mat4 viewmtx = glm::lookAt(glm::vec3(2.0f, 2.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 _0; // Scale
    glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 _1; // Position
    glm::vec3 _2; // Skew
    glm::vec4 _3; // Perspective

    glm::decompose(viewmtx, _0, orientation, _1, _2, _3);

    // ORenderer::PerspectiveCamera camera = ORenderer::PerspectiveCamera(glm::vec3(2.0f, 2.0f, 5.0f), glm::rotate(orientation, glm::radians(10.0f) * (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f)), 45.0f, renderrect.width / (float)renderrect.height, 0.1f, 1000.0f);
    // ORenderer::PerspectiveCamera camera = ORenderer::PerspectiveCamera(glm::vec3(2.0f, 2.0f, 5.0f), glm::rotate(orientation, glm::radians(-10.0f), glm::vec3(0.0f, 1.0f, 0.0f)), 45.0f, renderrect.width / (float)renderrect.height, 0.1f, 1000.0f);
    ORenderer::PerspectiveCamera *camera = (ORenderer::PerspectiveCamera *)cam;

    viewmtx = glm::lookAt(glm::vec3(2.0f, 0.0f, -10.0f), glm::vec3(0.0f, 0.0f, -20.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    glm::decompose(viewmtx, _0, orientation, _1, _2, _3);

    ORenderer::PerspectiveCamera tcamera = ORenderer::PerspectiveCamera(glm::vec3(2.0f, 0.0f, -10.0f), glm::rotate(orientation, glm::radians(10.0f) * (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f)), 45.0f, renderrect.width / (float)renderrect.height, 0.1f, 1000.0f);

    OScene::CullResult *res = scene.partitionmanager.cull(*camera);
    TracyMessageL("Done Culling");
    OScene::CullResult *reshead = res;
    size_t totalobjects = scene.objects.size();
    size_t visibleobjects = 0;
    ORenderer::ScratchBuffer *scratchbuffer;
    ASSERT(ORenderer::context->requestscratchbuffer(&scratchbuffer) == ORenderer::RESULT_SUCCESS, "Failed to request scratchbuffer.\n");
    if (res != NULL) {
        printf("rendering culled objects.\n");
        ZoneScopedN("Object Render");

        while (res != NULL) {
            for (size_t i = 0; i < res->header.count; i++) {
                OUtils::Handle<OScene::GameObject> obj = res->objects[i];

                if ((!(obj->flags & OScene::GameObject::IS_INVISIBLE)) && obj->hascomponent(OUtils::STRINGID("ModelInstance"))) {
                    ZoneScopedN("Individual");
                    visibleobjects++;
                    OUtils::Handle<OScene::ModelInstance> model = obj->getcomponent(OUtils::STRINGID("ModelInstance"))->gethandle<OScene::ModelInstance>();

                    model->model->claim(); // XXX: Claim access.

                    const float time = glfwGetTime();

                    for (size_t i = 0; i < model->model->as<ORenderer::Model>()->meshes.size(); i++) {
                        data.sampler = samplerid;
                        // XXX: Compress into material ID system.
                        data.base = model->model->as<ORenderer::Model>()->meshes[i].material.base.gpuid;
                        data.normal = model->model->as<ORenderer::Model>()->meshes[i].material.normal.gpuid;
                        data.mrid = model->model->as<ORenderer::Model>()->meshes[i].material.mr.gpuid;
                        data.model = obj->getglobalmatrix();
                        data.viewproj = camera->getviewproj(); // precalculated combination (so we don't have to do this calculation per fragment, this could be extended still by precalculating the mvp)
                        data.campos = camera->pos;

                        {
                            ZoneScopedN("Push Constants");
                            stream->pushconstants(state, &data, sizeof(struct ubo));
                        }

                        stream->setvtxbuffer(model->model->as<ORenderer::Model>()->meshes[i].vertexbuffer, 0);
                        stream->setidxbuffer(model->model->as<ORenderer::Model>()->meshes[i].indexbuffer, 0, false);
                        stream->drawindexed(model->model->as<ORenderer::Model>()->meshes[i].indices.size(), 1, 0, 0, 0);
                        }
                        model->model->release(); // XXX: Relinquish our claim on resource access.
                        TracyMessageL("finish single.");
                }
            }
            res = res->header.next;
        }

        // Release our result pages back to the allocator.
        scene.partitionmanager.freeresults(reshead);
    }

    // zone = stream->zonebegin("Im3D");
    // Im3d::AppData &ad = Im3d::GetAppData();
    // ad.m_deltaTime = delta;
    // ad.m_viewportSize = Im3d::Vec2(renderrect.width, renderrect.height);
    // ad.m_viewOrigin = Im3d::Vec3(camera.pos.x, camera.pos.y, camera.pos.z);
    // ad.m_viewDirection = Im3d::Vec3(camera.getforward().x, camera.getforward().y, camera.getforward().z);
    // ad.m_worldUp = Im3d::Vec3(0.0f, 1.0f, 0.0f);
    // ad.m_projOrtho = false;
    //
    // ad.m_projScaleY = tanf(glm::radians(camera.fov) * 0.5f) * 2.0f; // Classic projection.
    //
    // glm::mat4 viewproj = camera.getviewproj();
    // ad.setCullFrustum(Im3d::Mat4(
    //     viewproj[0][0], viewproj[0][1], viewproj[0][2], viewproj[0][3],
    //     viewproj[1][0], viewproj[1][1], viewproj[1][2], viewproj[1][3],
    //     viewproj[2][0], viewproj[2][1], viewproj[2][2], viewproj[2][3],
    //     viewproj[3][0], viewproj[3][1], viewproj[3][2], viewproj[3][3]
    // ), true);
    // Im3d::NewFrame();
    //
    // Im3d::DrawPoint(Im3d::Vec3(0.0f, 0.0f, 0.0f), 4, Im3d::Color_Red);
    // Im3d::DrawPoint(Im3d::Vec3(0.0f, 0.1f, 0.0f), 4, Im3d::Color_Red);
    // Im3d::DrawPoint(Im3d::Vec3(0.0f, 0.2f, 0.0f), 4, Im3d::Color_Red);
    //
    // // Do cool stuff here
    //
    // Im3d::EndFrame();
    //
    // struct im3dubo im3ddata = { };
    // im3ddata.viewproj = camera.getviewproj();
    // im3ddata.viewport = glm::vec2(renderrect.width, renderrect.height);
    // for (size_t i = 0; i < Im3d::GetDrawListCount(); i++) {
    //     const Im3d::DrawList &drawlist = Im3d::GetDrawLists()[i];
    //
    //     ASSERT(drawlist.m_primType != Im3d::DrawPrimitive_Triangles, "Unsupported primitive from Im3D.\n");
    //     ASSERT(drawlist.m_vertexCount < (64 * 1024) / 32, "Too many vertices.\n");
    //
    //     stream->setpipelinestate(im3dptstate);
    //     stream->setviewport(viewport);
    //     stream->setscissor((struct ORenderer::rect) { .x = 0, .y = 0, .width = renderrect.width, .height = renderrect.height });
    //
    //     ORenderer::ScratchBuffer *scratchbuffer;
    //     ORenderer::context->requestscratchbuffer(&scratchbuffer);
    //     size_t offset = scratchbuffer->write(&im3ddata, sizeof(struct im3dubo));
    //     struct ORenderer::bufferbind ubobind = { .buffer = scratchbuffer->buffer, .offset = offset, .range = sizeof(struct im3dubo) };
    //     stream->bindresource(0, ubobind, ORenderer::RESOURCE_UNIFORM);
    //     size_t dataoffset = scratchbuffer->write(drawlist.m_vertexData, sizeof(struct Im3d::VertexData) * drawlist.m_vertexCount);
    //     ubobind.offset = dataoffset;
    //     ubobind.range = 64 * 1024;
    //     stream->bindresource(1, ubobind, ORenderer::RESOURCE_UNIFORM);
    //     stream->commitresources();
    //     size_t primvtx = drawlist.m_primType == Im3d::DrawPrimitive_Points ? 1 : 2;
    //
    //     stream->setvtxbuffer(im3dvtxbuffer, 0);
    //     stream->draw(primvtx, drawlist.m_vertexCount / primvtx, 0, 0);
    // }


    stream->zoneend(zone);
    stream->endrenderpass();

    // printf("canvas.\n");

    {
        ZoneScopedN("Canvas Draw");
        // canvas.updateuniform(camera->getviewproj(), glfwGetTime());
        // canvas.clear();
        // canvas.line(glm::vec3(tcamera.pos), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        // // size_t totalobjects = scene.objects.size();
        // // size_t visibleobjects = 0;
        // for (auto it = scene.partitionmanager.map.begin(); it != scene.partitionmanager.map.end(); it++) {
        //     OScene::Cell *cell = it->second;
        //     canvas.drawbox(glm::mat4(1.0f), OMath::AABB(cell->header.origin, cell->header.origin + 150.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        // }
        // // canvas.drawfrustum(camera.getview(), camera.getproj(), glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
        // for (auto it = scene.objects.begin(); it != scene.objects.end(); it++) {
        //     if (!((*it)->flags & OScene::GameObject::IS_CULLABLE)) {
        //         continue;
        //     }
        //
        //     OUtils::Handle<OScene::GameObject> obj = (*it)->gethandle<OScene::GameObject>();
        //
        //
        //     canvas.drawbox(obj->getglobalmatrix(), obj->bounds, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        // }
    }
    // res = scene.partitionmanager.cull(tcamera);
    // if (res != NULL) {
    //
    //     // for (auto it = scene.objects.begin(); it != scene.objects.end(); it++) {
    //     while (res != NULL) {
    //         for (size_t i = 0; i < res->header.count; i++) {
    //             OUtils::Handle<OScene::CullableObject> obj = res->objects[i];
    //
    //             if ((!(obj->flags & OScene::GameObject::IS_INVISIBLE)) && obj->hascomponent(OUtils::STRINGID("ModelInstance"))) {
    //                 visibleobjects++;
    //                 canvas.drawbox(obj->getglobalmatrix(), obj->bounds, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    //             }
    //         }
    //         res = res->header.next;
    //     }
    //
    //     // Release out result pages back to the allocator.
        // scene.partitionmanager.freeresults(res);
    // }

    // canvas.updatebuffer();

    if (canvas.vertices.size()) {
        // canvas.fillstream(&imstream);
    }
    // } else { // If nothing happens here so we kind of just force it to the next step (backbuffer).
    //     struct ORenderer::texture texture;
    //     struct ORenderer::backbufferinfo backbufferinfo;
    //     ORenderer::context->requestbackbufferinfo(&backbufferinfo);
    //     ORenderer::context->requestbackbuffertexture(&texture);
    //     stream->transitionlayout(texture, backbufferinfo.format, ORenderer::LAYOUT_BACKBUFFER);
    // }

    {
        // printf("imgui.\n");
        ZoneScopedN("ImGui Steps");
        ImGuiIO &io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)renderrect.width, (float)renderrect.height);

        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(220.0f, 90.0f), 0);
        ImGui::Begin("Stats");
        ImGui::Text("Visible Objects: %lu/%lu", visibleobjects, totalobjects);
        ImGui::End();

        ImGui::Render();

        zone = stream->zonebegin("imgui");
        imr.updatebuffer();
        // ORenderer::Stream *imstream = ORenderer::context->getsecondary();
        imr.fillstream(stream);
        // imr.updatebuffer();
        // imr.fillstream(&imstream);
        // stream->submitstream(imstream);
        stream->zoneend(zone);
        // printf("done imgui.\n");
    }

    // struct ORenderer::texture texture;
    // struct ORenderer::backbufferinfo backbufferinfo;
    // ORenderer::context->requestbackbufferinfo(&backbufferinfo);
    // ORenderer::context->requestbackbuffertexture(&texture);
    // stream->transitionlayout(texture, backbufferinfo.format, ORenderer::LAYOUT_BACKBUFFER);

    stream->end();
    stream->release();
}
