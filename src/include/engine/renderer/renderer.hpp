#ifndef _ENGINE__RENDERER__RENDERER_HPP
#define _ENGINE__RENDERER__RENDERER_HPP

#include <engine/math/math.hpp>
#include <map>
#include <engine/resources/resource.hpp>
#include <engine/utils.hpp>
#include <engine/utils/memory.hpp>

namespace ORenderer {

    // CPU-accessible GPU resources have to be duplicated for frame latency
#define RENDERER_MAXLATENCY 2
#define RENDERER_MAXDRAWCALLS 64000

#define RENDERER_MAXATTRIBS 16
#define RENDERER_MAXATTACHMENTS 16
#define RENDERER_INVALIDHANDLE SIZE_MAX

    struct vertexattrib {
        uint8_t attribtype;
        size_t offset;
    };

    struct vertexlayout {
        size_t stride;
        size_t binding;
        size_t attribcount;
        struct vertexattrib attribs[RENDERER_MAXATTRIBS];
    };

    enum {
        VTXATTRIB_FLOAT,
        VTXATTRIB_VEC2,
        VTXATTRIB_VEC3,
        VTXATTRIB_VEC4,
        VTXATTRIB_IVEC2,
        VTXATTRIB_IVEC3,
        VTXATTRIB_IVEC4,
        VTXATTRIB_UVEC2,
        VTXATTRIB_UVEC3,
        VTXATTRIB_UVEC4,
        VTXATTRIB_COUNT
    };

    struct platformdata {
        void *ndt;
        void *nwh;
    };

    OMICRON_EXPORT extern struct capabilities capabilities;
    OMICRON_EXPORT extern struct platformdata platformdata;

    enum {
        UNIFORM_STANDARD,
        UNIFORM_SAMPLER,
        UNIFORM_COUNT
    };

    enum {
        FILTER_NEAREST,
        FILTER_LINEAR,
        FILTER_COUNT
    };

    enum {
        ADDR_REPEAT,
        ADDR_MIRROREDREPEAT,
        ADDR_CLAMPEDGE,
        ADDR_CLAMPBORDER,
        ADDR_MIRRORCLAMPEDGE,
        ADDR_COUNT
    };

    class Shader {
        public:
            uint8_t type;
            void *code;
            size_t size;

            Shader(void) {

            }

            Shader(const char *path, uint8_t type) {
                ASSERT(path != NULL, "Attempted to load shader with NULL path.\n");
                OUtils::Handle<OResource::Resource> res = OResource::manager.get(path);
                if (res->type == OResource::Resource::SOURCE_OSFS) {
                    FILE *f = fopen(path, "r");
                    ASSERT(f != NULL, "Failed to load shader '%s'.\n", path);
                    ASSERT(!fseek(f, 0, SEEK_END), "Failed to seek to the end of file to determine file size.\n");
                    size_t size = ftell(f);
                    ASSERT(size != SIZE_MAX, "Failed to determine file size.\n");
                    ASSERT(!fseek(f, 0, SEEK_SET), "Failed to reset file pointer.\n");
                    void *code = malloc(size);
                    ASSERT(code != NULL, "Failed to allocate memory for shader.\n");
                    ASSERT(fread(code, size, 1, f), "Failed to read shader contents.\n");
                    this->type = type;
                    this->code = code;
                    this->size = size;
                } else if (res->type == OResource::Resource::SOURCE_RPAK) {
                    struct OResource::RPak::tableentry entry = res->rpakentry;
                    void *code = malloc(entry.uncompressedsize);
                    ASSERT(code != NULL, "Failed to allocate memory for shader.\n");
                    ASSERT(res->rpak->read(path, code, entry.uncompressedsize, 0) > 0, "Failed to load shader '%s'.\n", path);
                    this->size = entry.uncompressedsize;
                    this->type = type;
                    this->code = code;
                }
            }

            Shader(void *code, size_t size, uint8_t type) {
                this->type = type;
                this->code = code;
                this->size = size;
            }

            void destroy(void) {
                free(this->code);
            }
    };

    struct init {
        uint16_t vendor;
        uint16_t device;
        struct platformdata platform;
        void *window;
    };

    enum {
        RENDERFLAG_FIRST = (1 << 0), // first ever render call
        RENDERFLAG_RECTUPDATED = (1 << 1), // camera rect updated
        RENDERFLAG_EDITOR = (1 << 2) // running inside editor
    };

#define RENDERER_MAXRESOURCES 256

    struct storagekey {
        uint32_t key;
        void *value;
        struct renderer_storagekey *next;
    };

    struct localstorage {
        OJob::Mutex mutex;
        struct storagekey *head;
        struct storagekey *tail;
        std::atomic<size_t> size;
        size_t capacity;
    };

    struct rect {
        uint16_t x;
        uint16_t y;
        uint16_t width;
        uint16_t height;
    };

    struct viewport {
        float x;
        float y;
        float width;
        float height;
        float mindepth;
        float maxdepth;
    };

    enum {
        CMPOP_NEVER,
        CMPOP_LESS,
        CMPOP_EQUAL,
        CMPOP_LEQUAL,
        CMPOP_GREATER,
        CMPOP_NEQUAL,
        CMPOP_GEQUAL,
        CMPOP_ALWAYS,
        CMPOP_COUNT
    };

    enum {
        STENCILOP_KEEP,
        STENCILOP_ZERO,
        STENCILOP_REPLACE,
        STENCILOP_INCCLAMP,
        STENCILOP_DECCLAMP,
        STENCILOP_INV,
        STENCILOP_INCWRAP,
        STENCILOP_DECWRAP,
        STENCILOP_COUNT
    };

    enum {
        LOGICOP_CLEAR,
        LOGICOP_AND,
        LOGICOP_ANDR,
        LOGICOP_COPY,
        LOGICOP_ANDINV,
        LOGICOP_NOOP,
        LOGICOP_XOR,
        LOGICOP_OR,
        LOGICOP_NOR,
        LOGICOP_EQ,
        LOGICOP_INV,
        LOGICOP_ORR,
        LOGICOP_COPYINV,
        LOGICOP_ORINV,
        LOGICOP_NAND,
        LOGICOP_SET,
        LOGICOP_COUNT
    };

    enum {
        ATTACHMENT_COLOUR,
        ATTACHMENT_DEPTH
    };

    enum {
        IMAGETYPE_1D,
        IMAGETYPE_2D,
        IMAGETYPE_3D,
        IMAGETYPE_CUBE,
        IMAGETYPE_1DARRAY,
        IMAGETYPE_2DARRAY,
        IMAGETYPE_CUBEARRAY,
        IMAGETYPE_COUNT
    };

