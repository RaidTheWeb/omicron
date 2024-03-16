#include <engine/engine.hpp>

#include <engine/renderer/pipeline/pbrpipeline.hpp>
#include <engine/renderer/renderer.hpp>
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
ORenderer::Model model;

struct ubo {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct vertex {
    glm::vec3 pos;
    glm::vec3 colour;
    glm::vec2 texcoord;
};

static struct vertex vertices[] = {
    { { -0.5f, 0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
    { { 0.5f, 0.0f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
    { { 0.5f, 0.0f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
    { { -0.5f, 0.0f, 0.5f }, { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },

    { { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
    { { 0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
    { { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
    { { -0.5f, -0.5f, 0.5f }, { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },

    { { -0.5f, -1.0f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
    { { 0.5f, -1.0f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
    { { 0.5f, -1.0f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
    { { -0.5f, -1.0f, 0.5f }, { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
};

static uint16_t indices[] = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4,
    8, 9, 10, 10, 11, 8
};

struct ORenderer::rect renderrect = { 0, 0, 1280, 720 };


#include <engine/renderer/backend/vulkan.hpp>

void PBRPipeline::init(void) {
    ORenderer::Shader stages[2] = { ORenderer::Shader("shaders/test.vert.spv", ORenderer::SHADER_VERTEX), ORenderer::Shader("shaders/test.frag.spv", ORenderer::SHADER_FRAGMENT) };

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
    rasteriser.cullmode = ORenderer::CULL_NONE;
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
    rtattachment.finallayout = ORenderer::LAYOUT_BACKBUFFER;

    struct ORenderer::rtattachmentdesc depthattachment = { };
    depthattachment.format = ORenderer::FORMAT_D24S8U;
    depthattachment.samples = ORenderer::SAMPLE_X1;
    depthattachment.loadop = ORenderer::LOADOP_CLEAR;
    depthattachment.storeop = ORenderer::STOREOP_STORE;
    depthattachment.stencilloadop = ORenderer::LOADOP_DONTCARE;
    depthattachment.stencilstoreop = ORenderer::STOREOP_DONTCARE;
    depthattachment.initiallayout = ORenderer::LAYOUT_UNDEFINED;
    depthattachment.finallayout = ORenderer::LAYOUT_DEPTHSTENCILATTACHMENT;

    struct ORenderer::rtattachmentrefdesc colourref = { };
    colourref.attachment = 0;

    struct ORenderer::rtattachmentrefdesc depthref = { };
    depthref.attachment = 1;

    struct ORenderer::renderpassdesc rpasscreate = { };
    rpasscreate.attachmentcount = 1;
    rpasscreate.attachments = &rtattachment;
    rpasscreate.colourrefcount = 1;
    rpasscreate.colourrefs = &colourref;
    rpasscreate.depthref = NULL;
    // rpasscreate.depthref = &depthref;

    ASSERT(ORenderer::context->createrenderpass(&rpasscreate, &rpass) == ORenderer::RESULT_SUCCESS, "Failed to create render pass.\n");

    struct ORenderer::pipelinestateresourcedesc resource = { };
    resource.binding = 0;
    resource.stages = ORenderer::STAGE_VERTEX;
    resource.type = ORenderer::RESOURCE_UNIFORM;

    struct ORenderer::pipelinestateresourcedesc samplerresource = { };
    samplerresource.binding = 1;
    samplerresource.stages = ORenderer::STAGE_FRAGMENT;
    samplerresource.type = ORenderer::RESOURCE_SAMPLER;

    struct ORenderer::depthstencilstatedesc depthstencildesc = { };
    depthstencildesc.stencil = false;
    depthstencildesc.depthtest = true;
    depthstencildesc.maxdepth = 1.0f;
    depthstencildesc.mindepth = 0.0f;
    depthstencildesc.depthwrite = true;
    depthstencildesc.depthboundstest = false;
    depthstencildesc.depthcompareop = ORenderer::CMPOP_LESS;

    struct ORenderer::pipelinestateresourcedesc resources[2];
    resources[0] = resource;
    resources[1] = samplerresource;

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
    // desc.depthstencil = &depthstencildesc;
    desc.depthstencil = NULL;
    desc.multisample = &multisample;
    desc.resources = resources;
    desc.resourcecount = 2;

    ASSERT(ORenderer::context->createpipelinestate(&desc, &state) == ORenderer::RESULT_SUCCESS, "Failed to create pipeline state.\n");

    struct ORenderer::texturedesc texturedesc = { };
    texturedesc.width = 1280;
    texturedesc.height = 720;
    texturedesc.layers = 1;
    texturedesc.type = ORenderer::IMAGETYPE_2D;
    texturedesc.mips = 1;
    texturedesc.depth = 1;
    texturedesc.samples = ORenderer::SAMPLE_X1;
    texturedesc.format = ORenderer::FORMAT_BGRA8;
    texturedesc.memlayout = ORenderer::MEMLAYOUT_OPTIMAL;
    texturedesc.usage = ORenderer::USAGE_COLOUR; // XXX: I forgot about this, fix that soon.
    // ASSERT(renderer_context->createtexture(&texturedesc, &tex) == RENDERER_RESULTSUCCESS, "Failed to create framebuffer texture.\n");

    struct ORenderer::textureviewdesc textureviewdesc = { };
    textureviewdesc.format = ORenderer::FORMAT_BGRA8;
    textureviewdesc.texture = tex;
    textureviewdesc.type = ORenderer::IMAGETYPE_2D;
    textureviewdesc.aspect = ORenderer::ASPECT_COLOUR;
    textureviewdesc.mipcount = 1;
    textureviewdesc.baselayer = 0;
    textureviewdesc.layercount = 1;
    // ASSERT(renderer_context->createtextureview(&textureviewdesc, &texview) == RENDERER_RESULTSUCCESS, "Failed to create framebuffer texture view.\n");

    struct ORenderer::texturedesc depthtexdesc = { };
    depthtexdesc.width = 1280;
    depthtexdesc.height = 720;
    depthtexdesc.layers = 1;
    depthtexdesc.type = ORenderer::IMAGETYPE_2D;
    depthtexdesc.mips = 1;
    depthtexdesc.depth = 1;
    depthtexdesc.samples = ORenderer::SAMPLE_X1;
    depthtexdesc.format = ORenderer::FORMAT_D24S8U;
    depthtexdesc.memlayout = ORenderer::MEMLAYOUT_OPTIMAL;
    depthtexdesc.usage = ORenderer::USAGE_DEPTHSTENCIL;
    // ASSERT(renderer_context->createtexture(&depthtexdesc, &depthtex) == RENDERER_RESULTSUCCESS, "Failed to create depth texture.\n");

    struct ORenderer::textureviewdesc depthviewdesc = { };
    depthviewdesc.format = ORenderer::FORMAT_D24S8U;
    depthviewdesc.texture = depthtex;
    depthviewdesc.type = ORenderer::IMAGETYPE_2D;
    depthviewdesc.aspect = ORenderer::ASPECT_DEPTH;
    depthviewdesc.mipcount = 1;
    depthviewdesc.baselayer = 0;
    depthviewdesc.layercount = 1;
    // ASSERT(renderer_context->createtextureview(&depthviewdesc, &depthtexview) == RENDERER_RESULTSUCCESS, "Failed to create depth texture view.\n");

    // struct renderer_textureview fbattachments[] = { texview, 0 };

    // struct renderer_framebufferdesc fbdesc = { };
    // fbdesc.width = 1280;
    // fbdesc.height = 720;
    // fbdesc.layers = 1;
    // fbdesc.attachmentcount = 1;
    // fbdesc.pass = rpass;
    // fbdesc.attachments = fbattachments;
    // ASSERT(renderer_context->createframebuffer(&fbdesc, &fb) == RENDERER_RESULTSUCCESS, "Failed to create framebuffer.\n");

    // struct ORenderer::bufferdesc bufferdesc = { };
    // bufferdesc.size = sizeof(vertices);
    // bufferdesc.usage = ORenderer::BUFFER_VERTEX | ORenderer::BUFFER_TRANSFERDST;
    // bufferdesc.memprops = ORenderer::MEMPROP_GPULOCAL;
    // ASSERT(ORenderer::context->createbuffer(&bufferdesc, &buffer) == ORenderer::RESULT_SUCCESS, "Failed to create buffer.\n");
    //
    // struct ORenderer::bufferdesc stagingdesc = { };
    // stagingdesc.size = sizeof(vertices);
    // stagingdesc.usage = ORenderer::BUFFER_TRANSFERSRC;
    // stagingdesc.memprops = ORenderer::MEMPROP_CPUVISIBLE | ORenderer::MEMPROP_CPUCOHERENT | ORenderer::MEMPROP_CPUSEQUENTIALWRITE;
    // stagingdesc.flags = 0;
    // struct ORenderer::buffer staging = { };
    // ASSERT(ORenderer::context->createbuffer(&stagingdesc, &staging) == ORenderer::RESULT_SUCCESS, "Failed to create staging buffer.\n");
    //
    // struct ORenderer::buffermapdesc stagingmapdesc = { };
    // stagingmapdesc.buffer = staging;
    // stagingmapdesc.size = sizeof(vertices);
    // stagingmapdesc.offset = 0;
    // struct ORenderer::buffermap stagingmap = { };
    // ASSERT(ORenderer::context->mapbuffer(&stagingmapdesc, &stagingmap) == ORenderer::RESULT_SUCCESS, "Failed to map staging buffer.\n");
    // memcpy(stagingmap.mapped[0], vertices, sizeof(vertices));
    // ORenderer::context->unmapbuffer(stagingmap);
    //
    // struct ORenderer::buffercopydesc buffercopy = { };
    // buffercopy.src = staging;
    // buffercopy.dst = buffer;
    // buffercopy.srcoffset = 0;
    // buffercopy.dstoffset = 0;
    // buffercopy.size = sizeof(vertices);
    // ORenderer::context->copybuffer(&buffercopy);
    //
    // ORenderer::context->destroybuffer(&staging); // destroy so we can recreate it
    //
    // bufferdesc.size = sizeof(indices);
    // bufferdesc.usage = ORenderer::BUFFER_INDEX | ORenderer::BUFFER_TRANSFERDST;
    // bufferdesc.memprops = ORenderer::MEMPROP_GPULOCAL;
    // ASSERT(ORenderer::context->createbuffer(&bufferdesc, &idxbuffer) == ORenderer::RESULT_SUCCESS, "Failed to create buffer.\n");
    //
    // stagingdesc.size = sizeof(indices);
    // stagingdesc.usage = ORenderer::BUFFER_TRANSFERSRC;
    // stagingdesc.memprops = ORenderer::MEMPROP_CPUVISIBLE | ORenderer::MEMPROP_CPUCOHERENT | ORenderer::MEMPROP_CPUSEQUENTIALWRITE;
    // stagingdesc.flags = 0;
    // ASSERT(ORenderer::context->createbuffer(&stagingdesc, &staging) == ORenderer::RESULT_SUCCESS, "Failed to create staging buffer.\n");
    //
    // stagingmapdesc.buffer = staging;
    // stagingmapdesc.size = sizeof(indices);
    // stagingmapdesc.offset = 0;
    // ASSERT(ORenderer::context->mapbuffer(&stagingmapdesc, &stagingmap) == ORenderer::RESULT_SUCCESS, "failed to map staging buffer.\n");
    // memcpy(stagingmap.mapped[0], indices, sizeof(indices));
    // ORenderer::context->unmapbuffer(stagingmap);
    //
    // buffercopy.src = staging;
    // buffercopy.dst = idxbuffer;
    // buffercopy.srcoffset = 0;
    // buffercopy.dstoffset = 0;
    // buffercopy.size = sizeof(indices);
    // ORenderer::context->copybuffer(&buffercopy);

    struct ORenderer::bufferdesc ubodesc = { };
    ubodesc.size = sizeof(struct ubo);
    ubodesc.memprops = ORenderer::MEMPROP_CPUVISIBLE | ORenderer::MEMPROP_CPUCOHERENT | ORenderer::MEMPROP_CPUSEQUENTIALWRITE;
    ubodesc.usage = ORenderer::BUFFER_UNIFORM;
    ubodesc.flags = ORenderer::BUFFERFLAG_PERFRAME;
    ASSERT(ORenderer::context->createbuffer(&ubodesc, &ubo) == ORenderer::RESULT_SUCCESS, "Failed to create buffer.\n");

    struct ORenderer::buffermapdesc ubomapdesc = { };
    ubomapdesc.buffer = ubo;
    ubomapdesc.size = sizeof(struct ubo);
    ubomapdesc.offset = 0;
    ASSERT(ORenderer::context->mapbuffer(&ubomapdesc, &ubomap) == ORenderer::RESULT_SUCCESS, "Failed to map uniform buffer.\n");

    ORenderer::context->createbackbuffer(rpass);
    renderrect.width = engine_engine->winsize.x;
    renderrect.height = engine_engine->winsize.y;

    outtex = OResource::Texture::load("misc/out.ktx2");

    struct ORenderer::textureviewdesc viewdesc = { };
    viewdesc.format = ORenderer::FORMAT_RGBA8SRGB;
    viewdesc.texture = outtex;
    viewdesc.type = ORenderer::IMAGETYPE_2D;
    viewdesc.aspect = ORenderer::ASPECT_COLOUR;
    viewdesc.mipcount = 1;
    viewdesc.baselayer = 0;
    viewdesc.layercount = 1;

    ASSERT(ORenderer::context->createtextureview(&viewdesc, &outtexview) == ORenderer::RESULT_SUCCESS, "Failed to create view of texture.\n");

    struct ORenderer::samplerdesc samplerdesc = { };
    samplerdesc.unnormalisedcoords = false;
    samplerdesc.maxlod = 0.0f;
    samplerdesc.minlod = 0.0f;
    samplerdesc.magfilter = ORenderer::FILTER_NEAREST;
    samplerdesc.minfilter = ORenderer::FILTER_NEAREST;
    samplerdesc.mipmode = ORenderer::FILTER_NEAREST;
    samplerdesc.addru = ORenderer::ADDR_CLAMPEDGE;
    samplerdesc.addrv = ORenderer::ADDR_CLAMPEDGE;
    samplerdesc.addrw = ORenderer::ADDR_CLAMPEDGE;
    samplerdesc.lodbias = 0.0f;
    samplerdesc.anisotropyenable = false;
    samplerdesc.maxanisotropy = 1.0f;
    samplerdesc.cmpenable = false;
    samplerdesc.cmpop = ORenderer::CMPOP_NEVER;
    ASSERT(ORenderer::context->createsampler(&samplerdesc, &outsampler) == ORenderer::RESULT_SUCCESS, "Failed to create sampler.\n");

    stages[0].destroy();
    stages[1].destroy();
}

void PBRPipeline::resize(struct ORenderer::rect rendersize) {
    renderrect = rendersize;
}

void PBRPipeline::update(uint64_t flags) {

}


#include <engine/scene/scene.hpp>

OScene::Scene scene;

void PBRPipeline::execute(ORenderer::Stream *stream) {
    struct ORenderer::clearcolourdesc colourdesc = { };
    colourdesc.count = 1;
    colourdesc.clear[0].isdepth = false;
    colourdesc.clear[0].colour = glm::vec4(0.0, 0.0, 0.0, 1.0);
    colourdesc.clear[1].isdepth = true;
    colourdesc.clear[1].depth = 1.0f;
    colourdesc.clear[1].stencil = 0;

    struct ubo data = { }; 

    struct ORenderer::framebuffer fb = { };
    ORenderer::context->requestbackbuffer(&fb);

    stream->claim();
    stream->beginrenderpass(rpass, fb, (struct ORenderer::rect) { .x = 0, .y = 0, .width = renderrect.width, .height = renderrect.height }, colourdesc);
    struct ORenderer::viewport viewport = { .x = 0, .y = 0, .width = (float)renderrect.width, .height = (float)renderrect.height, .mindepth = 0.0f, .maxdepth = 1.0f };


    stream->setpipelinestate(state); 
    stream->setviewport(viewport);
    stream->setscissor((struct ORenderer::rect) { .x = 0, .y = 0, .width = renderrect.width, .height = renderrect.height });
    for (auto it = scene.objects.begin(); it != scene.objects.end(); it++) {
        OUtils::Handle<OScene::GameObject> obj = *it;

        if (obj->flags & OScene::GameObject::IS_MODEL) {
            OUtils::Handle<OScene::ModelInstance> model = OUtils::Handle<OScene::ModelInstance>(obj);

            float time = glfwGetTime();
            int64_t start = utils_getcounter();
            data.model = glm::rotate(model->getglobalmatrix(), time * glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // rotate Z
            int64_t end = utils_getcounter();
            data.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            data.proj = glm::perspective(glm::radians(45.0f), renderrect.width / (float)renderrect.height, 0.1f, 10.0f);
            ORenderer::context->adjustprojection(&data.proj); // adjust for the difference between our renderer backend and GLM

            OVulkan::VulkanContext *ctx = (OVulkan::VulkanContext *)ORenderer::context;
            size_t offset = ctx->scratchbuffers[ctx->frame].write(&data, sizeof(struct ubo));

            struct ORenderer::bufferbind ubobind = { .buffer = ctx->scratchbuffers[ctx->frame].buffer, .offset = offset, .range = sizeof(struct ubo) };
            stream->bindresource(0, ubobind, ORenderer::RESOURCE_UNIFORM);
            struct ORenderer::sampledbind bind = { .sampler = outsampler, .view = model->model.meshes[0].material.base.view, .layout = ORenderer::LAYOUT_SHADERRO };
            stream->bindresource(1, bind, ORenderer::RESOURCE_SAMPLER);
            stream->commitresources();
 
            size_t offsets = 0;
            stream->setvtxbuffers(&model->model.meshes[0].vertexbuffer, &offsets, 0, 1);
            stream->setidxbuffer(model->model.meshes[0].indexbuffer, 0, false); 
            stream->drawindexed(model->model.meshes[0].indices.size(), 1, 0, 0, 0);
        }
    }
    stream->endrenderpass();
    stream->release();
}
