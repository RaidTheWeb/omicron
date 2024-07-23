#include <engine/renderer/im.hpp>

namespace ORenderer {
    void ImCanvas::init(void) {
        stages[0] = Shader("shaders/line.vert.spv", ORenderer::SHADER_VERTEX);
        stages[1] = Shader("shaders/line.frag.spv", ORenderer::SHADER_FRAGMENT);

        struct bufferdesc bufferdesc = { };
        bufferdesc.size = this->maxlinessize;
        bufferdesc.memprops = ORenderer::MEMPROP_GPULOCAL | ORenderer::MEMPROP_CPUVISIBLE | ORenderer::MEMPROP_CPURANDOMACCESS | ORenderer::MEMPROP_CPUCOHERENT;
        bufferdesc.usage = ORenderer::BUFFER_STORAGE | ORenderer::BUFFER_TRANSFERDST;
        bufferdesc.flags = ORenderer::BUFFERFLAG_PERFRAME;
        ASSERT(context->createbuffer(
            &this->buffer, this->maxlinessize, ORenderer::BUFFER_STORAGE | ORenderer::BUFFER_TRANSFERDST,
            ORenderer::MEMPROP_GPULOCAL | ORenderer::MEMPROP_CPUVISIBLE | ORenderer::MEMPROP_CPURANDOMACCESS | ORenderer::MEMPROP_CPUCOHERENT,
            ORenderer::BUFFERFLAG_PERFRAME
        ) == ORenderer::RESULT_SUCCESS, "Failed to create buffer.\n");

        ASSERT(context->mapbuffer(&this->buffermap, this->buffer, 0, this->maxlinessize) == ORenderer::RESULT_SUCCESS, "Failed to map buffer.\n");

        struct blendattachmentdesc attachment = { };
        attachment.writemask = ORenderer::WRITE_RGBA;
        attachment.blend = false;

        struct blendstatedesc blendstate = { };
        blendstate.logicopenable = false;
        blendstate.logicop = ORenderer::LOGICOP_COPY;
        blendstate.attachmentcount = 1;
        blendstate.attachments = &attachment;

        struct vtxinputdesc vtxinput = { };
        vtxinput.bindcount = 0;

        struct rasteriserdesc rasteriser = { };
        rasteriser.depthclamp = false;
        rasteriser.rasteriserdiscard = false;
        rasteriser.polygonmode = ORenderer::POLYGON_FILL;
        rasteriser.cullmode = ORenderer::CULL_NONE;
        rasteriser.frontface = ORenderer::FRONT_CCW;
        rasteriser.depthbias = false;
        rasteriser.depthbiasconstant = 0.0f;
        rasteriser.depthclamp = 0.0f;
        rasteriser.depthbiasslope = 0.0f;

        struct multisampledesc multisample = { };
        multisample.sampleshading = false;
        multisample.samples = ORenderer::SAMPLE_X1;
        multisample.minsampleshading = 1.0f;
        multisample.samplemask = NULL;
        multisample.alphatocoverage = false;
        multisample.alphatoone = false;

        struct pipelinestateresourcedesc resource = { };
        resource.binding = 0;
        resource.stages = ORenderer::STAGE_VERTEX;
        resource.type = ORenderer::RESOURCE_UNIFORM;

        struct pipelinestateresourcedesc dataresource = { };
        dataresource.binding = 1;
        dataresource.stages = ORenderer::STAGE_VERTEX;
        dataresource.type = ORenderer::RESOURCE_STORAGE;

        struct depthstencilstatedesc depthstencildesc = { };
        depthstencildesc.stencil = false;
        depthstencildesc.depthtest = true;
        depthstencildesc.maxdepth = 1.0f;
        depthstencildesc.mindepth = 0.0f;
        depthstencildesc.depthwrite = false;
        depthstencildesc.depthboundstest = false;
        depthstencildesc.depthcompareop = ORenderer::CMPOP_LESS;

        struct pipelinestateresourcedesc resources[2];
        resources[0] = resource;
        resources[1] = dataresource;

        // Renderpass
        struct backbufferinfo backbufferinfo = { };
        ASSERT(context->requestbackbufferinfo(&backbufferinfo) == ORenderer::RESULT_SUCCESS, "Failed to request backbuffer information.\n");

        struct rtattachmentdesc rtattachment = { };
        rtattachment.format = backbufferinfo.format; // since we're going to be using the backbuffer as our render attachment we have to use the format we specified
        rtattachment.samples = ORenderer::SAMPLE_X1;
        rtattachment.loadop = ORenderer::LOADOP_LOAD; // Don't overwrite existing data
        rtattachment.storeop = ORenderer::STOREOP_STORE;
        rtattachment.stencilloadop = ORenderer::LOADOP_DONTCARE;
        rtattachment.stencilstoreop = ORenderer::STOREOP_DONTCARE;
        rtattachment.initiallayout = ORenderer::LAYOUT_COLOURATTACHMENT;
        rtattachment.finallayout = ORenderer::LAYOUT_COLOURATTACHMENT;

        struct rtattachmentdesc depthattachment = { };
        depthattachment.format = ORenderer::FORMAT_D32F;
        depthattachment.samples = ORenderer::SAMPLE_X1;
        depthattachment.loadop = ORenderer::LOADOP_LOAD; // Do not clear
        depthattachment.storeop = ORenderer::STOREOP_DONTCARE; // We'll never need depth info later.
        depthattachment.stencilloadop = ORenderer::LOADOP_DONTCARE;
        depthattachment.stencilstoreop = ORenderer::STOREOP_DONTCARE;
        depthattachment.initiallayout = ORenderer::LAYOUT_DEPTHSTENCILATTACHMENT;
        depthattachment.finallayout = ORenderer::LAYOUT_DEPTHSTENCILATTACHMENT;

        struct rtattachmentrefdesc colourref = { };
        colourref.attachment = 0;

        struct rtattachmentrefdesc depthref = { };
        depthref.attachment = 1;

        struct ORenderer::rtattachmentdesc attachments[2] = { rtattachment, depthattachment };

        ASSERT(context->createrenderpass(&this->rpass, 2, attachments, 1, &colourref, &depthref) == ORenderer::RESULT_SUCCESS, "Failed to create render pass.\n");


        struct pipelinestatedesc desc = { };
        desc.stagecount = 2;
        desc.stages = this->stages;
        desc.tesspoints = 4;
        desc.scissorcount = 1;
        desc.viewportcount = 1;
        desc.primtopology = ORenderer::PRIMTOPOLOGY_LINELIST;
        desc.renderpass = &this->rpass;
        desc.blendstate = &blendstate;
        desc.rasteriser = &rasteriser;
        desc.vtxinput = &vtxinput;
        // desc.depthstencil = NULL;
        desc.depthstencil = &depthstencildesc;
        desc.multisample = &multisample;
        ASSERT(context->createpipelinestate(&desc, &this->state) == ORenderer::RESULT_SUCCESS, "Failed to create pipeline state.\n");
    }