    enum {
        EXTENT_NONE, // don't do anything automatically
        EXTENT_FIXED, // attachment is fixed to the size specified
        EXTENT_FACTOR, // attachment is automatically resized to a factor (percentage) of the viewport size
        EXTENT_AR, // attachment is automatically resized based on viewport in order to match an aspect ratio
    };

    enum {
        RESULT_SUCCESS,
        RESULT_INVALIDARG,
        RESULT_FAILED
    };

    enum {
        ASPECT_COLOUR = (1 << 0),
        ASPECT_DEPTH = (1 << 1),
        ASPECT_STENCIL = (1 << 2)
    };

    enum {
        // "": UNORM, "S": SNORM, "I": SINT, "U": UINT, "F": FLOAT
        FORMAT_R8,
        FORMAT_R8I,
        FORMAT_R8U,
        FORMAT_R8S,
        FORMAT_R8SRGB,

        FORMAT_R16,
        FORMAT_R16I,
        FORMAT_R16U,
        FORMAT_R16F,
        FORMAT_R16S,

        FORMAT_R32I,
        FORMAT_R32U,
        FORMAT_R32F,

        FORMAT_RG8,
        FORMAT_RG8I,
        FORMAT_RG8U,
        FORMAT_RG8S,
        FORMAT_RG8SRGB,

        FORMAT_RG16,
        FORMAT_RG16I,
        FORMAT_RG16U,
        FORMAT_RG16F,
        FORMAT_RG16S,

        FORMAT_RG32F,

        FORMAT_RGB8,
        FORMAT_RGB8I,
        FORMAT_RGB8U,
        FORMAT_RGB8S,
        FORMAT_RGB8SRGB,

        FORMAT_RGB16,
        FORMAT_RGB16I,
        FORMAT_RGB16U,
        FORMAT_RGB16F,
        FORMAT_RGB16S,

        FORMAT_RGB32F,

        FORMAT_BGRA8,
        FORMAT_BGRA8I,
        FORMAT_BGRA8U,
        FORMAT_BGRA8S,
        FORMAT_BGRA8SRGB,

        FORMAT_RGBA8,
        FORMAT_RGBA8I,
        FORMAT_RGBA8U,
        FORMAT_RGBA8S,
        FORMAT_RGBA8SRGB,

        FORMAT_RGBA16,
        FORMAT_RGBA16I,
        FORMAT_RGBA16U,
        FORMAT_RGBA16F,
        FORMAT_RGBA16S,

        FORMAT_RGBA32I,
        FORMAT_RGBA32U,
        FORMAT_RGBA32F,

        FORMAT_B5G6R5,
        FORMAT_R5G6B5,

        FORMAT_BGRA4,
        FORMAT_RGBA4,

        FORMAT_BGR5A1,
        FORMAT_RGB5A1,

        FORMAT_RGB10A2,

        FORMAT_D16,
        FORMAT_D32F,
        FORMAT_D0S8U,
        FORMAT_D16S8U,
        FORMAT_D24S8U,
        FORMAT_D32FS8U,

        FORMAT_COUNT
    };

    static inline bool iscolourformat(size_t format) {
        return format >= FORMAT_R8 && format <= FORMAT_RGB10A2;
    }

    static inline bool isdepthformat(size_t format) {
        return format >= FORMAT_D16 && format <= FORMAT_D32FS8U;
    }

    static inline bool isstencilformat(size_t format) {
        return format >= FORMAT_D0S8U && format <= FORMAT_D32FS8U;
    }

    enum {
        MEMLAYOUT_OPTIMAL,
        MEMLAYOUT_LINEAR,
        MEMLAYOUT_COUNT
    };

    enum {
        BUFFER_TRANSFERSRC = (1 << 0),
        BUFFER_TRANSFERDST = (1 << 1),
        BUFFER_UNIFORM = (1 << 2),
        BUFFER_STORAGE = (1 << 3),
        BUFFER_INDIRECT = (1 << 4),
        BUFFER_INDEX = (1 << 5),
        BUFFER_VERTEX = (1 << 6)
    };

    enum {
        MEMPROP_GPULOCAL = (1 << 0), // memory exists on the GPU
        MEMPROP_CPUVISIBLE = (1 << 1), // memory is visible over bus to CPU
        MEMPROP_CPUCOHERENT = (1 << 2), // memory writes from CPU are detected by the GPU
        MEMPROP_CPUCACHED = (1 << 3), // CPU caches this memory
        MEMPROP_LAZYALLOC = (1 << 4), // memory is only allocated when needed
        MEMPROP_PROTECTED = (1 << 5), // memory is protected
        MEMPROP_CPUSEQUENTIALWRITE = (1 << 6), // memory is written from CPU in sequential order
        MEMPROP_CPURANDOMACCESS = (1 << 7), // memory can be read/write in random order from CPU
    };

    enum {
        RATE_VERTEX,
        RATE_INSTANCE,
        RATE_COUNT
    };

    enum {
        PRIMTOPOLOGY_PTLIST,
        PRIMTOPOLOGY_LINELIST,
        PRIMTOPOLOGY_LINESTRIP,
        PRIMTOPOLOGY_TRILIST, // default triangles
        PRIMTOPOLOGY_TRISTRIP,
        PRIMTOPOLOGY_TRIFAN,
        PRIMTOPOLOGY_ADJLINELIST,
        PRIMTOPOLOGY_ADJLINESTRIP,
        PRIMTOPOLOGY_ADJTRILIST,
        PRIMTOPOLOGY_ADJTRISTRIP,
        PRIMTOPOLOGY_PATCHLIST,
        PRIMTOPOLOGY_COUNT
    };

    enum {
        POLYGON_FILL,
        POLYGON_LINES, // wireframe view
        POLYGON_POINTS,
        POLYGON_COUNT
    };

    enum {
        CULL_NONE,
        CULL_BACK,
        CULL_FRONT,
        CULL_FRONTBACK,
        CULL_COUNT
    };

    enum {
        FRONT_CW,
        FRONT_CCW,
        FRONT_COUNT
    };

    enum {
        WRITE_R = (1 << 0),
        WRITE_G = (1 << 1),
        WRITE_B = (1 << 2),
        WRITE_A = (1 << 3),
        WRITE_RGB = WRITE_R | WRITE_G | WRITE_B,
        WRITE_RGBA = WRITE_RGB | WRITE_A
    };

    enum {
        BLENDOP_ADD,
        BLENDOP_SUB,
        BLENDOP_RSUB,
        BLENDOP_MIN,
        BLENDOP_MAX,
        BLENDOP_COUNT
    };

