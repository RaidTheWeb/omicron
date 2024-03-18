#ifndef _ENGINE__RESOURCES__MODEL_HPP
#define _ENGINE__RESOURCES__MODEL_HPP

#include <engine/math/bounds.hpp>
#include <engine/math/math.hpp>
#include <stddef.h>
#include <stdint.h>

namespace OResource {
   
    // Header
    //      Magic (OMOD)
    //      Number of meshes
    //      Number of materials
    // Material data...
    //      Factors
    //      Texture paths
    // Mesh headers...
    //      Material ID
    //      Vertex count
    //      Index count
    //      Mesh data offset
    // Mesh data... (until file end)
    //      Vertices
    //      Indices
    
    class Model {
        public:
            struct material {
                float basefactor;
                float emissivefactor = 1.0f; // unused (assimp doesn't seem to do anything here)
                float metallicfactor;
                float roughnessfactor;
                char base[128];
                char normal[128];
                char mr[128];
                char emissive[128];
                char occlusion[128];
            } __attribute__((packed));

            struct meshhdr {
                uint32_t material; // material ID
                uint32_t vertexcount;
                uint32_t indexcount;
                glm::vec3 bmin;
                glm::vec3 bmax;

                size_t offset; // offset of vertex data followed by index data
            };

            struct mesh {
                struct meshhdr header;

                struct vertex {
                    glm::vec3 pos;
                    glm::vec2 texcoord;
                    glm::vec3 normal;
                    glm::vec3 tangent;
                    glm::vec3 bitangent;
                }; 

                struct vertex *vertices;
                uint16_t *indices;
            };

            struct header {
                char magic[5]; // OMOD\0
                uint32_t nummesh; // number of meshes
                uint32_t nummaterial; // number of materials
            };

            struct header header;
            struct mesh *meshes;
            struct material *materials;
            OMath::AABB bounds;

            Model(const char *path);
            ~Model(void) {
                for (size_t i = 0; i < this->header.nummesh; i++) {
                    free(this->meshes[i].vertices);
                    free(this->meshes[i].indices);
                }
                free(this->meshes);
                free(this->materials);
            }

            static void fromassimp(const char *path, const char *output);
    };
}

#endif