    void ImCanvas::fillstream(Stream *stream) {
        if (this->vertices.empty()) {
            return;
        }

        stream->claim();
        stream->begin();
        // size_t zone = stream->zonebegin("ImCanvas");

        struct clearcolourdesc colourdesc = { };
        colourdesc.count = 2;
        colourdesc.clear[0].isdepth = false;
        colourdesc.clear[0].colour = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        colourdesc.clear[1].isdepth = true;
        colourdesc.clear[1].depth = 1.0f;
        colourdesc.clear[1].stencil = 0;

        struct framebuffer fb = { };
        context->requestbackbuffer(&fb);

        ScratchBuffer *scratchbuffer = context->requestscratchbuffer();

        stream->beginrenderpass(this->rpass, fb, (struct rect) { .x = 0, .y = 0, .width = 1280, .height = 720 }, colourdesc);
        struct viewport viewport = { .x = 0, .y = 0, .width = 1280, .height = 720, .mindepth = 0.0f, .maxdepth = 1.0f };

        stream->setpipelinestate(state);
        stream->setviewport(viewport);
        stream->setscissor((struct rect) { .x = 0, .y = 0, .width = 1280, .height = 720 });

        size_t offset = scratchbuffer->write(&this->data, sizeof(struct ubo));
        struct ORenderer::bufferbind ubobind = { .buffer = scratchbuffer->buffer, .offset = offset, .range = sizeof(struct ubo) };
        stream->bindresource(0, ubobind, ORenderer::RESOURCE_UNIFORM);

        struct ORenderer::bufferbind vtxbind = { .buffer = this->buffer, .offset = 0, .range = this->vertices.size() * sizeof(struct vertexdata) };
        stream->bindresource(1, vtxbind, ORenderer::RESOURCE_STORAGE);
        stream->commitresources();
        stream->draw(this->vertices.size(), 1, 0, 0);

        stream->endrenderpass();

        // stream->zoneend(zone);
        stream->end();
        stream->release();
    }

