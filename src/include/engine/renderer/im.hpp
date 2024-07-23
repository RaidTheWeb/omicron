#ifndef _ENGINE__RENDERER__IM_HPP
#define _ENGINE__RENDERER__IM_HPP

#include <engine/renderer/bindless.hpp>
#include <engine/renderer/camera.hpp>
#include <engine/renderer/iconsfontawesome.h>
#include <engine/renderer/renderer.hpp>
#include <imgui.h>

namespace ORenderer {

    class ImCanvas {
        public:
            struct vertexdata {
                alignas(16) glm::vec3 position;
                alignas(16) glm::vec4 colour;
            };

            struct ubo {
                alignas(16) glm::mat4 mvp;
                alignas(4) float time;
            };

            struct ubo data;

            static const size_t maxlines = 65536;
            static const size_t maxlinessize = 2 * maxlines * sizeof(struct vertexdata);
            static const size_t uploadchunksize = 65536; // Upload our data in chunks of 64KB because otherwise the upload's we'd have to do would be massive.

            // XXX: INEXCUSABLE BEHAVIOUR, use a faster method of storing this data (it is okay for now since debug lines aren't aimed at release speeds anyway).
            std::vector<struct vertexdata> vertices;
            struct buffer buffer; // Geometry buffer.
            struct buffermap buffermap; // Buffer map.

            // Keep a persistent staging buffer for speed (creating and destroying buffers every frame is super expensive, even if we use a preexisting vulkan memory allocation for it).

            struct pipelinestate state;
            Shader stages[2];
            struct renderpass rpass;

            ImCanvas(void) { }
            void init(void);
            void fillstream(Stream *stream);
            void updatebuffer(void);
            void updateuniform(glm::mat4 mvp, float time);
            void clear(void);
            void line(glm::vec3 p1, glm::vec3 p2, glm::vec4 colour);
            void plane3d(glm::vec3 origin, glm::vec3 v1, glm::vec3 v2, int n1, int n2, float s1, float s2, glm::vec4 colour, glm::vec4 outline);
            void drawbox(glm::mat4 model, glm::vec3 size, glm::vec4 colour, glm::vec3 centre);
            void drawbox(glm::mat4 model, OMath::AABB bounds, glm::vec4 colour);
            void drawfrustum(glm::mat4 view, glm::mat4 proj, glm::vec4 colour);
    };

    class ImGuiRenderer {
        public:
            size_t buffersize;
            struct buffer ssbo;
            struct buffermap ssbomap;

            struct sampler fontsampler;
            struct texture font;
            ImFont *imfont[2];
            struct textureview fontview;

            struct buffer ssbostaging;
            struct buffermap ssbostagingmap;

            struct pipelinestate state;
            Shader stages[2];
            struct renderpass rpass;

            struct ubo {
                glm::mat4 vp;
            };

            struct pushconstants {
                uint64_t vtx;
                uint64_t idx;
                glm::mat4 vp;
                uint32_t sampler;
                uint32_t texture;
            };

            struct pushconstants pcs;

            struct ubo ubo;

            const ImDrawData *drawdata = NULL;
            static const uint32_t vtxbuffersize = 64 * 1024 * sizeof(ImDrawVert);
            static const uint32_t idxbuffersize = 64 * 1024 * sizeof(uint32_t);
            static const size_t uploadchunksize = 65536;

            uint32_t samplerid;

            void init(void);
            void drawcmd(uint32_t width, uint32_t height, Stream *stream, const ImDrawCmd *cmd, ImVec2 clipoff, ImVec2 clipscale, size_t idxoff, size_t vtxoff);
            void fillstream(Stream *stream);
            void updatebuffer(void);
    };
};

#endif
