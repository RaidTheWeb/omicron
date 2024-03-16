#ifndef _ENGINE__RENDERER__MESH_HPP
#define _ENGINE__RENDERER__MESH_HPP

#include <assimp/scene.h>
#include <engine/renderer/material.hpp>
#include <engine/renderer/renderer.hpp>
#include <engine/resources/model.hpp>

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

            std::vector<struct vertex> vertices;
            std::vector<uint16_t> indices;
            struct buffer vertexbuffer;
            struct buffer indexbuffer;

            Material material;

            Mesh(void) { };
            Mesh(OResource::Model::mesh *mesh, OResource::Model::material *material);

            // Generate a vertex layout descriptor for the specified binding following the attributes of the mesh class' vertex structure.
            static struct ORenderer::vertexlayout getlayout(size_t binding) {
                struct ORenderer::vertexlayout layout = { };
                layout.stride = sizeof(struct vertex);
                layout.binding = binding;
                layout.attribcount = 5;
                layout.attribs[0].offset = offsetof(struct vertex, pos);
                layout.attribs[0].attribtype = ORenderer::VTXATTRIB_VEC3;
                layout.attribs[1].offset = offsetof(struct vertex, texcoord);
                layout.attribs[1].attribtype = ORenderer::VTXATTRIB_VEC2;
                layout.attribs[2].offset = offsetof(struct vertex, normal);
                layout.attribs[2].attribtype = ORenderer::VTXATTRIB_VEC3;
                layout.attribs[3].offset = offsetof(struct vertex, tangent);
                layout.attribs[3].attribtype = ORenderer::VTXATTRIB_VEC3;
                layout.attribs[4].offset = offsetof(struct vertex, bitangent);
                layout.attribs[4].attribtype = ORenderer::VTXATTRIB_VEC3;
                return layout;
            }
    };

    class Model {
        public:
            enum importflags {
                IMPORT_OPTIMISE = (1 << 0)
            };

            std::vector<Mesh> meshes;

            Model(void) { };
            Model(const char *path);
    };
}

#endif
