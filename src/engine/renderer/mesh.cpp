#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <engine/renderer/mesh.hpp>
#include <engine/resources/model.hpp>
#include <engine/resources/texture.hpp>
#include <stdlib.h>

namespace ORenderer {
    Mesh::Mesh(OResource::Model::mesh *mesh, OResource::Model::material *material) {
        for (size_t i = 0; i < mesh->header.vertexcount; i++) {
            struct Mesh::vertex vertex;
            struct OResource::Model::mesh::vertex overtex = mesh->vertices[i];
            vertex.pos = overtex.pos;
            vertex.normal = overtex.normal;
            vertex.tangent = overtex.tangent;
            vertex.bitangent = overtex.bitangent;
            vertex.texcoord = overtex.texcoord;
            this->vertices.push_back(vertex);
        }

        for (size_t i = 0; i < mesh->header.indexcount; i++) {
            this->indices.push_back(mesh->indices[i]);
        }

        const size_t indicessize = this->indices.size() * sizeof(uint16_t);
        const size_t verticessize = this->vertices.size() * sizeof(struct vertex);

        ASSERT(ORenderer::context->createbuffer(
            &this->vertexbuffer, verticessize,
            ORenderer::BUFFER_VERTEX | ORenderer::BUFFER_TRANSFERDST,
            ORenderer::MEMPROP_GPULOCAL, 0
        ) == ORenderer::RESULT_SUCCESS, "Failed to create buffer.\n");

        ORenderer::uploadtobuffer(vertexbuffer, verticessize, this->vertices.data(), 0);

        ASSERT(ORenderer::context->createbuffer(
            &this->indexbuffer, indicessize,
            ORenderer::BUFFER_INDEX | ORenderer::BUFFER_TRANSFERDST,
            ORenderer::MEMPROP_GPULOCAL, 0
        ) == ORenderer::RESULT_SUCCESS, "Failed to create buffer.\n");

        ORenderer::uploadtobuffer(indexbuffer, indicessize, this->indices.data(), 0);

        this->material.basefactor = material->basefactor;
        this->material.emissivefactor = material->emissivefactor;
        this->material.metallicfactor = material->metallicfactor;
        this->material.roughnessfactor = material->roughnessfactor;

        // Might me a good idea to try refer to a previously created texture rather than load it brand new every time (if the material stays the same)
        this->material.base.texture = OResource::Texture::load(material->base); // XXX: Use information from texture to set up view data.
        this->material.normal.texture = OResource::Texture::load(material->normal);
        this->material.mr.texture = OResource::Texture::load(material->mr);

        ASSERT(ORenderer::context->createtextureview(
            &this->material.base.view, ORenderer::FORMAT_RGBA8SRGB,
            this->material.base.texture, ORenderer::IMAGETYPE_2D,
            ORenderer::ASPECT_COLOUR, 0, 1, 0, 1
        ) == ORenderer::RESULT_SUCCESS, "Failed to create material texture view.\n");
        ASSERT(ORenderer::context->createtextureview(
            &this->material.normal.view, ORenderer::FORMAT_RGBA8SRGB,
            this->material.normal.texture, ORenderer::IMAGETYPE_2D,
            ORenderer::ASPECT_COLOUR, 0, 1, 0, 1
        ) == ORenderer::RESULT_SUCCESS, "Failed to create material texture view.\n");
        ASSERT(ORenderer::context->createtextureview(
            &this->material.mr.view, ORenderer::FORMAT_RGBA8SRGB,
            this->material.mr.texture, ORenderer::IMAGETYPE_2D,
            ORenderer::ASPECT_COLOUR, 0, 1, 0, 1
        ) == ORenderer::RESULT_SUCCESS, "Failed to create material texture view.\n");
        this->bounds = OMath::AABB(mesh->header.bmin, mesh->header.bmax);
    }

    Model::Model(const char *path) {
        OResource::Model model = OResource::Model(path);
        this->bounds = model.bounds;
        for (size_t i = 0; i < model.header.nummesh; i++) {
            this->meshes.push_back(Mesh(&model.meshes[i], &model.materials[model.meshes[i].header.material]));
        }
    }
}