    enum {
        BLENDFACTOR_ZERO,
        BLENDFACTOR_ONE,
        BLENDFACTOR_SRCCOLOUR,
        BLENDFACTOR_ONEMINUSSRCCOLOUR,
        BLENDFACTOR_DSTCOLOUR,
        BLENDFACTOR_ONEMINUSDSTCOLOUR,
        BLENDFACTOR_SRCALPHA,
        BLENDFACTOR_ONEMINUSSRCALPHA,
        BLENDFACTOR_DSTALPHA,
        BLENDFACTOR_ONEMINUSDSTALPHA,
        BLENDFACTOR_CONSTANTCOLOUR,
        BLENDFACTOR_ONEMINUSCONSTANTCOLOUR,
        BLENDFACTOR_CONSTANTALPHA,
        BLENDFACTOR_ONEMINUSCONSTANTALPHA,
        BLENDFACTOR_SRCALPHASAT,
        BLENDFACTOR_SRC1COLOUR,
        BLENDFACTOR_ONEMINUSSRC1COLOUR,
        BLENDFACTOR_SRC1ALPHA,
        BLENDFACTOR_ONEMINUSSRC1ALPHA,
        BLENDFACTOR_COUNT
    };

    enum {
        SHADER_VERTEX,
        SHADER_TESSCONTROL,
        SHADER_TESSEVAL,
        SHADER_GEOMETRY,
        SHADER_FRAGMENT,
        SHADER_COMPUTE,
        SHADER_COUNT
    };

    enum {
        STAGE_VERTEX = (1 << 0),
        STAGE_TESSCONTROL = (1 << 1),
        STAGE_TESSEVALUATION = (1 << 2),
        STAGE_GEOMETRY = (1 << 3),
        STAGE_FRAGMENT = (1 << 4),
        STAGE_COMPUTE = (1 << 5),
        STAGE_ALL = (STAGE_VERTEX | STAGE_TESSCONTROL | STAGE_TESSEVALUATION | STAGE_GEOMETRY | STAGE_FRAGMENT | STAGE_COMPUTE),
        STAGE_ALLGRAPHIC = (STAGE_VERTEX | STAGE_TESSCONTROL | STAGE_TESSEVALUATION | STAGE_GEOMETRY | STAGE_FRAGMENT)
    };

    enum {
        USAGE_COLOUR = (1 << 0), // used as a colour attachment
        USAGE_DEPTHSTENCIL = (1 << 1), // used as a depth stencil attachment
        USAGE_SAMPLED = (1 << 2), // used in sampler
        USAGE_STORAGE = (1 << 3), // used as a compute storage image
        USAGE_SRC = (1 << 4), // transfer source
        USAGE_DST = (1 << 5), // transfer destination
    };

    enum {
        LOADOP_LOAD,
        LOADOP_CLEAR,
        LOADOP_DONTCARE,
        LOADOP_COUNT
    };

    enum {
        STOREOP_STORE,
        STOREOP_DONTCARE,
        STOREOP_COUNT
    };

    enum {
        LAYOUT_UNDEFINED,
        LAYOUT_GENERAL,
        LAYOUT_COLOURATTACHMENT,
        LAYOUT_DEPTHSTENCILATTACHMENT,
        LAYOUT_DEPTHSTENCILRO,
        LAYOUT_SHADERRO,
        LAYOUT_TRANSFERSRC,
        LAYOUT_TRANSFERDST,
        LAYOUT_BACKBUFFER, // for most backends this just means colour attachment or something
        LAYOUT_COUNT
    };

    enum {
        TYPEATTACHMENT_COLOUR,
        TYPEATTACHMENT_DEPTHSTENCIL
    };

    enum {
        SAMPLE_X1,
        SAMPLE_X2,
        SAMPLE_X4,
        SAMPLE_X8,
        SAMPLE_X16,
        SAMPLE_X32,
        SAMPLE_X64
    };

    enum {
        RESOURCE_SAMPLER,
        RESOURCE_TEXTURE,
        RESOURCE_STORAGETEXTURE,
        RESOURCE_UNIFORM, // UBO
        RESOURCE_STORAGE, // SSBO
    };

    struct texture {
        size_t handle = RENDERER_INVALIDHANDLE;
    };

    struct texturedesc {
        uint8_t type;

        uint32_t width;
        uint32_t height;
        uint32_t depth;

        uint32_t mips;
        uint32_t layers;

        uint8_t format;
        uint8_t memlayout; // how is the texture laid out in memory (linear or optimally?)

        size_t usage;
        uint8_t samples;
    };

    struct textureview {
        size_t handle = RENDERER_INVALIDHANDLE;
    };

    struct textureviewdesc {
        uint8_t format;
        struct texture texture;
        uint8_t type;

        size_t aspect;

        uint32_t basemiplevel;
        uint32_t mipcount;
        uint32_t baselayer;
        uint32_t layercount;
    };

    struct buffer {
        size_t handle = RENDERER_INVALIDHANDLE;
    };

    enum {
        BUFFERFLAG_MAPPED = (1 << 0), // buffer should be mapped
        BUFFERFLAG_PERFRAME = (1 << 1), // buffer is a per-frame resource (we should create a copy for every frame of latency we allow the CPU to be ahead of the GPU)
    };

    struct bufferdesc {
        size_t size;
        size_t usage;
        size_t memprops = 0;
        size_t flags = 0;
    };

    struct framebuffer {
        size_t handle = RENDERER_INVALIDHANDLE;
    };

    struct vtxbinddesc {
        uint8_t rate;
        struct vertexlayout layout;
    };

    struct vtxinputdesc {
        size_t bindcount;
        struct vtxbinddesc *binddescs;
    };

    struct rasteriserdesc {
        bool depthclamp;
        bool rasteriserdiscard;
        uint8_t polygonmode;
        uint8_t cullmode;
        uint8_t frontface;
        bool depthbias;
        float depthbiasconstant;
        float depthbiasclamp;
        float depthbiasslope;
    };

    struct multisampledesc {
        bool sampleshading;
        uint8_t samples;
        float minsampleshading;
        uint32_t *samplemask;
        bool alphatocoverage;
        bool alphatoone;
    };

    struct blendattachmentdesc {
        size_t writemask;
        bool blend;
        size_t srccolourblendfactor;
        size_t dstcolourblendfactor;
        uint8_t colourblendop;
        size_t srcalphablendfactor;
        size_t dstalphablendfactor;
        uint8_t alphablendop;
    };

    struct blendstatedesc {
        bool logicopenable;
        uint8_t logicop;
        size_t attachmentcount;
        struct blendattachmentdesc *attachments;
    };

