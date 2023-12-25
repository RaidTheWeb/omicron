#ifndef _RENDERER__MATERIAL_HPP
#define _RENDERER__MATERIAL_HPP

#include <renderer/renderer.hpp>

namespace ORenderer {

    class Material {

        struct texturepair {
            struct texture texture;
            struct textureview view;
        };

        // Full PBR metallic roughness material pipeline
        struct texturepair base;
        struct texturepair normal;
        struct texturepair mr;
        struct texturepair emissive;
        struct texturepair occlusion;

        float basefactor;
        float emissivefactor;
        float metallicfactor;
        float roughnessfactor;
    };

}

#endif
