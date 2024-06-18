#include <engine/renderer/bindless.hpp>

namespace ORenderer {
    BindlessManager setmanager;

    void BindlessManager::init(RendererContext *context) {

        this->context = context;

        struct resourcedesc resources[] = {
            (struct resourcedesc) {
                .binding = 0,
                .stages = STAGE_ALL,
                .count = POOLSIZE,
                .type = RESOURCE_SAMPLER,
                .flag = RESOURCEFLAG_BINDLESS,
            },
            (struct resourcedesc) {
                .binding = 1,
                .stages = STAGE_ALL,
                .count = POOLSIZE,
                .type = RESOURCE_TEXTURE,
                .flag = RESOURCEFLAG_BINDLESS
            }
        };

        struct resourcesetdesc desc = { };
        desc.flag = RESOURCEFLAG_BINDLESS;
        desc.resourcecount = 2;
        desc.resources = resources;

        ASSERT(context->createresourcesetlayout(&desc, &this->layout) == RESULT_SUCCESS, "Failed to create resource set layout.\n");

        ASSERT(context->createresourceset(&this->layout, &this->set) == RESULT_SUCCESS, "Failed to create resource set.\n");

        this->freesampler = &this->samplerids[0];
        this->freetexture = &this->textureids[0];

        for (size_t i = 0; i < POOLSIZE - 1; i++) {
            this->samplerids[i].id = i;
            this->samplerids[i].next = &this->samplerids[i + 1];

            this->textureids[i].id = i;
            this->textureids[i].next = &this->textureids[i + 1];
        }

        this->samplerids[POOLSIZE - 1].next = NULL;
        this->textureids[POOLSIZE - 1].next = NULL;
    }

    uint32_t BindlessManager::registersampler(struct sampler sampler) {
        ASSERT(sampler.handle != RENDERER_INVALIDHANDLE, "Invalid sampler.\n");

        ASSERT(this->freesampler != NULL, "Improper set management. No more free samplers.\n");

        this->spin.lock();
        struct poolid *id = this->freesampler;
        this->freesampler = id->next;
        uint32_t ret = id->id;

        struct samplerbind bind = { };
        bind.id = ret;
        bind.layout = LAYOUT_SHADERRO;
        bind.sampler = sampler;
        this->context->updateset(this->set, &bind);

        this->spin.unlock();

        return ret;
    }

    uint32_t BindlessManager::registertexture(struct textureview texture) {
        ASSERT(this->freetexture != NULL, "Improper set management. No more free texturess.\n");

        this->spin.lock();
        struct poolid *id = this->freetexture;
        this->freetexture = id->next;
        uint32_t ret = id->id;

        struct texturebind bind = { };
        bind.id = ret;
        bind.layout = LAYOUT_SHADERRO;
        bind.view = texture;
        this->context->updateset(this->set, &bind);
        this->spin.unlock();

        return ret;
    }

    void BindlessManager::removesampler(uint32_t id) {
        ASSERT(id < POOLSIZE, "Invalid ID.\n");

        this->spin.lock();
        this->samplerids[id].next = this->freesampler;
        this->freesampler = &this->samplerids[id];
        this->spin.unlock();
    }

    void BindlessManager::removetexture(uint32_t id) {
        ASSERT(id < POOLSIZE, "Invalid ID.\n");

        this->spin.lock();
        this->textureids[id].next = this->freetexture;
        this->freetexture = &this->textureids[id];
        this->spin.unlock();
    }
}
