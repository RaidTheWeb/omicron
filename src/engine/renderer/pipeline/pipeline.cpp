#include <engine/engine.hpp>

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

struct ORenderer::Shader im3dpt[2] = { };
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

void setupim(void) {
    canvas.init();

    // Upload raw quad to GPU for later manipulation.
    struct ORenderer::bufferdesc vtxdesc = {};
    vtxdesc.size = sizeof(im3dvtx);
    vtxdesc.usage = ORenderer::BUFFER_VERTEX | ORenderer::BUFFER_TRANSFERDST;
    vtxdesc.memprops = ORenderer::MEMPROP_GPULOCAL;
    ASSERT(ORenderer::context->createbuffer(&vtxdesc, &im3dvtxbuffer) == ORenderer::RESULT_SUCCESS, "Failed to create buffer.\n");

    ORenderer::uploadtobuffer(im3dvtxbuffer, sizeof(im3dvtx), im3dvtx, 0);

    im3dlayout.stride = sizeof(glm::vec4);
    im3dlayout.binding = 0;
    im3dlayout.attribcount = 1;
    im3dlayout.attribs[0].offset = 0;
    im3dlayout.attribs[0].attribtype = ORenderer::VTXATTRIB_VEC4;

    // Create and map the uniform buffer
    struct ORenderer::bufferdesc ubodesc = { };
    ubodesc.size = sizeof(struct im3dubo);
    ubodesc.memprops = ORenderer::MEMPROP_CPUVISIBLE | ORenderer::MEMPROP_CPUCOHERENT | ORenderer::MEMPROP_CPUSEQUENTIALWRITE;
    ubodesc.usage = ORenderer::BUFFER_UNIFORM;
    ubodesc.flags = ORenderer::BUFFERFLAG_PERFRAME;
    ASSERT(ORenderer::context->createbuffer(&ubodesc, &im3dubo) == ORenderer::RESULT_SUCCESS, "Failed to create buffer.\n");

    struct ORenderer::buffermapdesc ubomapdesc = { };
    ubomapdesc.size = sizeof(struct im3dubo);
    ubomapdesc.buffer = im3dubo;
    ubomapdesc.offset = 0;
    ASSERT(ORenderer::context->mapbuffer(&ubomapdesc, &im3dubomap) == ORenderer::RESULT_SUCCESS, "Failed to map uniform buffer.\n");

    // Do it again but for the vertex data block (inputs from im3d).
    ubodesc.size = 64 * 1024; // minimum the vulkan spec claims to offer for a single buffer.
    ubodesc.memprops = ORenderer::MEMPROP_CPUVISIBLE | ORenderer::MEMPROP_CPUCOHERENT | ORenderer::MEMPROP_CPUSEQUENTIALWRITE;
    ubodesc.usage = ORenderer::BUFFER_UNIFORM;
    ubodesc.flags = ORenderer::BUFFERFLAG_PERFRAME; // Unfortunately so, I'd rather it wasn't the case but alas.
    ASSERT(ORenderer::context->createbuffer(&ubodesc, &im3dvtxdata) == ORenderer::RESULT_SUCCESS, "Failed to create buffer.\n");

    ubomapdesc.size = 64 * 1024;
    ubomapdesc.buffer = im3dvtxdata;
    ubomapdesc.offset = 0;
    ASSERT(ORenderer::context->mapbuffer(&ubomapdesc, &im3dvtxdatamap) == ORenderer::RESULT_SUCCESS, "Failed to map uniform buffer.\n");

    struct ORenderer::blendattachmentdesc attachment = { };
    attachment.writemask = ORenderer::WRITE_RGBA;
    attachment.blend = true;
    attachment.colourblendop = ORenderer::BLENDOP_ADD;
    attachment.alphablendop = ORenderer::BLENDOP_ADD;
    attachment.srccolourblendfactor = ORenderer::BLENDFACTOR_SRCALPHA; // blend colour based on alpha
    attachment.dstalphablendfactor = ORenderer::BLENDFACTOR_ONEMINUSSRCALPHA; // remove alpha value from src
    attachment.dstalphablendfactor = ORenderer::BLENDFACTOR_ZERO; // contribute nothing
    attachment.srcalphablendfactor = ORenderer::BLENDFACTOR_ONE; // contribute all
    
    struct ORenderer::blendstatedesc blendstate = { };
    blendstate.logicopenable = false;
    blendstate.logicop = ORenderer::LOGICOP_COPY;
    blendstate.attachmentcount = 1;
    blendstate.attachments = &attachment;

    struct ORenderer::vtxbinddesc bind = { };
    bind.rate = ORenderer::RATE_VERTEX;
    bind.layout = im3dlayout;
    
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

    struct ORenderer::pipelinestateresourcedesc resource = { };
    resource.binding = 0;
    resource.stages = ORenderer::STAGE_VERTEX;
    resource.type = ORenderer::RESOURCE_UNIFORM;

    struct ORenderer::pipelinestateresourcedesc dataresource = { };
    dataresource.binding = 1;
    dataresource.stages = ORenderer::STAGE_VERTEX;
    dataresource.type = ORenderer::RESOURCE_UNIFORM;

    struct ORenderer::depthstencilstatedesc depthstencildesc = { };
    depthstencildesc.stencil = false;
    depthstencildesc.depthtest = false;
    depthstencildesc.maxdepth = 1.0f;
    depthstencildesc.mindepth = 0.0f;
    depthstencildesc.depthwrite = false;
    depthstencildesc.depthboundstest = false;
    depthstencildesc.depthcompareop = ORenderer::CMPOP_LESS;

    struct ORenderer::pipelinestateresourcedesc resources[2];
    resources[0] = resource;
    resources[1] = dataresource;

    struct ORenderer::pipelinestatedesc desc = { };
    desc.stagecount = 2;
    desc.stages = im3dpt;
    desc.tesspoints = 4;
    desc.scissorcount = 1;
    desc.viewportcount = 1;
    desc.primtopology = ORenderer::PRIMTOPOLOGY_TRISTRIP;
    desc.renderpass = &rpass;
    desc.blendstate = &blendstate;
    desc.rasteriser = &rasteriser;
    desc.vtxinput = &vtxinput;
    // desc.depthstencil = NULL;
    desc.depthstencil = &depthstencildesc;
    desc.multisample = &multisample;
    desc.resources = resources;
    desc.resourcecount = 2;
    ASSERT(ORenderer::context->createpipelinestate(&desc, &im3dptstate) == ORenderer::RESULT_SUCCESS, "Failed to create pipeline state.\n");
}