    struct stencilstate {
        uint8_t failop;
        uint8_t passop;
        uint8_t depthfailop;
        uint8_t cmpop;
        uint32_t cmpmask;
        uint32_t writemask;
        uint32_t ref;
    };

    struct depthstencilstatedesc {
        bool depthtest;
        bool depthwrite;
        uint8_t depthcompareop;
        bool depthboundstest;
        float mindepth;
        float maxdepth;
        bool stencil;
        struct stencilstate front;
        struct stencilstate back;
    };

    struct pipelinestateresourcedesc {
        size_t binding;
        size_t stages;
        size_t type;
    };

    struct rtattachmentdesc {
        uint8_t format;
        uint8_t samples;
        uint8_t loadop;
        uint8_t storeop;
        uint8_t stencilloadop;
        uint8_t stencilstoreop;
        uint8_t initiallayout; // layout as renderpass begins
        uint8_t finallayout; // layout after renderpass finishes
    };

    struct rtattachmentrefdesc {
        size_t attachment;
    };

    struct renderpass {
        size_t handle = RENDERER_INVALIDHANDLE;
    };

    struct renderpassdesc {
        size_t attachmentcount;
        struct rtattachmentdesc *attachments;
        size_t colourrefcount;
        struct rtattachmentrefdesc *colourrefs;
        struct rtattachmentrefdesc *depthref; // this is included within attachments to.
    };

    struct framebufferdesc {
        size_t attachmentcount;
        struct textureview *attachments;
        uint32_t width;
        uint32_t height;
        uint32_t layers;
        struct renderpass pass;
    };

    struct clearcolour {
        bool isdepth;
        union {
            glm::vec4 colour;
            struct {
                float depth;
                uint32_t stencil;
            };
        };
    };

    struct clearcolourdesc {
        size_t count;
        struct clearcolour clear[RENDERER_MAXATTACHMENTS];
    };

    enum {
        COMPUTEPIPELINE,
        GRAPHICSPIPELINE
    };

    struct pipelinestate {
        size_t handle = RENDERER_INVALIDHANDLE;
    };

    struct buffermapdesc {
        struct buffer buffer;
        size_t offset;
        size_t size;
    };

    struct buffermap {
        void *mapped[RENDERER_MAXLATENCY];
        struct buffer buffer;
    };

    struct computepipelinestatedesc {
        size_t resourcecount;
        size_t constantssize = 0;
        struct resourcesetlayout *reslayout;
        Shader stage;
    };

    struct buffercopydesc {
        struct buffer src;
        struct buffer dst;
        size_t srcoffset;
        size_t dstoffset;
        size_t size;
    };

    struct backbufferinfo {
        size_t format;
        uint32_t width;
        uint32_t height;
    };

    struct pipelinestatedesc {
        uint8_t primtopology;
        size_t tesspoints;
        size_t scissorcount;
        size_t viewportcount;
        size_t stagecount;
        size_t constantssize = 0;
        struct vtxinputdesc *vtxinput;
        struct rasteriserdesc *rasteriser;
        struct multisampledesc *multisample;
        struct blendstatedesc *blendstate;
        struct depthstencilstatedesc *depthstencil;
        struct resourcesetlayout *reslayout;
        struct renderpass *renderpass;
        Shader *stages;
    };

    struct samplerdesc {
        size_t magfilter;
        size_t minfilter;
        size_t mipmode;
        size_t addru;
        size_t addrv;
        size_t addrw;
        float lodbias;
        bool anisotropyenable;
        float maxanisotropy;
        bool cmpenable;
        size_t cmpop;
        float minlod;
        float maxlod;
        bool unnormalisedcoords;
    };

    struct sampler {
        size_t handle = RENDERER_INVALIDHANDLE;
    };

    struct samplerbind {
        struct sampler sampler;
        size_t layout;
        size_t id;
    };

    struct texturebind {
        struct textureview view;
        size_t layout;
        size_t id;
    };

    enum {
        RESOURCEFLAG_BINDLESS
    };

    struct resourcedesc {
        size_t binding;
        size_t stages;
        size_t count;
        size_t type;
        size_t flag = 0;
    };

    struct resourcesetlayout {
        size_t handle = RENDERER_INVALIDHANDLE;
    };

    struct resourceset {
        size_t handle = RENDERER_INVALIDHANDLE;
    };

    struct resourcesetdesc {
        struct resourcedesc *resources;
        size_t resourcecount;
        size_t flag = 0;
    };

    class Stream;
    class ScratchBuffer;

    class RendererContext {
        public:

            RendererContext(struct init *init) { };
            virtual ~RendererContext(void) { };

            // Create a texture.
            virtual uint8_t createtexture(struct texturedesc *desc, struct texture *texture) { return RESULT_SUCCESS; }
            uint8_t createtexture(struct texture *texture, uint8_t type, uint32_t width, uint32_t height, uint32_t depth, uint32_t mips, uint32_t layers, uint8_t format, uint8_t memlayout, size_t usage, uint8_t samples) {
                struct texturedesc desc = (struct texturedesc) {
                    .type = type,
                    .width = width,
                    .height = height,
                    .depth = depth,
                    .mips = mips,
                    .layers = layers,
                    .format = format,
                    .memlayout = memlayout,
                    .usage = usage,
                    .samples = samples
                };
                return this->createtexture(&desc, texture);
            }

            // Create a texture view.
            virtual uint8_t createtextureview(struct textureviewdesc *desc, struct textureview *view) { return RESULT_SUCCESS; }
            uint8_t createtextureview(struct textureview *view, uint8_t format, struct texture texture, uint8_t type, size_t aspect, uint32_t basemiplevel, uint32_t mipcount, uint32_t baselayer, uint32_t layercount) {
                struct textureviewdesc desc = (struct textureviewdesc) {
                    .format = format,
                    .texture = texture,
                    .type = type,
                    .aspect = aspect,
                    .basemiplevel = basemiplevel,
                    .mipcount = mipcount,
                    .baselayer = baselayer,
                    .layercount = layercount
                };
                return this->createtextureview(&desc, view);
            }

            // Create a buffer of any type.
            virtual uint8_t createbuffer(struct bufferdesc *desc, struct buffer *buffer) { return RESULT_SUCCESS; }
            uint8_t createbuffer(struct buffer *buffer, size_t size, size_t usage, size_t memprops, size_t flags) {
                struct bufferdesc desc = (struct bufferdesc) {
                    .size = size,
                    .usage = usage,
                    .memprops = memprops,
                    .flags = flags
                };
                return this->createbuffer(&desc, buffer);
            }

