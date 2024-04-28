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

        struct ORenderer::bufferdesc bufferdesc = { };
        bufferdesc.size = verticessize;
        bufferdesc.usage = ORenderer::BUFFER_VERTEX | ORenderer::BUFFER_TRANSFERDST;
        bufferdesc.memprops = ORenderer::MEMPROP_GPULOCAL;
        ASSERT(ORenderer::context->createbuffer(&bufferdesc, &this->vertexbuffer) == ORenderer::RESULT_SUCCESS, "Failed to create buffer.\n");

        // struct ORenderer::bufferdesc stagingdesc = { };
        // stagingdesc.size = verticessize;
        // stagingdesc.usage = ORenderer::BUFFER_TRANSFERSRC;
        // stagingdesc.memprops = ORenderer::MEMPROP_CPUVISIBLE | ORenderer::MEMPROP_CPUCOHERENT | ORenderer::MEMPROP_CPUSEQUENTIALWRITE;
        // stagingdesc.flags = 0;
        // struct ORenderer::buffer staging = { };
        // ASSERT(ORenderer::context->createbuffer(&stagingdesc, &staging) == ORenderer::RESULT_SUCCESS, "Failed to create staging buffer.\n");
        //
        // struct ORenderer::buffermapdesc stagingmapdesc = { };
        // stagingmapdesc.buffer = staging;
        // stagingmapdesc.size = verticessize;
        // stagingmapdesc.offset = 0;
        // struct ORenderer::buffermap stagingmap = { };
        // ASSERT(ORenderer::context->mapbuffer(&stagingmapdesc, &stagingmap) == ORenderer::RESULT_SUCCESS, "Failed to map staging buffer.\n");
        // memcpy(stagingmap.mapped[0], this->vertices.data(), verticessize);
        // ORenderer::context->unmapbuffer(stagingmap);
        //
        // struct ORenderer::buffercopydesc buffercopy = { };
        // buffercopy.src = staging;
        // buffercopy.dst = this->vertexbuffer;
        // buffercopy.srcoffset = 0;
        // buffercopy.dstoffset = 0;
        // buffercopy.size = verticessize;
        // ORenderer::context->copybuffer(&buffercopy);
        //
        // ORenderer::context->destroybuffer(&staging); // destroy so we can recreate it

        ORenderer::uploadtobuffer(vertexbuffer, verticessize, this->vertices.data(), 0);

        bufferdesc.size = indicessize;
        bufferdesc.usage = ORenderer::BUFFER_INDEX | ORenderer::BUFFER_TRANSFERDST;
        bufferdesc.memprops = ORenderer::MEMPROP_GPULOCAL;
        ASSERT(ORenderer::context->createbuffer(&bufferdesc, &this->indexbuffer) == ORenderer::RESULT_SUCCESS, "Failed to create buffer.\n");

        // stagingdesc.size = indicessize;
        // stagingdesc.usage = ORenderer::BUFFER_TRANSFERSRC;
        // stagingdesc.memprops = ORenderer::MEMPROP_CPUVISIBLE | ORenderer::MEMPROP_CPUCOHERENT | ORenderer::MEMPROP_CPUSEQUENTIALWRITE;
        // stagingdesc.flags = 0;
        // ASSERT(ORenderer::context->createbuffer(&stagingdesc, &staging) == ORenderer::RESULT_SUCCESS, "Failed to create staging buffer.\n");
        //
        // stagingmapdesc.buffer = staging;
        // stagingmapdesc.size = indicessize;
        // stagingmapdesc.offset = 0;
        // ASSERT(ORenderer::context->mapbuffer(&stagingmapdesc, &stagingmap) == ORenderer::RESULT_SUCCESS, "Failed to map staging buffer.\n");
        // memcpy(stagingmap.mapped[0], this->indices.data(), indicessize);
        // ORenderer::context->unmapbuffer(stagingmap);
        //
        // buffercopy.src = staging;
        // buffercopy.dst = this->indexbuffer;
        // buffercopy.srcoffset = 0;
        // buffercopy.dstoffset = 0;
        // buffercopy.size = indicessize;
        // ORenderer::context->copybuffer(&buffercopy);
        //
        // ORenderer::context->destroybuffer(&staging);
        
        ORenderer::uploadtobuffer(indexbuffer, indicessize, this->indices.data(), 0);

        this->material.basefactor = material->basefactor;
        this->material.emissivefactor = material->emissivefactor;
        this->material.metallicfactor = material->metallicfactor;
        this->material.roughnessfactor = material->roughnessfactor;

        // Might me a good idea to try refer to a previously created texture rather than load it brand new every time (if the material stays the same)
        this->material.base.texture = OResource::Texture::load(material->base); // XXX: Use information from texture to set up view data.
        struct ORenderer::textureviewdesc viewdesc = { };
        viewdesc.texture = this->material.base.texture;
        viewdesc.type = ORenderer::IMAGETYPE_2D;
        viewdesc.aspect = ORenderer::ASPECT_COLOUR;
        viewdesc.format = ORenderer::FORMAT_RGBA8SRGB;
        viewdesc.mipcount = 1;
        viewdesc.baselayer = 0;
        viewdesc.layercount = 1;
        viewdesc.basemiplevel = 0;
        ORenderer::context->createtextureview(&viewdesc, &this->material.base.view);
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
