#ifndef _ENGINE__ENVMAP_HPP
#define _ENGINE__ENVMAP_HPP

#include <engine/math.hpp>
#include <engine/resources/resource.hpp>
#include <engine/utils.hpp>

struct envmap {
    // struct resource_gpu *irradiance;
    // struct resource_gpu *prefiltered;
};

struct envmap *envmap_initfromfile(const char *path);
void envmap_free(struct envmap *envmap);

#endif