#include <engine/renderer/backend/vulkan.hpp>

ORenderer::ImGuiRenderer imr;

void PBRPipeline::init(void) {
    ORenderer::Shader stages[2] = { ORenderer::Shader("shaders/test.vert.spv", ORenderer::SHADER_VERTEX), ORenderer::Shader("shaders/test.frag.spv", ORenderer::SHADER_FRAGMENT) };
    im3dpt[0] = ORenderer::Shader("shaders/im3dpt.vert.spv", ORenderer::SHADER_VERTEX);
    im3dpt[1] = ORenderer::Shader("shaders/im3dpt.frag.spv", ORenderer::SHADER_FRAGMENT);

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

    struct ORenderer::renderpassdesc rpasscreate = { };
    rpasscreate.attachmentcount = 2;
    rpasscreate.attachments = attachments;
    rpasscreate.colourrefcount = 1;
    rpasscreate.colourrefs = &colourref;
    rpasscreate.depthref = &depthref;

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
    desc.depthstencil = &depthstencildesc;
    // desc.depthstencil = NULL;
    desc.multisample = &multisample;
    desc.resources = resources;
    desc.resourcecount = 2;

    ASSERT(ORenderer::context->createpipelinestate(&desc, &state) == ORenderer::RESULT_SUCCESS, "Failed to create pipeline state.\n");

    struct ORenderer::texturedesc depthtexdesc = { };
    depthtexdesc.width = 1280;
    depthtexdesc.height = 720;
    depthtexdesc.layers = 1;
    depthtexdesc.type = ORenderer::IMAGETYPE_2D;
    depthtexdesc.mips = 1;
    depthtexdesc.depth = 1;
    depthtexdesc.samples = ORenderer::SAMPLE_X1;
    depthtexdesc.format = ORenderer::FORMAT_D32F;
    depthtexdesc.memlayout = ORenderer::MEMLAYOUT_OPTIMAL;
    depthtexdesc.usage = ORenderer::USAGE_DEPTHSTENCIL;
    ASSERT(ORenderer::context->createtexture(&depthtexdesc, &depthtex) == ORenderer::RESULT_SUCCESS, "Failed to create depth texture.\n");

    struct ORenderer::textureviewdesc depthviewdesc = { };
    depthviewdesc.format = ORenderer::FORMAT_D32F;
    depthviewdesc.texture = depthtex;
    depthviewdesc.type = ORenderer::IMAGETYPE_2D;
    depthviewdesc.aspect = ORenderer::ASPECT_DEPTH;
    depthviewdesc.mipcount = 1;
    depthviewdesc.baselayer = 0;
    depthviewdesc.layercount = 1;
    ASSERT(ORenderer::context->createtextureview(&depthviewdesc, &depthtexview) == ORenderer::RESULT_SUCCESS, "Failed to create depth texture view.\n");

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

    ORenderer::context->createbackbuffer(rpass, &depthtexview);
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

    setupim();
    imr.init();
}

