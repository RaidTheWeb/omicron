#ifndef _ENGINE__ENVMAP_HPP
#define _ENGINE__ENVMAP_HPP

#include <bgfx/c99/bgfx.h>
#include <engine/math.hpp>
#include <resources/resource.hpp>
#include <utils.hpp>

struct envmap {
    // struct resource_gpu *irradiance;
    // struct resource_gpu *prefiltered;
};

struct envmap *envmap_initfromfile(const char *path);
struct envmap *envmap_initfromcubemap(bgfx_texture_handle_t cubemap);
void envmap_free(struct envmap *envmap);

#endif
