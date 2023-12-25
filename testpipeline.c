#include <renderer/pipeline.hpp>

size_t script_type = SCRIPT_TYPEPIPELINE;

static void pipeline_testrender(struct pipeline_pass *pass) {

}

struct ubo {
    mat4s model;
    mat4s view;
    mat4s proj;
};

struct vertex {
    vec3s pos;
    vec3s colour;
    vec2s texcoord;
};

int script_init(struct pipeline *pipeline) {
    struct pipeline_passcfg *pass = malloc(sizeof(struct pipeline_passcfg));
    memset(pass, 0, sizeof(struct pipeline_passcfg));
    pass->name = utils_stringlifetime("TestPass");
    pass->passtype = PIPELINE_PASSRENDERALWAYS;
    pass->shaderflags = PIPELINE_PASSSHADERVERTEX | PIPELINE_PASSSHADERFRAGMENT;
    pass->fsh = renderer_loadshader("t.frag.spv", RENDERER_SHADERFRAGMENT);
    pass->vsh = renderer_loadshader("t.vert.spv", RENDERER_SHADERVERTEX);
    pass->clearcolour = (vec4s) { 0.0f, 0.0f, 0.0f, 1.0f };
    pass->cleardepth = 1.0f;
    pass->clearstencil = 0;

    pass->colourwritemask = RENDERER_WRITER | RENDERER_WRITEG | RENDERER_WRITEB | RENDERER_WRITEA;
    pass->blendenable = true;
    pass->srcblendfactor = RENDERER_BLENDFACTORSRCALPHA;
    pass->dstblendfactor = RENDERER_BLENDFACTORONEMINUSSRCALPHA;
    pass->blendop = RENDERER_BLENDOPADD;
    pass->srcalphablendfactor = RENDERER_BLENDFACTORONEMINUSSRCALPHA;
    pass->dstalphablendfactor = RENDERER_BLENDFACTORZERO;
    pass->alphablendop = RENDERER_BLENDOPADD;

    pass->blendlogicopenable = false;
    pass->blendlogicop = RENDERER_LOGICOPCOPY;

    pass->depthtest = true;
    pass->depthwrite = true;
    pass->depthcompareop = RENDERER_CMPALWAYS;
    pass->depthboundstest = false;
    pass->mindepth = 0.0f;
    pass->maxdepth = 1.0f;
    pass->stenciltest = false;

    pass->depthclampenable = false;
    pass->rasteriserdiscard = false;
    pass->polygonmode = RENDERER_POLYGONFILL;
    pass->cullmode = RENDERER_CULLNONE;
    pass->frontface = RENDERER_FRONTCW;
    pass->depthbiasenable = false;
    pass->depthbiasfactor = 0.0f;
    pass->depthbiasclamp = 0.0f;
    pass->depthbiasslope = 0.0f;

    pass->primtopology = RENDERER_PRIMTOPOLOGYTRILIST;

    pass->uniformrefcount = 2;
    pass->uniformrefs = malloc(sizeof(struct pipeline_uniformref) * 2);
    pass->uniformrefs[0].id = 0;
    pass->uniformrefs[0].binding = 0;
    pass->uniformrefs[1].id = 1;
    pass->uniformrefs[1].binding = 1;

    size_t *attachments = malloc(sizeof(size_t));
    attachments[0] = 0;
    pass->framebuffer = (struct pipeline_framebuffercfg) {
        .attachmentcount = 1,
        .attachments = attachments,
        .depth = true,
        .depthattachment = 1,
        .width = 1280,
        .height = 720,
        .layers = 1
    };

    pass->tesspoints = 4;

    pass->render = pipeline_testrender;

    pass->vtxlayoutcount = 0;
    // XXX: Having the user malloc everything manually is a stupid idea
    pass->vtxlayouts = malloc(sizeof(struct renderer_vertexlayout));
    pass->vtxlayouts[0].binding = 0;
    pass->vtxlayouts[0].stride = sizeof(struct vertex);
    pass->vtxlayouts[0].attribcount = 3;
    pass->vtxlayouts[0].attribs[0].offset = offsetof(struct vertex, pos);
    pass->vtxlayouts[0].attribs[0].attribtype = RENDERER_VTXATTRIBVEC3;
    pass->vtxlayouts[0].attribs[1].offset = offsetof(struct vertex, colour);
    pass->vtxlayouts[0].attribs[1].attribtype = RENDERER_VTXATTRIBVEC3;
    pass->vtxlayouts[0].attribs[2].offset = offsetof(struct vertex, texcoord);
    pass->vtxlayouts[0].attribs[2].attribtype = RENDERER_VTXATTRIBVEC2;

    struct pipeline_viewcfg *view = malloc(sizeof(struct pipeline_viewcfg));
    memset(view, 0, sizeof(struct pipeline_viewcfg));
    view->name = utils_stringlifetime("TestView");
    view->passcount = 1;
    view->passes = pass;
    struct pipeline_uniformcfg *uniforms = malloc(sizeof(struct pipeline_uniformcfg) * 2);
    uniforms[0].name = utils_stringlifetime("u_ubo");
    uniforms[0].id = 0;
    uniforms[0].size = sizeof(struct ubo);
    uniforms[0].flags = PIPELINE_UNIFORMBLOCK;
    uniforms[0].type = RENDERER_UNIFORMSTANDARD;
    uniforms[1].name = utils_stringlifetime("s_texture");
    uniforms[1].size = 0;
    uniforms[1].id = 1;
    uniforms[1].flags = PIPELINE_UNIFORMSTATIC;
    uniforms[1].type = RENDERER_UNIFORMSAMPLER;
    uniforms[1].sampler = (struct pipeline_samplercfg) {
        .minfilter = RENDERER_FILTERLINEAR,
        .magfilter = RENDERER_FILTERLINEAR,
        .umode = RENDERER_SAMPLERREPEAT,
        .vmode = RENDERER_SAMPLERREPEAT,
        .wmode = RENDERER_SAMPLERREPEAT,
        .anisotropy = true,
        .maxanistropy = 16.0f,
        .normalised = true,
        .compare = false,
        .compareop = RENDERER_CMPALWAYS,
        .mipmode = RENDERER_FILTERLINEAR,
        .lodbias = 0.0f,
        .minlod = 0.0f,
        .maxlod = 0.0f
    };

    view->uniformcount = 2;
    view->uniforms = uniforms;

    view->attachments = malloc(sizeof(struct pipeline_attachmentcfg) * 2);
    view->attachmentcount = 2;
    view->attachments[0] = (struct pipeline_attachmentcfg) {
        .id = 0,
        .format = RENDERER_TEXTUREFORMATBGRA8,
        .flags = PIPELINE_ATTACHMENTCOLOUR | PIPELINE_ATTACHMENTBLITOUTPUT,
        .width = 1280,
        .height = 720,
        .depth = 1,
        .imagetype = RENDERER_IMAGETYPE2D,
        .mips = 1,
        .layers = 1,
        .extenttype = RENDERER_EXTENTNONE,
    };
    view->attachments[1] = (struct pipeline_attachmentcfg) {
        .id = 1,
        .format = RENDERER_TEXTUREFORMATD24S8,
        .flags = PIPELINE_ATTACHMENTDEPTH,
        .width = 1280,
        .height = 720,
        .depth = 1,
        .imagetype = RENDERER_IMAGETYPE2D,
        .mips = 1,
        .layers = 1,
        .extenttype = RENDERER_EXTENTNONE
    };
    pipeline_registerview(pipeline, view);
    return 0;
}