void PBRPipeline::resize(struct ORenderer::rect rendersize) {
    renderrect = rendersize;
}

void PBRPipeline::update(uint64_t flags) {

}


#include <engine/scene/scene.hpp>

OScene::Scene scene;
ORenderer::Stream imstream = ORenderer::Stream();

void PBRPipeline::execute(ORenderer::Stream *stream) {
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
    size_t zone = stream->zonebegin("Object Render");
    stream->beginrenderpass(rpass, fb, (struct ORenderer::rect) { .x = 0, .y = 0, .width = renderrect.width, .height = renderrect.height }, colourdesc);
    struct ORenderer::viewport viewport = { .x = 0, .y = 0, .width = (float)renderrect.width, .height = (float)renderrect.height, .mindepth = 0.0f, .maxdepth = 1.0f };


    stream->setpipelinestate(state); // Only ever set the state when we need to. Ideally an UBER shader should be used as we can afford the branching costs in exchange for not having to worry about materials. 
    stream->setviewport(viewport);
    stream->setscissor((struct ORenderer::rect) { .x = 0, .y = 0, .width = renderrect.width, .height = renderrect.height });

    glm::mat4 viewmtx = glm::lookAt(glm::vec3(2.0f, 2.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 _0; // Scale
    glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 _1; // Position
    glm::vec3 _2; // Skew
    glm::vec4 _3; // Perspective

    glm::decompose(viewmtx, _0, orientation, _1, _2, _3);

    ORenderer::PerspectiveCamera camera = ORenderer::PerspectiveCamera(glm::vec3(2.0f, 2.0f, 5.0f), glm::rotate(orientation, glm::radians(10.0f) * (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f)), 45.0f, renderrect.width / (float)renderrect.height, 0.1f, 1000.0f);

    viewmtx = glm::lookAt(glm::vec3(2.0f, 0.0f, -10.0f), glm::vec3(0.0f, 0.0f, -20.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    glm::decompose(viewmtx, _0, orientation, _1, _2, _3);

    ORenderer::PerspectiveCamera tcamera = ORenderer::PerspectiveCamera(glm::vec3(2.0f, 0.0f, -10.0f), glm::rotate(orientation, glm::radians(10.0f) * (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f)), 45.0f, renderrect.width / (float)renderrect.height, 0.1f, 1000.0f);

    OScene::CullResult *res = scene.partitionmanager.cull(camera);
    OScene::CullResult *reshead = res;
    size_t totalobjects = scene.objects.size();
    size_t visibleobjects = 0;
    ORenderer::ScratchBuffer *scratchbuffer;
    ASSERT(ORenderer::context->requestscratchbuffer(&scratchbuffer) == ORenderer::RESULT_SUCCESS, "Failed to request scratchbuffer.\n");
    if (res != NULL) {
        ZoneScopedN("Object Render");

        // for (auto it = scene.objects.begin(); it != scene.objects.end(); it++) {
        while (res != NULL) {
            for (size_t i = 0; i < res->header.count; i++) {
                ZoneScopedN("Individual Object Render");
                OUtils::Handle<OScene::GameObject> obj = res->objects[i];

                if ((!(obj->flags & OScene::GameObject::IS_INVISIBLE)) && obj->hascomponent(OUtils::STRINGID("ModelInstance"))) {
                    visibleobjects++;
                    OUtils::Handle<OScene::ModelInstance> model = obj->getcomponent(OUtils::STRINGID("ModelInstance"))->gethandle<OScene::ModelInstance>();

                    float time = glfwGetTime(); 

                    // data.model = glm::rotate(obj->getglobalmatrix(), time * glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // rotate Z
                    data.model = obj->getglobalmatrix();
                    data.view = camera.getview();
                    data.proj = camera.getproj();

                    size_t offset = scratchbuffer->write(&data, sizeof(struct ubo));

                    struct ORenderer::bufferbind ubobind = { .buffer = scratchbuffer->buffer, .offset = offset, .range = sizeof(struct ubo) };
                    stream->bindresource(0, ubobind, ORenderer::RESOURCE_UNIFORM);
                    struct ORenderer::sampledbind bind = { .sampler = outsampler, .view = model->model->meshes[0].material.base.view, .layout = ORenderer::LAYOUT_SHADERRO };
                    stream->bindresource(1, bind, ORenderer::RESOURCE_SAMPLER);
                    stream->commitresources();
         
                    size_t offsets = 0;
                    stream->setvtxbuffers(&model->model->meshes[0].vertexbuffer, &offsets, 0, 1);
                    stream->setidxbuffer(model->model->meshes[0].indexbuffer, 0, false); 
                    stream->drawindexed(model->model->meshes[0].indices.size(), 1, 0, 0, 0);
                }
            }
            res = res->header.next;
        }

        // Release out result pages back to the allocator.
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

    {
        ZoneScopedN("Canvas Draw");
        canvas.updateuniform(camera.getviewproj(), glfwGetTime());
        canvas.clear();
        canvas.line(glm::vec3(tcamera.pos), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        // size_t totalobjects = scene.objects.size();
        // size_t visibleobjects = 0;
        for (auto it = scene.partitionmanager.map.begin(); it != scene.partitionmanager.map.end(); it++) {
            OScene::Cell *cell = it->second;
            canvas.drawbox(glm::mat4(1.0f), OMath::AABB(cell->header.origin, cell->header.origin + 150.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        }
        // canvas.drawfrustum(camera.getview(), camera.getproj(), glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
        for (auto it = scene.objects.begin(); it != scene.objects.end(); it++) {
            if (!((*it)->flags & OScene::GameObject::IS_CULLABLE)) {
                continue;
            }

            OUtils::Handle<OScene::CullableObject> obj = (*it)->gethandle<OScene::CullableObject>();


            canvas.drawbox(obj->getglobalmatrix(), obj->bounds, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        }
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
    //     scene.partitionmanager.freeresults(res);
    // }

    canvas.updatebuffer();
    
    imstream.flushcmd();
    
    if (canvas.vertices.size()) {
        canvas.fillstream(&imstream);
    }
    // } else { // If nothing happens here so we kind of just force it to the next step (backbuffer).
    //     struct ORenderer::texture texture;
    //     struct ORenderer::backbufferinfo backbufferinfo;
    //     ORenderer::context->requestbackbufferinfo(&backbufferinfo);
    //     ORenderer::context->requestbackbuffertexture(&texture);
    //     stream->transitionlayout(texture, backbufferinfo.format, ORenderer::LAYOUT_BACKBUFFER);
    // }

    {
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
        imr.fillstream(&imstream);
        stream->submitstream(&imstream);
        stream->zoneend(zone);
    }

    stream->release();
}