    void ImCanvas::updatebuffer(void) {
        ZoneScopedN("ImCanvas GPU Upload");
        if (this->vertices.empty()) {
            return;
        }

        size_t datasize = this->vertices.size() * sizeof(struct vertexdata);
        // int64_t remainingdata = datasize;
        // size_t offset = 0;
        // uint8_t *input = (uint8_t *)this->vertices.data();
        // while (remainingdata > 0) {
        //     size_t chunkcount = remainingdata < this->uploadchunksize ? remainingdata : this->uploadchunksize; // Upload data in a chunk.
        //
        //     memcpy(((uint8_t *)this->stagingmap.mapped[0]), input + offset, chunkcount);
        //
        //     struct ORenderer::buffercopydesc buffercopy = { };
        //     buffercopy.src = this->staging;
        //     buffercopy.dst = this->buffer;
        //     buffercopy.srcoffset = 0;
        //     buffercopy.dstoffset = offset;
        //     buffercopy.size = chunkcount;
        //     ORenderer::context->copybuffer(&buffercopy); // XXX: Stream this.
        //
        //     remainingdata -= chunkcount;
        //     offset += chunkcount;
        // }

        ASSERT(datasize <= this->maxlinessize, "Too much data.\n");
        memcpy(this->buffermap.mapped[context->getlatency()], this->vertices.data(), datasize);
    }

    void ImCanvas::updateuniform(glm::mat4 mvp, float time) {
        this->data.mvp = mvp;
        this->data.time = time;
    }

    void ImCanvas::clear(void) {
        this->vertices.clear();
    }

    void ImCanvas::line(glm::vec3 p1, glm::vec3 p2, glm::vec4 colour) {
        // Push vertices for lines (pairs of two indicates start and end point)
        this->vertices.push_back({ .position = p1, .colour = colour });
        this->vertices.push_back({ .position = p2, .colour = colour });
    }

    void ImCanvas::plane3d(glm::vec3 origin, glm::vec3 v1, glm::vec3 v2, int n1, int n2, float s1, float s2, glm::vec4 colour, glm::vec4 outline) {
        this->line(
            origin - s1 / 2.0f * v1 - s2 / 2.0f * v2,
            origin - s1 / 2.0f * v1 + s2 / 2.0f * v2,
            outline);
        this->line(
            origin + s1 / 2.0f * v1 - s2 / 2.0f * v2,
            origin + s1 / 2.0f * v1 + s2 / 2.0f * v2,
            outline);
        this->line(
            origin - s1 / 2.0f * v1 + s2 / 2.0f * v2,
            origin + s1 / 2.0f * v1 + s2 / 2.0f * v2,
            outline);
        this->line(
            origin - s1 / 2.0f * v1 - s2 / 2.0f * v2,
            origin + s1 / 2.0f * v1 - s2 / 2.0f * v2,
            outline);

        for (int i = 1; i < n1; i++) {
            const float t = ((float)i - (float)n1 / 2.0f) * s1 / (float)n1;

            const glm::vec3 o1 = origin + t * v1;
            this->line(o1 - s2 / 2.0f * v2, o1 + s2 / 2.0f * v2, colour);
        }

        for (int i = 1; i < n2; i++) {
            const float t = ((float)i - (float)n2 / 2.0f) * s2 / (float)n2;

            const glm::vec3 o2 = origin + t * v2;
            this->line(o2 - s1 / 2.0f * v1, o2 + s1 / 2.0f * v1, colour);
        }
    }

