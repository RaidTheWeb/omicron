#ifndef _RENDERER__RENDERER_HPP
#define _RENDERER__RENDERER_HPP

#include <engine/math.hpp>
#include <map>
#include <resources/resource.hpp>
#include <utils.hpp>
#include <utils/memory.hpp>

namespace ORenderer {

    // CPU-accessible GPU resources have to be duplicated for frame latency
#define RENDERER_MAXLATENCY 2

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
        SAMPLER_REPEAT,
        SAMPLER_MIRROREDREPEAT,
        SAMPLER_CLAMPEDGE,
        SAMPLER_CLAMPBORDER,
        SAMPLER_MIRRORCLAMPEDGE,
        SAMPLER_COUNT
    };

    class Shader {
        public:
            uint8_t type;
            void *code;
            size_t size;

            Shader(const char *path, uint8_t type) {
                FILE *f = fopen(path, "r");
                ASSERT(f != NULL, "Failed to load shader '%s'.\n", path);
                fseek(f, 0, SEEK_END);
                size_t size = ftell(f);
                fseek(f, 0, SEEK_SET);
                void *code = malloc(size);
                fread(code, size, 1, f);
                this->type = type;
                this->code = code;
                this->size = size;
            }

            Shader(void *code, size_t size, uint8_t type) {
                this->type = type;
                this->code = code;
                this->size = size;
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

        FORMAT_RGBA8,
        FORMAT_RGBA8I,
        FORMAT_RGBA8U,
        FORMAT_RGBA8S,

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
        STAGE_COMPUTE = (1 << 5)
    };

#define STAGE_ALL (STAGE_VERTEX | STAGE_TESSCONTROL | STAGE_TESSEVALUATION | STAGE_GEOMETRY | STAGE_FRAGMENT | STAGE_COMPUTE)
#define STAGE_ALLGRAPHIC (STAGE_VERTEX | STAGE_TESSCONTROL | STAGE_TESSEVALUATION | STAGE_GEOMETRY | STAGE_FRAGMENT)

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
        RESOURCE_STORAGEIMAGE,
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
        struct pipelinestateresourcedesc *resources;
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
        size_t resourcecount;
        size_t stagecount;
        struct vtxinputdesc *vtxinput;
        struct rasteriserdesc *rasteriser;
        struct multisampledesc *multisample;
        struct blendstatedesc *blendstate;
        struct depthstencilstatedesc *depthstencil;
        struct pipelinestateresourcedesc *resources;
        struct renderpass *renderpass;
        Shader *stages;
    };

    class Stream;

    class RendererContext {
        public:
            RendererContext(struct init *init) { };
            virtual ~RendererContext(void) { };

            // Create a texture.
            virtual uint8_t createtexture(struct texturedesc *desc, struct texture *texture) { return RESULT_SUCCESS; }

            // Create a texture view.
            virtual uint8_t createtextureview(struct textureviewdesc *desc, struct textureview *view) { return RESULT_SUCCESS; }

            // Create a buffer of any type.
            virtual uint8_t createbuffer(struct bufferdesc *desc, struct buffer *buffer) { return RESULT_SUCCESS; }

            // Create a framebuffer.
            virtual uint8_t createframebuffer(struct framebufferdesc *desc, struct framebuffer *framebuffer) { return RESULT_SUCCESS; }

            // Create a renderpass.
            virtual uint8_t createrenderpass(struct renderpassdesc *desc, struct renderpass *pass) { return RESULT_SUCCESS; }

            // Create a graphics pipeline state.
            virtual uint8_t createpipelinestate(struct pipelinestatedesc *desc, struct pipelinestate *state) { return RESULT_SUCCESS; }

            // Create a compute pipeline state.
            virtual uint8_t createcomputepipelinestate(struct computepipelinestatedesc *desc, struct pipelinestate *state) { return RESULT_SUCCESS; }

            // XXX: Think real hard about everything, how do I want to go about it?
            // Probably anything synchronous (on demand and not related to a command "stream") should be put here as we want our changes to be reflected immediately rather than have them appear at a later date.
            // Map a buffer's memory ranges to CPU-addressable memory.
            virtual uint8_t mapbuffer(struct buffermapdesc *desc, struct buffermap *map) { return RESULT_SUCCESS; }
            // Unmap a memory map.
            virtual uint8_t unmapbuffer(struct buffermap map) { return RESULT_SUCCESS; }
            // Get the current renderer latency frame.
            virtual uint8_t getlatency(void) { return 0; }
            // Copy data between two buffers.
            virtual uint8_t copybuffer(struct buffercopydesc *desc) { return RESULT_SUCCESS; }
            // Create the renderer backbuffer(s).
            virtual uint8_t createbackbuffer(struct renderpass pass) { return RESULT_SUCCESS; }
            // Request a reference to the backbuffer of the current frame.
            virtual uint8_t requestbackbuffer(struct framebuffer *framebuffer) { return RESULT_SUCCESS; }
            // Request information pertaining to the current size of the backbuffer and the selected data format for it.
            virtual uint8_t requestbackbufferinfo(struct backbufferinfo *info) { return RESULT_SUCCESS; }
            // Transition texture between image layouts.
            virtual uint8_t transitionlayout(struct texture texture, size_t format, size_t state) { return RESULT_SUCCESS; }

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

            // Adjust a projection matrix to represent the intended results from GLM
            virtual void adjustprojection(glm::mat4 *mtx) { }
    };


    extern RendererContext *context;
    RendererContext *createcontext(void);

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
                struct buffer *buffers;
                size_t *offsets;
                size_t firstbind;
                size_t bindcount;
            } vtxbuffers;
            struct {
                size_t binding;
                size_t type;
                union {
                    struct buffer buffer;
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
                size_t binding;
                struct buffer buffer;
            } bindbuffer;
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
        };
    };

    struct pipelinestateresourcemap {
        size_t binding;
        size_t type;
        union {
            struct buffer buffer;
            size_t format;
            size_t state;
        };
    };

    // Stream system that allows render commands to be submitted by any number of jobs and have them be executed with concurrent safety in a render context later.
    class Stream {
        public:
            OJob::Mutex mutex;

            // these few variables after this are cleared every frame
            std::vector<struct streamnibble> cmd;
            // we keep some temporary memory per-frame in order to allow us to offer persistence to data instead of letting it go out of scope, used for anything pointer related (eg. copy to temporary buffer and free later)
            OUtils::StackAllocator tempmem = OUtils::StackAllocator(8096); // a stack allocator is suitable for our purposes as we can allocate or free from a preallocated block of memory to reduce the overhead from malloc() and free() (makes a big difference as it adds up)
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
                OP_TRANSITIONLAYOUT
            };

            // Flush stream command list and clear all temporary memory.
            void flushcmd(void) {
                this->cmd.clear();
                this->tempmem.clear();
                this->mappings.clear();
            }

            inline struct buffer *allocfor(struct buffer *buffers, size_t count) {
                struct buffer *allocation = (struct buffer *)this->tempmem.alloc(sizeof(struct buffer) * count);
                memcpy(allocation, buffers, sizeof(struct buffer) * count);
                return allocation;
            }

            inline size_t *allocfor(size_t *offsets, size_t count) {
                size_t *allocation = (size_t *)this->tempmem.alloc(sizeof(size_t) * count);
                memcpy(allocation, offsets, sizeof(size_t) * count);
                return allocation;
            }

            // Set viewport for renderering.
            void setviewport(struct viewport viewport) {
                this->mutex.lock();
                this->cmd.push_back((struct streamnibble) { .type = OP_SETVIEWPORT, .viewport = viewport });
                this->mutex.unlock();
            }

            // Set scissor for renderering
            void setscissor(struct rect rect) {
                this->mutex.lock();
                this->cmd.push_back((struct streamnibble) { .type = OP_SETSCISSOR, .scissor = rect });
                this->mutex.unlock();
            }

            // Begin a renderpass for a framebuffer with renderering area and clear colours.
            void beginrenderpass(struct renderpass renderpass, struct framebuffer framebuffer, struct rect area, struct clearcolourdesc clear) {
                this->mutex.lock();
                this->cmd.push_back((struct streamnibble) { .type = OP_BEGINRENDERPASS, .renderpass = {
                    .rpass = renderpass,
                    .fb = framebuffer,
                    .area = area,
                    .clear = clear
                } });
                this->mutex.unlock();
            }

            // End a renderpass.
            void endrenderpass(void) {
                this->mutex.lock();
                this->cmd.push_back((struct streamnibble) { .type = OP_ENDRENDERPASS });
                this->mutex.unlock();
            }

            // Set the current pipeline state.
            void setpipelinestate(struct pipelinestate pipeline) {
                this->mutex.lock();
                this->cmd.push_back((struct streamnibble) { .type = OP_SETPIPELINESTATE, .pipeline = pipeline });
                this->mutex.unlock();
            }

            // Set the current index buffer for use before a draw call.
            void setidxbuffer(struct buffer buffer, size_t offset, bool index32) {
                this->mutex.lock();
                this->cmd.push_back((struct streamnibble) { .type = OP_SETIDXBUFFER, .idxbuffer = {
                    .buffer = buffer,
                    .offset = offset,
                    .index32 = false,
                } });
                this->mutex.unlock();
            }

            // Set the current vertex buffers for use before a draw call.
            void setvtxbuffers(struct buffer *buffers, size_t *offsets, size_t firstbind, size_t bindcount) {
                this->mutex.lock();
                this->cmd.push_back((struct streamnibble) { .type = OP_SETVTXBUFFERS, .vtxbuffers = {
                    .buffers = this->allocfor(buffers, bindcount),
                    .offsets = this->allocfor(offsets, bindcount),
                    .firstbind = firstbind,
                    .bindcount = bindcount
                } });
                this->mutex.unlock();
            }

            // Draw unindexed vertices.
            void draw(size_t vtxcount, size_t instancecount, size_t firstvtx, size_t firstinstance) {
                this->mutex.lock();
                this->cmd.push_back((struct streamnibble) { .type = OP_DRAW, .draw = {
                    .vtxcount = vtxcount,
                    .instancecount = instancecount,
                    .firstvtx = firstvtx,
                    .firstinstance = firstinstance
                } });
                this->mutex.unlock();
            }

            // Draw indexed vertices.
            void drawindexed(size_t idxcount, size_t instancecount, size_t firstidx, size_t vtxoffset, size_t firstinstance) {
                this->mutex.lock();
                this->cmd.push_back((struct streamnibble) { .type = OP_DRAWINDEXED, .drawindexed = {
                    .idxcount = idxcount,
                    .instancecount = instancecount,
                    .firstidx = firstidx,
                    .vtxoffset = vtxoffset,
                    .firstinstance = firstinstance
                } });
                this->mutex.unlock();
            }

            // Commit currently set pipeline resources.
            void commitresources(void) {
                this->mutex.lock();
                this->cmd.push_back((struct streamnibble) { .type = OP_COMMITRESOURCES });
                this->mutex.unlock();
            }

            // Bind a pipeline resource to a binding.
            void bindresource(size_t binding, struct buffer buffer, size_t type) {
                this->mutex.lock();
                this->cmd.push_back((struct streamnibble) { .type = OP_BINDRESOURCE, .resource = { .binding = binding, .type = type, .buffer = buffer } });
                this->mutex.unlock();
            }

            // Transition a texture between layouts.
            void transitionlayout(struct texture texture, size_t format, size_t state) {
                this->mutex.lock();
                this->cmd.push_back((struct streamnibble) { .type = OP_TRANSITIONLAYOUT, .layout = {
                    .texture = texture, .format = format, .state = state
                } });
                this->mutex.unlock();
            }

            // Submit another stream's command list to the current command list (the result will be as if the commands were submitted to this current stream)
            void submitstream(Stream *stream) {
                this->mutex.lock();
                stream->mutex.lock();
                for (auto it = stream->cmd.begin(); it != stream->cmd.end(); it++) {
                    if (it->type == OP_SETVTXBUFFERS) {
                        this->cmd.push_back((struct streamnibble) { .type = OP_SETVTXBUFFERS, .vtxbuffers = {
                            .buffers = this->allocfor(it->vtxbuffers.buffers, it->vtxbuffers.bindcount),
                            .offsets = this->allocfor(it->vtxbuffers.offsets, it->vtxbuffers.bindcount),
                            .firstbind = it->vtxbuffers.firstbind,
                            .bindcount = it->vtxbuffers.bindcount
                        } });
                    } else {
                        this->cmd.push_back(*it);
                    }
                }

                stream->mutex.unlock();
                this->mutex.unlock();
            }
    };

    // add with value
    // OMICRON_EXPORT void renderer_addlocalstorage(struct renderer_localstorage *local, uint32_t key, void *value);
    // find and replace
    // OMICRON_EXPORT void renderer_setlocalstorage(struct renderer_localstorage *local, uint32_t key, void *value);
    // OMICRON_EXPORT void *renderer_getlocalstorage(struct renderer_localstorage *local, uint32_t key);

    // OMICRON_EXPORT struct game_object **renderer_visible(struct renderer *renderer);

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
