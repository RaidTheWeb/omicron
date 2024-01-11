#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <renderer/mesh.hpp>
#include <stdlib.h>

namespace ORenderer {

    Mesh::Mesh(aiMesh *mesh, const aiScene *scene) {
        for (size_t i = 0; i < mesh->mNumVertices; i++) {
            struct Mesh::vertex vertex;
            vertex.pos = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            if (mesh->mTangents != NULL) {
                vertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            }
            if (mesh->mBitangents != NULL) {
                vertex.bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
            }
            if (mesh->mTextureCoords[0] != NULL) {
                vertex.texcoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            } else {
                vertex.texcoord = glm::vec2(0.0f, 0.0f);
            }
            this->vertices.push_back(vertex);
        }

        for (size_t i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            ASSERT(face.mNumIndices == 3, "Degenerate triangle of %u indices.\n", face.mNumIndices);
            this->indices.push_back(face.mIndices[0]);
            this->indices.push_back(face.mIndices[1]);
            this->indices.push_back(face.mIndices[2]);
        }

        const size_t indicessize = this->indices.size() * sizeof(uint16_t);
        const size_t verticessize = this->vertices.size() * sizeof(struct vertex);

        struct ORenderer::bufferdesc bufferdesc = { };
        bufferdesc.size = verticessize;
        bufferdesc.usage = ORenderer::BUFFER_VERTEX | ORenderer::BUFFER_TRANSFERDST;
        bufferdesc.memprops = ORenderer::MEMPROP_GPULOCAL;
        ASSERT(ORenderer::context->createbuffer(&bufferdesc, &this->vertexbuffer) == ORenderer::RESULT_SUCCESS, "Failed to create buffer.\n");

        struct ORenderer::bufferdesc stagingdesc = { };
        stagingdesc.size = verticessize;
        stagingdesc.usage = ORenderer::BUFFER_TRANSFERSRC;
        stagingdesc.memprops = ORenderer::MEMPROP_CPUVISIBLE | ORenderer::MEMPROP_CPUCOHERENT | ORenderer::MEMPROP_CPUSEQUENTIALWRITE;
        stagingdesc.flags = 0;
        struct ORenderer::buffer staging = { };
        ASSERT(ORenderer::context->createbuffer(&stagingdesc, &staging) == ORenderer::RESULT_SUCCESS, "Failed to create staging buffer.\n");

        struct ORenderer::buffermapdesc stagingmapdesc = { };
        stagingmapdesc.buffer = staging;
        stagingmapdesc.size = verticessize;
        stagingmapdesc.offset = 0;
        struct ORenderer::buffermap stagingmap = { };
        ASSERT(ORenderer::context->mapbuffer(&stagingmapdesc, &stagingmap) == ORenderer::RESULT_SUCCESS, "Failed to map staging buffer.\n");
        memcpy(stagingmap.mapped[0], this->vertices.data(), verticessize);
        ORenderer::context->unmapbuffer(stagingmap);

        struct ORenderer::buffercopydesc buffercopy = { };
        buffercopy.src = staging;
        buffercopy.dst = this->vertexbuffer;
        buffercopy.srcoffset = 0;
        buffercopy.dstoffset = 0;
        buffercopy.size = verticessize;
        ORenderer::context->copybuffer(&buffercopy);

        ORenderer::context->destroybuffer(&staging); // destroy so we can recreate it

        bufferdesc.size = indicessize;
        bufferdesc.usage = ORenderer::BUFFER_INDEX | ORenderer::BUFFER_TRANSFERDST;
        bufferdesc.memprops = ORenderer::MEMPROP_GPULOCAL;
        ASSERT(ORenderer::context->createbuffer(&bufferdesc, &this->indexbuffer) == ORenderer::RESULT_SUCCESS, "Failed to create buffer.\n");

        stagingdesc.size = indicessize;
        stagingdesc.usage = ORenderer::BUFFER_TRANSFERSRC;
        stagingdesc.memprops = ORenderer::MEMPROP_CPUVISIBLE | ORenderer::MEMPROP_CPUCOHERENT | ORenderer::MEMPROP_CPUSEQUENTIALWRITE;
        stagingdesc.flags = 0;
        ASSERT(ORenderer::context->createbuffer(&stagingdesc, &staging) == ORenderer::RESULT_SUCCESS, "Failed to create staging buffer.\n");

        stagingmapdesc.buffer = staging;
        stagingmapdesc.size = indicessize;
        stagingmapdesc.offset = 0;
        ASSERT(ORenderer::context->mapbuffer(&stagingmapdesc, &stagingmap) == ORenderer::RESULT_SUCCESS, "Failed to map staging buffer.\n");
        memcpy(stagingmap.mapped[0], this->indices.data(), indicessize);
        ORenderer::context->unmapbuffer(stagingmap);

        buffercopy.src = staging;
        buffercopy.dst = this->indexbuffer;
        buffercopy.srcoffset = 0;
        buffercopy.dstoffset = 0;
        buffercopy.size = indicessize;
        ORenderer::context->copybuffer(&buffercopy);

        ORenderer::context->destroybuffer(&staging);
    }

    static const aiScene *loadfromres(OResource::Resource *resource, uint32_t flags) {
        const uint32_t defaultflags =
            aiProcess_CalcTangentSpace |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ImproveCacheLocality |
            aiProcess_LimitBoneWeights |
            aiProcess_RemoveRedundantMaterials |
            aiProcess_SplitLargeMeshes |
            aiProcess_Triangulate |
            aiProcess_GenUVCoords |
            aiProcess_SortByPType |
            aiProcess_FindInstances |
            aiProcess_ValidateDataStructure |
            aiProcess_FindInvalidData;

        const aiScene *scene = NULL;

        switch (resource->type) {
            case OResource::Resource::SOURCE_RPAK: {
                struct OResource::RPak::tableentry entry = resource->rpakentry;
                uint8_t *buffer = (uint8_t *)malloc(entry.uncompressedsize);
                ASSERT(resource->rpak->read(resource->path, buffer, entry.uncompressedsize, 0) > 0, "Failed to read RPak file.\n");
                scene = aiImportFileFromMemory((const char *)buffer, entry.uncompressedsize, defaultflags | ((flags & ORenderer::Model::IMPORT_OPTIMISE) ? aiProcess_OptimizeMeshes : 0), "");
                break;
            }
            case OResource::Resource::SOURCE_OSFS: {
                scene = aiImportFile(resource->path, defaultflags | ((flags & ORenderer::Model::IMPORT_OPTIMISE) ? aiProcess_OptimizeMeshes : 0));
                break;
            }
            default:
                ASSERT(false, "Invalid resource source type for model load.\n");
                return NULL;
        }

        ASSERT(scene != NULL && !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && scene->mRootNode != NULL, "Failed to import model with assimp error: `%s`.\n", aiGetErrorString());
        return scene;
    }

    void processnode(Model *model, const aiScene *scene, aiNode *node) {
        for (size_t i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            model->meshes.push_back(Mesh(mesh, scene));
        }

        for (size_t i = 0; i < node->mNumChildren; i++) {
            processnode(model, scene, node->mChildren[i]);
        }
    }

    Model::Model(const char *path, uint32_t flags) {
        const aiScene *scene = loadfromres(OResource::manager.get(path), flags);
        processnode(this, scene, scene->mRootNode);
        aiReleaseImport(scene);
    }
}
