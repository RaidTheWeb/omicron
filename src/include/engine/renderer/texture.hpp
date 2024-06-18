#ifndef _ENGINE__RENDERER__TEXTURE_HPP
#define _ENGINE__RENDERER__TEXTURE_HPP

#include <engine/renderer/renderer.hpp>

namespace ORenderer {

    // XXX: Custom streamable texture container.

    class TextureManager {
        public:
            // Texture streaming as described in 28/05/24

            // Eviction deadline of 60ms.
            static const uint64_t EVICTDEADLINE = 1000000 * 60; // Deadline for time that needs to elapse after the last reference to a texture before it is unloaded (ns).


            // create texture handle empty and let it persist, only load up the data when needed.

    };
}

#endif