            // Create a framebuffer.
            virtual uint8_t createframebuffer(struct framebufferdesc *desc, struct framebuffer *framebuffer) { return RESULT_SUCCESS; }
            uint8_t createframebuffer(struct framebuffer *framebuffer, size_t attachmentcount, struct textureview *attachments, uint32_t width, uint32_t height, uint32_t layers, struct renderpass pass) {
                struct framebufferdesc desc = (struct framebufferdesc) {
                    .attachmentcount = attachmentcount,
                    .attachments = attachments,
                    .width = width,
                    .height = height,
                    .layers = layers,
                    .pass = pass
                };
                return this->createframebuffer(&desc, framebuffer);

            }

            // Create a renderpass.
            virtual uint8_t createrenderpass(struct renderpassdesc *desc, struct renderpass *pass) { return RESULT_SUCCESS; }
            uint8_t createrenderpass(struct renderpass *pass, size_t attachmentcount, struct rtattachmentdesc *attachments, size_t colourrefcount, struct rtattachmentrefdesc *colourrefs, struct rtattachmentrefdesc *depthref) {
                struct renderpassdesc desc = (struct renderpassdesc) {
                    .attachmentcount = attachmentcount,
                    .attachments = attachments,
                    .colourrefcount = colourrefcount,
                    .colourrefs = colourrefs,
                    .depthref = depthref
                };
                return this->createrenderpass(&desc, pass);
            }

            // Create a graphics pipeline state.
            virtual uint8_t createpipelinestate(struct pipelinestatedesc *desc, struct pipelinestate *state) { return RESULT_SUCCESS; }

            // Create a compute pipeline state.
            virtual uint8_t createcomputepipelinestate(struct computepipelinestatedesc *desc, struct pipelinestate *state) { return RESULT_SUCCESS; }

            virtual uint8_t createresourcesetlayout(struct resourcesetdesc *desc, struct resourcesetlayout *layout) { return RESULT_SUCCESS; }
            uint8_t createresourcesetlayout(struct resourcesetlayout *layout, struct resourcedesc *resources, size_t resourcecount, size_t flag) {
                struct resourcesetdesc desc = (struct resourcesetdesc) {
                    .resources = resources,
                    .resourcecount = resourcecount,
                    .flag = flag
                };

                return this->createresourcesetlayout(&desc, layout);
            }

            virtual uint8_t createresourceset(struct resourcesetlayout *layout, struct resourceset *set) { return RESULT_SUCCESS; }

            // Create a sampler.
            virtual uint8_t createsampler(struct samplerdesc *desc, struct sampler *sampler) { return RESULT_SUCCESS; }
            uint8_t createsampler(
                struct sampler *sampler, size_t magfilter,
                size_t minfilter, size_t mipmode,
                size_t addru, size_t addrv, size_t addrw,
                float lodbias, bool anisotropyenable, float maxanisotropy,
                bool cmpenable, size_t cmpop, float minlod, float maxlod,
                bool unnormalisedcoords) {
                struct samplerdesc desc = (struct samplerdesc) {
                    .magfilter = magfilter,
                    .minfilter = minfilter,
                    .mipmode = mipmode,
                    .addru = addru,
                    .addrv = addrv,
                    .addrw = addrw,
                    .lodbias = lodbias,
                    .anisotropyenable = anisotropyenable,
                    .maxanisotropy = maxanisotropy,
                    .cmpenable = cmpenable,
                    .cmpop = cmpop,
                    .minlod = minlod,
                    .maxlod = maxlod,
                    .unnormalisedcoords = unnormalisedcoords
                };
                return this->createsampler(&desc, sampler);
            }
            uint8_t createsampler(
                struct sampler *sampler, size_t magfilter,
                size_t minfilter, size_t mipmode, size_t addru,
                size_t addrv, size_t addrw, float lodbias,
                bool anisotropyenable, float maxanisotropy) {
                return this->createsampler(sampler, magfilter, minfilter, mipmode, addru, addrv, addrw, lodbias, anisotropyenable, maxanisotropy, false, CMPOP_NEVER, 0.0f, 0.0f, false);
            }
            uint8_t createsampler(
                struct sampler *sampler, size_t filter,
                size_t mipmode, size_t addr, float lodbias,
                bool anisotropyenable, float maxanisotropy) {
                return this->createsampler(sampler, filter, filter, mipmode, addr, addr, addr, lodbias, anisotropyenable, maxanisotropy);
            }

            // XXX: Think real hard about everything, how do I want to go about it?
            // Probably anything synchronous (on demand and not related to a command "stream") should be put here as we want our changes to be reflected immediately rather than have them appear at a later date.
            // Map a buffer's memory ranges to CPU-addressable memory.
            virtual uint8_t mapbuffer(struct buffermapdesc *desc, struct buffermap *map) { return RESULT_SUCCESS; }
            uint8_t mapbuffer(struct buffermap *map, struct buffer buffer, size_t offset, size_t size) {
                struct buffermapdesc desc = (struct buffermapdesc) {
                    .buffer = buffer,
                    .offset = offset,
                    .size = size
                };
                return this->mapbuffer(&desc, map);
            }

            // Unmap a memory map.
            virtual uint8_t unmapbuffer(struct buffermap map) { return RESULT_SUCCESS; }
            // Get the current renderer latency frame.
            virtual uint8_t getlatency(void) { return 0; }

            // Copy data between two buffers.
            virtual uint8_t copybuffer(struct buffercopydesc *desc) { return RESULT_SUCCESS; }
            uint8_t copybuffer(struct buffer src, struct buffer dst, size_t srcoffset, size_t dstoffset, size_t size) {
                struct buffercopydesc desc = (struct buffercopydesc) {
                    .src = src,
                    .dst = dst,
                    .srcoffset = srcoffset,
                    .dstoffset = dstoffset,
                    .size = size
                };
                return this->copybuffer(&desc);
            }

            // Create the renderer backbuffer(s).
            virtual uint8_t createbackbuffer(struct renderpass pass, struct ORenderer::textureview *depth = NULL) { return RESULT_SUCCESS; }
            // Request a reference to the backbuffer of the current frame.
            virtual uint8_t requestbackbuffer(struct framebuffer *framebuffer) { return RESULT_SUCCESS; }
            virtual uint8_t requestbackbuffertexture(struct texture *texture) { return RESULT_SUCCESS; }
            // Request information pertaining to the current size of the backbuffer and the selected data format for it.
            virtual uint8_t requestbackbufferinfo(struct backbufferinfo *info) { return RESULT_SUCCESS; }
            // Request a reference to the current frame's scratchbuffer.
            virtual uint8_t requestscratchbuffer(ScratchBuffer **scratchbuffer) { return RESULT_SUCCESS; };
            // Transition texture between image layouts.
            virtual uint8_t transitionlayout(struct texture texture, size_t format, size_t state) { return RESULT_SUCCESS; }