    void ImCanvas::drawbox(glm::mat4 model, glm::vec3 size, glm::vec4 colour, glm::vec3 centre) {
        glm::vec3 pts[8] = {
            glm::vec3(size.x, size.y, size.z),
            glm::vec3(size.x, size.y, -size.z),
            glm::vec3(size.x, -size.y, size.z),
            glm::vec3(size.x, -size.y, -size.z),
            glm::vec3(-size.x, size.y, size.z),
            glm::vec3(-size.x, size.y, -size.z),
            glm::vec3(-size.x, -size.y, size.z),
            glm::vec3(-size.x, -size.y, -size.z)
        };

        for (size_t i = 0; i < 8; i++) {
            pts[i] = glm::vec3(model * glm::vec4(pts[i], 1.0f)); // Transform points by matrix
        }

        this->line(pts[0], pts[1], colour);
        this->line(pts[2], pts[3], colour);
        this->line(pts[4], pts[5], colour);
        this->line(pts[6], pts[7], colour);

        this->line(pts[0], pts[2], colour);
        this->line(pts[1], pts[3], colour);
        this->line(pts[4], pts[6], colour);
        this->line(pts[5], pts[7], colour);

        this->line(pts[0], pts[4], colour);
        this->line(pts[1], pts[5], colour);
        this->line(pts[2], pts[6], colour);
        this->line(pts[3], pts[7], colour);

        this->line(centre, pts[7], colour);
    }

    void ImCanvas::drawbox(glm::mat4 model, OMath::AABB bounds, glm::vec4 colour) {
        this->drawbox(model * glm::translate(glm::mat4(1.0f), bounds.centre), bounds.extent * 0.5f, colour, bounds.transformed(model).getcentre());
    }

    void ImCanvas::drawfrustum(glm::mat4 view, glm::mat4 proj, glm::vec4 colour) {
        const glm::vec3 corners[8] = {
            glm::vec3(1, -1, -1), glm::vec3(1, -1, 1),
            glm::vec3(1, 1, -1), glm::vec3(1, 1, 1),
            glm::vec3(-1, 1, -1), glm::vec3(-1, 1, 1),
            glm::vec3(-1, -1, -1), glm::vec3(-1, -1, 1)
        };

        glm::vec3 pp[8];

        for (size_t i = 0; i < 8; i++) {
            glm::vec4 q = glm::inverse(view) * glm::inverse(proj) * glm::vec4(corners[i], 1.0f);
            pp[i] = glm::vec3(q.x / q.w, q.y / q.w, q.z / q.w);
        }

        for (size_t i = 0; i < 4; i++) {
            this->line(pp[i * 2], pp[i * 2 + 1], colour);

            for (size_t j = 0; j < 2; j++) {
                this->line(pp[j + i * 2], pp[j + ((i + 1) % 4) * 2], colour);
            }
        }
    }



    // ImGui

