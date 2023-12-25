#ifndef _RENDERER__MESH_HPP
#define _RENDERER__MESH_HPP

#include <renderer/material.hpp>
#include <renderer/renderer.hpp>

namespace ORenderer {
    class Mesh {
        public:
            // Generic vertex attribute
            struct vertex {
                glm::vec3 pos;
                glm::vec2 texcoord;
                glm::vec3 normal;
                glm::vec3 tangent;
                glm::vec3 bitangent;
            }; // this should be aligned

            std::vector<struct vertices> vertices;
            std::vector<uint16_t> indices;
            struct buffer vertexbuffer;
            struct buffer indexbuffer;

            struct Material material;

            Mesh(void);

            bool load(const char *path); // Load from resource system
    };
}

#endif