            virtual ORenderer::Stream *getimmediate(void) { return NULL; }
            virtual ORenderer::Stream *getsecondary(void) { return NULL; }
            // Get reference to a buffer, be it a handle or a direct address (Vulkan).
            virtual uint64_t getbufferref(struct ORenderer::buffer buffer, uint8_t latency = UINT8_MAX) { return 0; }

            virtual void updateset(struct resourceset set, struct samplerbind *bind) { }
            virtual void updateset(struct resourceset set, struct texturebind *bind) { }

            // submitting a stream will have it executed on GPU ASAP.
            // not to be used in the pipeline for the primary stream.
            // Submit a renderer stream to be executed on the GPU ASAP.
            virtual uint8_t submitstream(Stream *stream) { return RESULT_SUCCESS; }
            // XXX: Have a way to submit another stream's commands to our current stream

            // Destroy a texture.
            virtual void destroytexture(struct texture *texture) { }
            // Destroy a texture view.
            virtual void destroytextureview(struct textureview *textureview) { }
            // Destroy a buffer.
            virtual void destroybuffer(struct buffer *buffer) { }
            // Destroy a framebuffer.
            virtual void destroyframebuffer(struct framebuffer *framebuffer) { }
            // Destroy a renderpass.
            virtual void destroyrenderpass(struct renderpass *pass) { }
            // Destroy a pipeline state.
            virtual void destroypipelinestate(struct pipelinestate *state) { }

            virtual void setdebugname(struct ORenderer::texture texture, const char *name) { }
            virtual void setdebugname(struct ORenderer::textureview view, const char *name) { }
            virtual void setdebugname(struct ORenderer::buffer buffer, const char *name) { }
            virtual void setdebugname(struct ORenderer::framebuffer framebuffer, const char *name) { }
            virtual void setdebugname(struct ORenderer::renderpass renderpass, const char *name) { }
            virtual void setdebugname(struct ORenderer::pipelinestate state, const char *name) { }

            // Adjust a projection matrix to represent the intended results from GLM
            virtual void adjustprojection(glm::mat4 *mtx) { }
            // Adjust top and bottom for intended results from GLM.
            virtual void adjustorthoprojection(float *top, float *bottom) { };
            virtual void flushrange(struct buffer buffer, size_t size) { };
    };

    class ScratchBuffer {
        public:
            struct ORenderer::buffer buffer;
            struct ORenderer::buffermap map;
            size_t size;
            size_t pos;
            size_t alignment;
            RendererContext *ctx;

            ScratchBuffer(void) { };

            void reset(void) {
                this->pos = 0;
            }

            void create(RendererContext *ctx, size_t size, size_t count, size_t alignment) {
                this->alignment = alignment;
                this->ctx = ctx;
                const size_t entsize = utils_stridealignment(size, this->alignment);

                struct ORenderer::bufferdesc desc = { };
                desc.size = entsize * count;
                desc.usage = ORenderer::BUFFER_UNIFORM;
                // exists on the GPU itself but we allow the CPU to see that memory and pass data to it when flushing the ranges, we also need random access to the memory as we rarely are writing from start to finish in its entirety.
                desc.memprops = ORenderer::MEMPROP_GPULOCAL | ORenderer::MEMPROP_CPUVISIBLE | ORenderer::MEMPROP_CPURANDOMACCESS;
                ASSERT(this->ctx->createbuffer(&desc, &this->buffer) == ORenderer::RESULT_SUCCESS, "Failed to create scratch buffer.\n");

                this->size = entsize * count;
                this->pos = 0;

                struct ORenderer::buffermapdesc mapdesc = { };
                mapdesc.buffer = this->buffer;
                mapdesc.size = entsize * count;
                mapdesc.offset = 0;
                ASSERT(this->ctx->mapbuffer(&mapdesc, &this->map) == ORenderer::RESULT_SUCCESS, "Failed to map scratch buffer.\n");
            }

            size_t write(const void *data, size_t size) {
                ZoneScopedN("Scratchbuffer Write");
                ASSERT(this->pos + size < this->size, "Not enough space for scratchbuffer write of %lu bytes.\n", size);
                size_t off = this->pos;

                memcpy(&((uint8_t *)this->map.mapped[0])[this->pos], data, size);

                this->pos += utils_stridealignment(size, this->alignment);

                return off;
            }

            void flush(void) {
                ZoneScopedN("Scratchbuffer Flush");
                const size_t size = glm::min((size_t)utils_stridealignment(this->pos, this->alignment), this->size);
                this->ctx->flushrange(this->buffer, size);
            }
    };



    extern RendererContext *context;
    RendererContext *createcontext(void);

    struct bufferimagecopy {
        size_t offset;
        size_t rowlen;
        size_t imgheight;
        glm::vec3 imgoff;
        struct {
            size_t width;
            size_t height;
            size_t depth;
        } imgextent;
        uint8_t aspect;
        size_t mip;
        size_t baselayer;
        size_t layercount;
    };

    struct sampledbind {
        struct sampler sampler;
        struct textureview view;
        size_t layout;
    };

    struct bufferbind {
        struct buffer buffer;
        size_t offset;
        size_t range;
    };

    struct pipelinestateresourcemap {
        size_t binding;
        size_t type;
        union {
            struct bufferbind bufferbind;
            struct sampledbind sampledbind;
        };
    };

    struct streamnibble {
        size_t type;
        union {
            struct viewport viewport;
            struct rect scissor;
            struct {
                struct renderpass rpass;
                struct framebuffer fb;
                struct rect area;
                struct clearcolourdesc clear;
            } renderpass;
            struct pipelinestate pipeline;
            struct {
                struct buffer buffer;
                size_t offset;
                bool index32;
            } idxbuffer;
            struct {
                struct buffer buffers[4]; // maximum bind count
                size_t offsets[4];
                size_t firstbind;
                size_t bindcount;
            } vtxbuffers;
            struct {
                size_t binding;
                size_t type;
                union {
                    struct bufferbind bufferbind;
                    struct sampledbind sampledbind;
                };
            } resource;
            struct {
                size_t vtxcount;
                size_t instancecount;
                size_t firstvtx;
                size_t firstinstance;
            } draw;
            struct {
                size_t idxcount;
                size_t instancecount;
                size_t firstidx;
                size_t vtxoffset;
                size_t firstinstance;
            } drawindexed;
            struct {
                struct buffer src;
                struct buffer dst;
                size_t srcoffset;
                size_t dstoffset;
                size_t size;
            } bufferbuffer;
            struct {
                uint8_t *data; // pointer to data, we free this after use
                struct buffer buffer;
                size_t offset;
                size_t size;
            } databuffer;
            struct {
                struct texture texture;
                size_t format;
                size_t state;
            } layout;
            struct {
                struct bufferimagecopy region;
                struct buffer buffer;
                struct texture texture;
            } copybufferimage;
            struct {
                void *dst;
                void *src;
                size_t size;
            } stagedmemcopy;
            struct {
                char *name;
                void *zone;
            } zonebegin;
            size_t zoneend;
        };
    };