    void ImGuiRenderer::init(void) {
        stages[0] = Shader("shaders/imgui.vert.spv", ORenderer::SHADER_VERTEX);
        stages[1] = Shader("shaders/imgui.frag.spv", ORenderer::SHADER_FRAGMENT);
        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        io.IniFilename = NULL;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

        ImGuiStyle &style = ImGui::GetStyle();
        ImGui::StyleColorsDark(&style);
        style.FrameRounding = 4.0f;
        style.WindowBorderSize = 0.0f;

        ImFontConfig conf = ImFontConfig();
        conf.FontDataOwnedByAtlas = false;
        conf.MergeMode = false;

        this->imfont[0] = io.Fonts->AddFontFromFileTTF("src/engine/fonts/JetBrainsMono-Regular.ttf", 18, &conf, io.Fonts->GetGlyphRangesDefault());
        conf.MergeMode = true;
        conf.DstFont = this->imfont[0];

        ImWchar range[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
        this->imfont[1] = io.Fonts->AddFontFromFileTTF("src/engine/fonts/Font_Awesome_5-Free-Solid-900.otf", 15, &conf, range);

        uint8_t *pixels = NULL;
        int width, height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        const size_t size = width * height * 4;

        ASSERT(pixels != NULL, "Failed to load font texture.\n");

        struct ORenderer::buffer staging = { };
        ORenderer::context->createbuffer(
            &staging, size, ORenderer::BUFFER_TRANSFERSRC,
            ORenderer::MEMPROP_CPUVISIBLE | ORenderer::MEMPROP_CPUCOHERENT | ORenderer::MEMPROP_CPUSEQUENTIALWRITE,
            0
        );

        struct ORenderer::buffermap stagingmap = { };
        ORenderer::context->mapbuffer(&stagingmap, staging, 0, size);
        memcpy(stagingmap.mapped[0], pixels, size);
        ORenderer::context->unmapbuffer(stagingmap);

        ORenderer::Stream *stream = ORenderer::context->requeststream(ORenderer::STREAM_IMMEDIATE);
        struct ORenderer::bufferimagecopy region = { };
        region.offset = 0;
        region.rowlen = 0;
        region.imgheight = 0;
        region.imgoff = { 0, 0, 0 };
        region.imgextent = { (size_t)width, (size_t)height, 1 };
        region.aspect = ORenderer::ASPECT_COLOUR;
        region.mip = 0;
        region.baselayer = 0;
        region.layercount = 1;

        struct ORenderer::texturedesc desc = { };
        desc.width = width;
        desc.height = height;
        desc.depth = 1;
        desc.format = ORenderer::FORMAT_RGBA8;
        desc.type = ORenderer::IMAGETYPE_2D;
        desc.mips = 1;
        desc.layers = 1;
        desc.samples = ORenderer::SAMPLE_X1;
        desc.usage = ORenderer::USAGE_SAMPLED | ORenderer::USAGE_DST;

        ORenderer::context->createtexture(
            &this->font, ORenderer::IMAGETYPE_2D, width, height,
            1, 1, 1, ORenderer::FORMAT_RGBA8, ORenderer::MEMLAYOUT_OPTIMAL,
            ORenderer::USAGE_SAMPLED | ORenderer::USAGE_DST, ORenderer::SAMPLE_X1
        );

        stream->claim();
        stream->begin();
        stream->barrier(
            this->font, desc.format, ORenderer::LAYOUT_UNDEFINED, ORenderer::LAYOUT_TRANSFERDST,
            ORenderer::PIPELINE_STAGETOP, ORenderer::PIPELINE_STAGETRANSFER, 0,
            ORenderer::ACCESS_TRANSFERWRITE
        );
        stream->copybufferimage(region, staging, this->font);
        stream->barrier(
            this->font, desc.format, ORenderer::LAYOUT_TRANSFERDST, ORenderer::LAYOUT_SHADERRO,
            ORenderer::PIPELINE_STAGETRANSFER, ORenderer::PIPELINE_STAGEFRAGMENT,
            ORenderer::ACCESS_TRANSFERWRITE, ORenderer::ACCESS_SHADERREAD | ORenderer::ACCESS_INPUTREAD
        );
        stream->end();
        stream->release();
        context->submitstream(stream, true);

        ORenderer::context->destroybuffer(&staging);


        // Recreate staging buffer but this time persistently for the uploader.
        // XXX: Consider making the upload system a separate system rather than just this.

        // Create a big SSBO for vertex and index buffer data.
        ORenderer::context->createbuffer(
            &this->ssbo, this->vtxbuffersize + this->idxbuffersize,
            ORenderer::BUFFER_STORAGE,
            ORenderer::MEMPROP_GPULOCAL | ORenderer::MEMPROP_CPUVISIBLE | ORenderer::MEMPROP_CPURANDOMACCESS | ORenderer::MEMPROP_CPUCOHERENT,
            ORenderer::BUFFERFLAG_PERFRAME
        );

        ORenderer::context->mapbuffer(
            &this->ssbomap, this->ssbo, 0,
            this->vtxbuffersize + this->idxbuffersize
        );

        pcs.vtx = context->getbufferref(this->ssbo);
        pcs.idx = pcs.vtx + this->vtxbuffersize; // Simple pointer arithmetic to point to our buffer.

        struct ORenderer::textureviewdesc viewdesc = { };
        viewdesc.format = ORenderer::FORMAT_RGBA8;
        viewdesc.texture = this->font;
        viewdesc.type = ORenderer::IMAGETYPE_2D;
        viewdesc.aspect = ORenderer::ASPECT_COLOUR;
        viewdesc.mipcount = 1;
        viewdesc.baselayer = 0;
        viewdesc.layercount = 1;
        viewdesc.basemiplevel = 0;
        ORenderer::context->createtextureview(
            &this->fontview, ORenderer::FORMAT_RGBA8, this->font,
            ORenderer::IMAGETYPE_2D, ORenderer::ASPECT_COLOUR, 0, 1, 0, 1
        );

        ORenderer::context->createsampler(
            &this->fontsampler, ORenderer::FILTER_NEAREST, ORenderer::FILTER_NEAREST,
            ORenderer::ADDR_CLAMPEDGE, 0.0f, false, 1.0f
        );

        this->samplerid = setmanager.registersampler(this->fontsampler);
        this->pcs.sampler = this->samplerid;

        this->pcs.texture = setmanager.registertexture(this->fontview);

        io.Fonts->TexID = 0;
        io.FontDefault = this->imfont[0];
        io.DisplayFramebufferScale = ImVec2(1, 1);

        // conf.DstFont = imfont;

        // Pipeline and Renderpass
        struct blendattachmentdesc attachment = { };
        attachment.writemask = ORenderer::WRITE_RGBA;
        attachment.blend = true;
        attachment.colourblendop = ORenderer::BLENDOP_ADD;
        attachment.alphablendop = ORenderer::BLENDOP_ADD;
        attachment.srcalphablendfactor = ORenderer::BLENDFACTOR_SRCALPHA;
        attachment.dstalphablendfactor = ORenderer::BLENDFACTOR_ONEMINUSSRCALPHA;
        attachment.srccolourblendfactor = ORenderer::BLENDFACTOR_SRCALPHA;
        attachment.dstcolourblendfactor = ORenderer::BLENDFACTOR_ONEMINUSSRCALPHA;

        struct blendstatedesc blendstate = { };
        blendstate.logicopenable = false;
        blendstate.logicop = ORenderer::LOGICOP_COPY;
        blendstate.attachmentcount = 1;
        blendstate.attachments = &attachment;

        struct vtxinputdesc vtxinput = { };
        vtxinput.bindcount = 0;

        struct rasteriserdesc rasteriser = { };
        rasteriser.depthclamp = false;
        rasteriser.rasteriserdiscard = false;
        rasteriser.polygonmode = ORenderer::POLYGON_FILL;
        rasteriser.cullmode = ORenderer::CULL_NONE;
        rasteriser.frontface = ORenderer::FRONT_CCW;
        rasteriser.depthbias = false;
        rasteriser.depthbiasconstant = 0.0f;
        rasteriser.depthclamp = 0.0f;
        rasteriser.depthbiasslope = 0.0f;

        struct multisampledesc multisample = { };
        multisample.sampleshading = false;
        multisample.samples = ORenderer::SAMPLE_X1;
        multisample.minsampleshading = 1.0f;
        multisample.samplemask = NULL;
        multisample.alphatocoverage = false;
        multisample.alphatoone = false;

        struct pipelinestateresourcedesc resource = { };
        resource.binding = 0;
        resource.stages = ORenderer::STAGE_VERTEX;
        resource.type = ORenderer::RESOURCE_UNIFORM;

        struct pipelinestateresourcedesc samplerresource = { };
        samplerresource.binding = 1;
        samplerresource.stages = ORenderer::STAGE_FRAGMENT;
        samplerresource.type = ORenderer::RESOURCE_SAMPLER;

        struct pipelinestateresourcedesc texresource = { };
        texresource.binding = 2;
        texresource.stages = ORenderer::STAGE_FRAGMENT;
        texresource.type = ORenderer::RESOURCE_TEXTURE;

        struct pipelinestateresourcedesc resources[3];
        resources[0] = resource;
        resources[1] = samplerresource;
        resources[2] = texresource;

        // Renderpass
        struct backbufferinfo backbufferinfo = { };
        ORenderer::context->requestbackbufferinfo(&backbufferinfo);

        struct rtattachmentdesc rtattachment = { };
        rtattachment.format = backbufferinfo.format; // since we're going to be using the backbuffer as our render attachment we have to use the format we specified
        rtattachment.samples = ORenderer::SAMPLE_X1;
        rtattachment.loadop = ORenderer::LOADOP_LOAD; // Don't overwrite existing data
        rtattachment.storeop = ORenderer::STOREOP_STORE;
        rtattachment.stencilloadop = ORenderer::LOADOP_DONTCARE;
        rtattachment.stencilstoreop = ORenderer::STOREOP_DONTCARE;
        rtattachment.initiallayout = ORenderer::LAYOUT_COLOURATTACHMENT;
        rtattachment.finallayout = ORenderer::LAYOUT_BACKBUFFER;

        struct rtattachmentdesc depthattachment = { };
        depthattachment.format = ORenderer::FORMAT_D32F;
        depthattachment.samples = ORenderer::SAMPLE_X1;
        depthattachment.loadop = ORenderer::LOADOP_LOAD; // Do not clear
        depthattachment.storeop = ORenderer::STOREOP_DONTCARE; // We'll never need depth info later.
        depthattachment.stencilloadop = ORenderer::LOADOP_DONTCARE;
        depthattachment.stencilstoreop = ORenderer::STOREOP_DONTCARE;
        depthattachment.initiallayout = ORenderer::LAYOUT_DEPTHSTENCILATTACHMENT;
        depthattachment.finallayout = ORenderer::LAYOUT_DEPTHSTENCILATTACHMENT;

        struct depthstencilstatedesc depthstencildesc = { };
        depthstencildesc.stencil = false;
        depthstencildesc.depthtest = false;
        depthstencildesc.maxdepth = 1.0f;
        depthstencildesc.mindepth = 0.0f;
        depthstencildesc.depthwrite = false;
        depthstencildesc.depthboundstest = false;
        depthstencildesc.depthcompareop = ORenderer::CMPOP_LESS;

        struct rtattachmentrefdesc colourref = { };
        colourref.attachment = 0;

        struct rtattachmentrefdesc depthref = { };
        depthref.attachment = 1;

        struct ORenderer::rtattachmentdesc attachments[2] = { rtattachment, depthattachment };

        struct renderpassdesc rpasscreate = { };
        rpasscreate.attachmentcount = 2;
        rpasscreate.attachments = attachments;
        rpasscreate.colourrefcount = 1;
        rpasscreate.colourrefs = &colourref;
        rpasscreate.depthref = &depthref;

        ORenderer::context->createrenderpass(
            &this->rpass, 2, attachments,
            1, &colourref, &depthref
        );


        struct pipelinestatedesc pdesc = { };
        pdesc.stagecount = 2;
        pdesc.stages = this->stages;
        pdesc.tesspoints = 4;
        pdesc.scissorcount = 1;
        pdesc.viewportcount = 1;
        pdesc.primtopology = ORenderer::PRIMTOPOLOGY_TRILIST;
        pdesc.renderpass = &this->rpass;
        pdesc.blendstate = &blendstate;
        pdesc.rasteriser = &rasteriser;
        pdesc.vtxinput = &vtxinput;
        pdesc.constantssize = sizeof(struct pushconstants);
        // pdesc.depthstencil = NULL;
        pdesc.depthstencil = &depthstencildesc;
        pdesc.multisample = &multisample;
        pdesc.reslayout = &setmanager.layout;
        ORenderer::context->createpipelinestate(&pdesc, &this->state);
    }

    void ImGuiRenderer::drawcmd(uint32_t width, uint32_t height, Stream *stream, const ImDrawCmd *cmd, ImVec2 clipoff, ImVec2 clipscale, size_t idxoff, size_t vtxoff) {
        if (cmd->UserCallback != NULL) {
            return; // Don't do anything here.
        }

        ImVec4 cliprect;
        cliprect.x = (cmd->ClipRect.x - clipoff.x) * clipscale.x;
        cliprect.y = (cmd->ClipRect.y - clipoff.y) * clipscale.y;
        cliprect.z = (cmd->ClipRect.z - clipoff.x) * clipscale.x;
        cliprect.w = (cmd->ClipRect.w - clipoff.y) * clipscale.y;

        if (cliprect.x < width && cliprect.y < height && cliprect.z >= 0.0f && cliprect.w >= 0.0f) {
            // Clamp absolute.
            cliprect.x = cliprect.x < 0.0f ? 0.0f : cliprect.x;
            cliprect.y = cliprect.y < 0.0f ? 0.0f : cliprect.y;
            stream->setviewport((struct viewport) {
                .x = 0,
                .y = 0,
                .width = 1280,
                .height = 720
            });
            stream->setscissor((struct rect) {
                .x = (uint16_t)cliprect.x,
                .y = (uint16_t)cliprect.y,
                .width = (uint16_t)(cliprect.z - cliprect.x),
                .height = (uint16_t)(cliprect.w - cliprect.y)
            });

            // size_t offset = scratchbuffer->write(&this->ubo, sizeof(struct ubo));
            // struct ORenderer::bufferbind ubobind = { .buffer = scratchbuffer->buffer, .offset = offset, .range = sizeof(struct ubo) };
            // stream->bindresource(0, ubobind, ORenderer::RESOURCE_UNIFORM);;

            // struct ORenderer::sampledbind bind = { .sampler = this->fontsampler, .view = this->fontview, .layout = ORenderer::LAYOUT_SHADERRO };
            // stream->bindresource(1, bind, ORenderer::RESOURCE_SAMPLER);
            // stream->bindresource(2, bind, ORenderer::RESOURCE_TEXTURE);

            // push our constants for the shader
            stream->pushconstants(this->state, &this->pcs, sizeof(struct pushconstants));
            // stream->commitresources();

            stream->draw(cmd->ElemCount, 1, cmd->IdxOffset + idxoff, cmd->VtxOffset + vtxoff);
        }

    }

    void ImGuiRenderer::fillstream(Stream *stream) {
        // stream->claim();
        // stream->begin();
        // size_t zone = stream->zonebegin("ImGui");

        struct clearcolourdesc colourdesc = { };
        colourdesc.count = 2;
        colourdesc.clear[0].isdepth = false;
        colourdesc.clear[0].colour = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        colourdesc.clear[1].isdepth = true;
        colourdesc.clear[1].depth = 1.0f;
        colourdesc.clear[1].stencil = 0;

        struct framebuffer fb = { };
        context->requestbackbuffer(&fb);

        ScratchBuffer *scratchbuffer = context->requestscratchbuffer();

        stream->beginrenderpass(this->rpass, fb, (struct rect) { .x = 0, .y = 0, .width = 1280, .height = 720 }, colourdesc);
        // XXX: TODO

        const ImDrawData *drawdata = ImGui::GetDrawData();
        ImVec2 clipoff = drawdata->DisplayPos;
        ImVec2 clipscale = drawdata->FramebufferScale;

        stream->setpipelinestate(this->state);
        stream->bindset(setmanager.set);

        size_t vtxoff = 0;
        size_t idxoff = 0;
        for (int i = 0; i < drawdata->CmdListsCount; i++) {
            const ImDrawList *cmdlist = drawdata->CmdLists[i];
            for (int j = 0; j < cmdlist->CmdBuffer.Size; j++) {
                const ImDrawCmd *cmd = &cmdlist->CmdBuffer[j];
                this->drawcmd(1280, 720, stream, cmd, clipoff, clipscale, idxoff, vtxoff);
            }
            idxoff += cmdlist->IdxBuffer.Size;
            vtxoff += cmdlist->VtxBuffer.Size;
        }

        stream->endrenderpass();

        // stream->zoneend(zone);
        // stream->end();
        // stream->release();
    }

    void ImGuiRenderer::updatebuffer(void) {
        ZoneScopedN("ImGui Renderer Buffer Update");
        const ImDrawData *drawdata = ImGui::GetDrawData();

        const float left = drawdata->DisplayPos.x;
        const float right = drawdata->DisplayPos.x + drawdata->DisplaySize.x;
        float top = drawdata->DisplayPos.y;
        float bottom = drawdata->DisplayPos.y + drawdata->DisplaySize.y;
        context->adjustorthoprojection(&top, &bottom);

        const glm::mat4 proj = glm::ortho(left, right, bottom, top); // Create orthographic projection from this.
        this->ubo.vp = proj; // Copy to ubo.
        this->pcs.vp = proj;


        // Same loop as before but this time we upload vertex and index differently.
        ImDrawVert *vtx = (ImDrawVert *)this->ssbomap.mapped[context->getlatency()];
        uint32_t *idx = (uint32_t *)((uint8_t *)this->ssbomap.mapped[context->getlatency()] + this->vtxbuffersize);

        // One loop for both indices and vertices since we need not worry about sequential writes here.
        for (int i = 0; i < drawdata->CmdListsCount; i++) {
            const ImDrawList *cmdlist = drawdata->CmdLists[i];
            // Copy vertices out.
            memcpy(vtx, cmdlist->VtxBuffer.Data, cmdlist->VtxBuffer.Size * sizeof(ImDrawVert));
            // Copy indices out.
            const uint16_t *src = cmdlist->IdxBuffer.Data;
            for (int j = 0; j < cmdlist->IdxBuffer.Size; j++) {
                *idx++ = (uint32_t)*src++;
            }

            vtx += cmdlist->VtxBuffer.Size;
        }
    }

}
