#ifndef _ENGINE__RENDERER__BINDLESS_HPP
#define _ENGINE__RENDERER__BINDLESS_HPP

#include <engine/renderer/renderer.hpp>

namespace ORenderer {
    class BindlessManager {
        public:

            RendererContext *context;

            // XXX: Store two descriptor sets for each frame of latency?

            struct resourcesetlayout layout;
            struct resourceset set;

            static const uint32_t POOLSIZE = 1000;


            struct poolid {
                uint32_t id;
                struct poolid *next;
            };

            OJob::Spinlock spin;

            struct poolid samplerids[POOLSIZE];
            struct poolid textureids[POOLSIZE];

            struct poolid *freesampler;
            struct poolid *freetexture;

            BindlessManager(void) { }

            void init(RendererContext *context);
            BindlessManager(RendererContext *context) {
                this->init(context);
            }

            uint32_t registersampler(struct sampler sampler, size_t layout = LAYOUT_SHADERRO);
            uint32_t registertexture(struct textureview texture, size_t layout = LAYOUT_SHADERRO);
            // Update existing texture bind in place with a new layout or view.
            void updatetexture(uint32_t id, struct textureview texture, size_t layout = LAYOUT_SHADERRO);
            void removesampler(uint32_t id);
            void removetexture(uint32_t id);
    };

    extern BindlessManager setmanager;
}

#endif