    // Stream system that allows render commands to be submitted by any number of jobs and have them be executed with concurrent safety in a render context later.
    class Stream {
        private:
            OJob::Mutex mutex;
        public:
            // these few variables after this are cleared every frame
            std::vector<struct streamnibble> cmd;
            // we keep some temporary memory per-frame in order to allow us to offer persistence to data instead of letting it go out of scope, used for anything pointer related (eg. copy to temporary buffer and free later)
            OUtils::StackAllocator tempmem = OUtils::StackAllocator(16384); // a stack allocator is suitable for our purposes as we can allocate or free from a preallocated block of memory to reduce the overhead from malloc() and free() (makes a big difference as it adds up)
            // temporary storage for descriptor mappings per pipeline state
            std::vector<struct pipelinestateresourcemap> mappings;
            // temporary reference to the active pipeline state
            struct pipelinestate pipelinestate;

            enum opcode {
                OP_SETVIEWPORT,
                OP_SETSCISSOR,
                OP_BEGINRENDERPASS,
                OP_ENDRENDERPASS,
                OP_SETPIPELINESTATE,
                OP_SETIDXBUFFER,
                OP_SETVTXBUFFERS,
                OP_DRAW,
                OP_DRAWINDEXED,
                OP_BINDRESOURCE,
                OP_COMMITRESOURCES,
                OP_TRANSITIONLAYOUT,
                OP_COPYBUFFERIMAGE,
                OP_STAGEDMEMCOPY,
                OP_DEBUGZONEBEGIN,
                OP_DEBUGZONEEND
            };

            // Lock the stream to protect against out-of-order command submission (not strictly needed, but useful to prevent race conditions).
            void claim(void) {
                this->mutex.lock();
                this->tempmem.claim();
            }

            // Release stream to allow it to be claimed again.
            void release(void) {
                this->tempmem.release();
                this->mutex.unlock();
            }

            // Flush stream command list and clear all temporary memory.
            virtual void flushcmd(void) {
                // this->cmd.clear();
                // this->tempmem.clear();
                // this->mappings.clear();
            }

            virtual void setviewport(struct viewport viewport) { }

            virtual void pushconstants(struct pipelinestate state, void *data, size_t size) { }

            // Set viewport for renderering.
            // void setviewport(struct viewport viewport) {
                // this->mutex.lock();
                // this->cmd.push_back((struct streamnibble) { .type = OP_SETVIEWPORT, .viewport = viewport });
                // this->mutex.unlock();
            // }

            // Set scissor for renderering
            virtual void setscissor(struct rect rect) {
                // this->mutex.lock();
                // this->cmd.push_back((struct streamnibble) { .type = OP_SETSCISSOR, .scissor = rect });
                // this->mutex.unlock();
            }

            // Begin a renderpass for a framebuffer with renderering area and clear colours.
            virtual void beginrenderpass(struct renderpass renderpass, struct framebuffer framebuffer, struct rect area, struct clearcolourdesc clear) {
                // ZoneScoped;
                // this->mutex.lock();
                // this->cmd.push_back((struct streamnibble) { .type = OP_BEGINRENDERPASS, .renderpass = {
                    // .rpass = renderpass,
                    // .fb = framebuffer,
                    // .area = area,
                    // .clear = clear
                // } });
                // this->mutex.unlock();
            }

            // End a renderpass.
            virtual void endrenderpass(void) {
                // this->mutex.lock();
                // this->cmd.push_back((struct streamnibble) { .type = OP_ENDRENDERPASS });
                // this->mutex.unlock();
            }

            // Set the current pipeline state.
            virtual void setpipelinestate(struct pipelinestate pipeline) {
                // ZoneScoped;
                // this->mutex.lock();
                // this->cmd.push_back((struct streamnibble) { .type = OP_SETPIPELINESTATE, .pipeline = pipeline });
                // this->mutex.unlock();
            }

            // Set the current index buffer for use before a draw call.
            virtual void setidxbuffer(struct buffer buffer, size_t offset, bool index32) {
                // ZoneScoped;
                // this->mutex.lock();
                // this->cmd.push_back((struct streamnibble) { .type = OP_SETIDXBUFFER, .idxbuffer = {
                    // .buffer = buffer,
                    // .offset = offset,
                    // .index32 = false,
                // } });
                // this->mutex.unlock();
            }

            // Set the current vertex buffers for use before a draw call.
            virtual void setvtxbuffers(struct buffer *buffers, size_t *offsets, size_t firstbind, size_t bindcount) {
                // ZoneScoped;
                // ASSERT(bindcount <= 4, "Too many vertex buffers submitted to stream at once.\n");
                // this->mutex.lock();
                // struct streamnibble nibble = (struct streamnibble) { .type = OP_SETVTXBUFFERS, .vtxbuffers = {
                    // .buffers = { },
                    // .offsets = { },
                    // .firstbind = firstbind,
                    // .bindcount = bindcount
                // } };
                // for (size_t i = 0; i < bindcount; i++) {
                    // nibble.vtxbuffers.buffers[i] = buffers[i];
                    // nibble.vtxbuffers.offsets[i] = offsets[i];
                // }

                // this->cmd.push_back(nibble);
                // this->mutex.unlock();
            }

            virtual void setvtxbuffer(struct buffer buffer, size_t offset) {
                // ZoneScoped;
                // this->mutex.lock();
                // struct streamnibble nibble = (struct streamnibble) { .type = OP_SETVTXBUFFERS, .vtxbuffers = {
                    // .buffers = { buffer },
                    // .offsets = { offset },
                    // .firstbind = 0,
                    // .bindcount = 1
                // } };

                // this->cmd.push_back(nibble);
                // this->mutex.unlock();
            }

            // Draw unindexed vertices.
            virtual void draw(size_t vtxcount, size_t instancecount, size_t firstvtx, size_t firstinstance) {
                // ZoneScoped;
                // this->mutex.lock();
                // this->cmd.push_back((struct streamnibble) { .type = OP_DRAW, .draw = {
                //     .vtxcount = vtxcount,
                //     .instancecount = instancecount,
                //     .firstvtx = firstvtx,
                //     .firstinstance = firstinstance
                // } });
                // this->mutex.unlock();
            }

