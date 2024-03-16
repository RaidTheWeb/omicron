#include <engine/renderer/envmap.hpp>
#include <engine/utils.hpp>


// XXX: This can cause crashes sometimes? (irradiance map usually shows up as incomplete and then breaks everything)
// TODO: Use an encoder instead of the default submit for draw calls?
struct envmap *envmap_initfromfile(const char *path) {
    struct envmap *envmap = (struct envmap *)malloc(sizeof(struct envmap));

    if (utils_checkktxpath(path)) {
        // size_t size = 1024;
        // // TODO: Implement a resource manager thread that can free marked resources
        // struct resource_gpu *cubemap = RESOURCE_INIT(utils_loadcubemapfromktx(path, false), GPU_TEXTURE);
        // envmap->prefiltered = RESOURCE_INIT(bgfx_create_texture_cube(size, true, 1, BGFX_TEXTURE_FORMAT_RGBA16F, BGFX_TEXTURE_COMPUTE_WRITE, NULL), GPU_TEXTURE);
        // envmap->irradiance = RESOURCE_INIT(bgfx_create_texture_cube(64, false, 1, BGFX_TEXTURE_FORMAT_RGBA16F, BGFX_TEXTURE_COMPUTE_WRITE, NULL), GPU_TEXTURE);
        // RESOURCE_MARKFREE(cubemap);
        //
        // float maxmip = log2f(size);
        // for (float miplevel = 0.0f; miplevel <= maxmip; ++miplevel) {
        //     uint16_t mipwidth = size / (uint16_t)powf(2.0f, miplevel);
        //     float rough = miplevel / maxmip;
        //     vec4s params = (vec4s) { rough, miplevel, size, 0.0f };
        //     bgfx_set_uniform(u_params->uniform, &params, 1);
        //     bgfx_set_texture(0, s_prefiltercubemap->uniform, cubemap->texture, UINT32_MAX);
        //     bgfx_set_image(1, envmap->prefiltered->texture, miplevel, BGFX_ACCESS_WRITE, BGFX_TEXTURE_FORMAT_RGBA16F);
        //     bgfx_dispatch(v_prefilter, p_prefilter->program, mipwidth / 8, mipwidth / 8, 1, BGFX_DISCARD_ALL);
        // }
        //
        // bgfx_set_texture(0, s_irradiancecubemap->uniform, cubemap->texture, UINT32_MAX);
        // bgfx_set_image(1, envmap->irradiance->texture, 0, BGFX_ACCESS_WRITE, BGFX_TEXTURE_FORMAT_RGBA16F);
        // bgfx_dispatch(v_irradiance, p_irradiance->program, 64 / 8, 64 / 8, 1, BGFX_DISCARD_ALL);
        // printf("dispatched filter compute shaders on first frame\n");
    } else if (true) {

    } else {
        free(envmap);
        // TODO: Error!
        return NULL;
    }

    return envmap;
}

void envmap_free(struct envmap *envmap) {
    // bgfx_destroy_texture(envmap->irradiance->texture);
    // bgfx_destroy_texture(envmap->prefiltered->texture);
    free(envmap);
}