            // Draw indexed vertices.
            virtual void drawindexed(size_t idxcount, size_t instancecount, size_t firstidx, size_t vtxoffset, size_t firstinstance) {
                // ZoneScoped;
                // this->mutex.lock();
                // this->cmd.push_back((struct streamnibble) { .type = OP_DRAWINDEXED, .drawindexed = {
                //     .idxcount = idxcount,
                //     .instancecount = instancecount,
                //     .firstidx = firstidx,
                //     .vtxoffset = vtxoffset,
                //     .firstinstance = firstinstance
                // } });
                // this->mutex.unlock();
            }

            // Commit currently set pipeline resources.
            virtual void commitresources(void) {
                // ZoneScoped;
                // this->mutex.lock();
                // this->cmd.push_back((struct streamnibble) { .type = OP_COMMITRESOURCES });
                // this->mutex.unlock();
            }

            // Bind a pipeline resource to a binding.
            virtual void bindresource(size_t binding, struct bufferbind bind, size_t type) {
                // ZoneScoped;
                // this->mutex.lock();
                // this->cmd.push_back((struct streamnibble) { .type = OP_BINDRESOURCE, .resource = { .binding = binding, .type = type, .bufferbind = bind } });
                // this->mutex.unlock();
            }

            virtual void bindresource(size_t binding, struct sampledbind bind, size_t type) {
                // ZoneScoped;
                // this->mutex.lock();
                // this->cmd.push_back((struct streamnibble) { .type = OP_BINDRESOURCE, .resource = { .binding = binding, .type = type, .sampledbind = bind } });
                // this->mutex.unlock();
            }

            virtual void bindset(struct resourceset set) {

            }

            // Transition a texture between layouts.
            virtual void transitionlayout(struct texture texture, size_t format, size_t state) {
                // ZoneScoped;
                // this->mutex.lock();
                // this->cmd.push_back((struct streamnibble) { .type = OP_TRANSITIONLAYOUT, .layout = {
                    // .texture = texture, .format = format, .state = state
                // } });
                // this->mutex.unlock();
            }

            virtual void copybufferimage(struct bufferimagecopy region, struct buffer buffer, struct texture texture) {
                // ZoneScoped;
                // this->mutex.lock();
                // this->cmd.push_back((struct streamnibble) { .type = OP_COPYBUFFERIMAGE, .copybufferimage =  {
                    // .region = region, .buffer = buffer, .texture = texture
                // } });
                // this->mutex.unlock();
            }

            // Submit another stream's command list to the current command list (the result will be as if the commands were submitted to this current stream)
            virtual void submitstream(Stream *stream) {
                // ZoneScoped;
                // this->mutex.lock();
                // stream->mutex.lock();
                // stream->claim();
                // for (auto it = stream->cmd.begin(); it != stream->cmd.end(); it++) {
                //     if (it->type == OP_DEBUGZONEBEGIN) {
                //         this->zonebegin(it->zonebegin.name);
                //     } else {
                //        this->cmd.push_back(*it);
                //     }
                // }
                //
                // stream->release();
                // stream->mutex.unlock();
                // this->mutex.unlock();
            }

            virtual void stagedmemcopy(void *dst, void *src, size_t size) {
                // ZoneScoped;
                // void *srccpy = this->tempmem.alloc(size);
                // memcpy(srccpy, src, size);
                // this->cmd.push_back((struct streamnibble) { .type = OP_STAGEDMEMCOPY, .stagedmemcopy = {
                //     .dst = dst,
                //     .src = srccpy,
                //     .size = size
                // } });
            }

            virtual uint64_t zonebegin(const char *name) {
                // ZoneScoped;
                // size_t len = strnlen(name, 63);
                // char *tmp = (char *)this->tempmem.alloc(len + 1); // Hard limit here to prevent a pipeline zone name from consuming all of the stack memory.
                // memset(tmp, 0, len + 1);
                // strncpy(tmp, name, len);
                // size_t index = this->cmd.size();
                // this->cmd.push_back((struct streamnibble) { .type = OP_DEBUGZONEBEGIN, .zonebegin = {
                //     .name = tmp,
                //     .zone = NULL
                // } });
                return 0;
            }

            virtual void zoneend(uint64_t zone) {
                // this->cmd.push_back((struct streamnibble) { .type = OP_DEBUGZONEEND, .zoneend = zone });
            }

            virtual void begin(void) { }
            virtual void end(void) { }
    };

    // Upload data of a certain size to an offset in a buffer (this can be on a GPU or on the CPU itself, doesn't matter, all this does is do a staging buffer copy).
    static inline void uploadtobuffer(ORenderer::buffer buffer, size_t size, void *data, size_t off = 0) {
        // TODO: Consider using mapped memory if supported to allow for a quicker upload.
        struct ORenderer::bufferdesc bufferdesc = { };
        bufferdesc.size = size;
        bufferdesc.usage = ORenderer::BUFFER_TRANSFERSRC;
        bufferdesc.memprops = ORenderer::MEMPROP_CPUVISIBLE | ORenderer::MEMPROP_CPUCOHERENT | ORenderer::MEMPROP_CPUSEQUENTIALWRITE;
        bufferdesc.flags = 0;
        struct ORenderer::buffer staging = { };
        ASSERT(ORenderer::context->createbuffer(&bufferdesc, &staging) == ORenderer::RESULT_SUCCESS, "Failed to create staging buffer.\n");

        struct ORenderer::buffermapdesc stagingmapdesc = { };
        stagingmapdesc.buffer = staging;
        stagingmapdesc.size = size;
        stagingmapdesc.offset = 0;
        struct ORenderer::buffermap stagingmap = { };
        ASSERT(ORenderer::context->mapbuffer(&stagingmapdesc, &stagingmap) == ORenderer::RESULT_SUCCESS, "Failed to map staging buffer.\n");
        memcpy(stagingmap.mapped[0], data, size);
        ORenderer::context->unmapbuffer(stagingmap);

        struct ORenderer::buffercopydesc buffercopy = { };
        buffercopy.src = staging;
        buffercopy.dst = buffer;
        buffercopy.srcoffset = 0;
        buffercopy.dstoffset = off;
        buffercopy.size = size;
        ORenderer::context->copybuffer(&buffercopy);

        ORenderer::context->destroybuffer(&staging);
    }

    struct renderer {
        // struct camera *camera;

        // localised storage per renderer
        // used for things like cluster AABBs (based on renderer width, height, and depth)
        struct localstorage *local;
    };

    extern std::map<uint32_t, struct pipelinestate> pipelinestatecache;

    OMICRON_EXPORT struct renderer *initrenderer(void);
    // OMICRON_EXPORT struct renderer *init(void);

}

#endif
