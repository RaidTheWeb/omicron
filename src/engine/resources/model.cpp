#include <assimp/cimport.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <engine/resources/model.hpp>
#include <engine/resources/resource.hpp>
#include <engine/resources/rpak.hpp>
#include <engine/utils/print.hpp>

namespace OResource {

    Model::Model(const char *path) {
        OUtils::Handle<Resource> res = manager.get(path);

        if (res->type == Resource::SOURCE_OSFS) {
            FILE *f = fopen(res->path, "r");
            ASSERT(f != NULL, "Failed to open OMod file.\n");
            fseek(f, 0, SEEK_END);
            size_t size = ftell(f);
            fseek(f, 0, SEEK_SET);
            size_t reqsize = size; // required size

            ASSERT(reqsize >= sizeof(struct header), "OMod file is not large enough to accomodate for at least the header size.\n");
            reqsize -= sizeof(struct header);
            ASSERT(fread(&this->header, sizeof(struct header), 1, f) > 0, "Failed to read OMod file header.\n");
            ASSERT(!strncmp(this->header.magic, "OMOD", sizeof(this->header.magic)), "Invalid OMod file magic.\n");

            OUtils::print("Model file header:\n\tMagic: %s\n\tMesh count: %u\n\tMaterial count: %u\n", this->header.magic, this->header.nummesh, this->header.nummaterial);
            ASSERT(reqsize >= (sizeof(struct material) * this->header.nummaterial), "OMod file is not large enough to accomodate for the specified number of materials.\n");
            this->materials = (struct material *)malloc(sizeof(struct material) * this->header.nummaterial);
            ASSERT(this->materials != NULL, "Failed to allocate memory for model materials.\n");
            ASSERT(fread(
                this->materials, sizeof(struct material) * this->header.nummaterial,
                1, f) > 0, "Failed to read OMod file materials.\n"
            );
            reqsize -= sizeof(struct material) * this->header.nummaterial;

            ASSERT(reqsize >= (sizeof(struct meshhdr) * this->header.nummesh), "OMod file is not large enough to accomodate for the specified number of mesh headers.\n");

            struct meshhdr *meshheaders = (struct meshhdr *)malloc(sizeof(struct meshhdr) * this->header.nummesh);
            ASSERT(meshheaders != NULL, "Failed to allocate memory for model mesh headers.\n");
            ASSERT(fread(
                meshheaders, sizeof(struct meshhdr) * this->header.nummesh,
                1, f) > 0, "Failed to read OMod file mesh headers.\n"
            );
            OUtils::print("Material ID %u, Vertices: %u, Indices: %u, Offset: 0x%lx\n", meshheaders[0].material, meshheaders[0].vertexcount, meshheaders[0].indexcount, meshheaders[0].offset);

            size_t totalrequired = 0;
            for (size_t i = 0; i < this->header.nummesh; i++) {
                ASSERT(meshheaders[i].material <= this->header.nummaterial, "Invalid material ID in mesh %lu for OMod file\n.", i);
                totalrequired += (sizeof(struct mesh::vertex) * meshheaders[i].vertexcount) + (sizeof(uint16_t) * meshheaders[i].indexcount);
            }
            ASSERT(reqsize >= totalrequired, "OMod file is not large enough to accomodate for all described meshes.\n");

            this->meshes = (struct mesh *)malloc(sizeof(struct mesh) * this->header.nummesh);
            ASSERT(this->meshes != NULL, "Failed to allocate memory for model meshes.\n");
            for (size_t i = 0; i < this->header.nummesh; i++) {
                this->meshes[i].header = meshheaders[i];
                this->bounds.merge(OMath::AABB(meshheaders[i].bmin, meshheaders[i].bmax));
                this->meshes[i].vertices = (struct mesh::vertex *)malloc(sizeof(struct mesh::vertex) * this->meshes[i].header.vertexcount);
                ASSERT(this->meshes[i].vertices != NULL, "Failed to allocate memory for mesh vertices.\n");
                this->meshes[i].indices = (uint16_t *)malloc(sizeof(uint16_t) * this->meshes[i].header.indexcount);
                ASSERT(this->meshes[i].indices != NULL, "Failed to allocate memory for mesh indices.\n");

                ASSERT(!fseek(f, this->meshes[i].header.offset, SEEK_SET), "Failed to seek mesh data offset.\n");
                ASSERT(fread(
                    this->meshes[i].vertices, sizeof(struct mesh::vertex) * this->meshes[i].header.vertexcount,
                    1, f) > 0,
                    "Failed to read OMod file mesh %lu vertices.\n", i
                );
                ASSERT(fread(
                    this->meshes[i].indices, sizeof(uint16_t) * this->meshes[i].header.indexcount,
                    1, f) > 0,
                    "Failed to read OMod file mesh %lu indices.\n", i
                );
            }
            free(meshheaders);
            fclose(f);
        } else if (res->type == Resource::SOURCE_RPAK) {
            struct RPak::tableentry entry = res->rpakentry;
            RPak *rpak = res->rpak;
            size_t reqsize = entry.uncompressedsize; // required size

            ASSERT(reqsize >= sizeof(struct header), "OMod file is not large enough to accomodate for at least the header size.\n");
            reqsize -= sizeof(struct header);
            ASSERT(rpak->read(res->path, &this->header, sizeof(struct header), 0) > 0, "Failed to read OMod file header from RPak.\n");
            ASSERT(!strncmp(this->header.magic, "OMOD", sizeof(this->header.magic)), "Invalid OMod file magic.\n");

            OUtils::print("Model file header:\n\tMagic: %s\n\tMesh count: %u\n\tMaterial count: %u\n", this->header.magic, this->header.nummesh, this->header.nummaterial);
            ASSERT(reqsize >= (sizeof(struct material) * this->header.nummaterial), "OMod file is not large enough to accomodate for the specified number of materials.\n");
            this->materials = (struct material *)malloc(sizeof(struct material) * this->header.nummaterial);
            ASSERT(this->materials != NULL, "Failed to allocate memory for model materials.\n");
            ASSERT(rpak->read(
                res->path, this->materials, sizeof(struct material) * this->header.nummaterial,
                sizeof(struct header)) > 0, "Failed to read OMod file materials from RPak.\n"
            );
            reqsize -= sizeof(struct material) * this->header.nummaterial;

            ASSERT(reqsize >= (sizeof(struct meshhdr) * this->header.nummesh), "OMod file is not large enough to accomodate for the specified number of mesh headers.\n");

            struct meshhdr *meshheaders = (struct meshhdr *)malloc(sizeof(struct meshhdr) * this->header.nummesh);
            ASSERT(meshheaders != NULL, "Failed to allocate memory for mesh headers.\n");
            ASSERT(rpak->read(
                res->path, meshheaders, sizeof(struct meshhdr) * this->header.nummesh,
                sizeof(struct header) + (sizeof(struct material) * this->header.nummaterial)) > 0, "Failed to read OMod file mesh headers from RPak.\n"
            );
            OUtils::print("Material ID %u, Vertices: %u, Indices: %u, Offset: 0x%lx\n", meshheaders[0].material, meshheaders[0].vertexcount, meshheaders[0].indexcount, meshheaders[0].offset);

            size_t totalrequired = 0;
            for (size_t i = 0; i < this->header.nummesh; i++) {
                ASSERT(meshheaders[i].material <= this->header.nummaterial, "Invalid material ID in mesh %lu for OMod file\n.", i);
                totalrequired += (sizeof(struct mesh::vertex) * meshheaders[i].vertexcount) + (sizeof(uint16_t) * meshheaders[i].indexcount);
            }
            ASSERT(reqsize >= totalrequired, "OMod file is not large enough to accomodate for all described meshes.\n");

            this->meshes = (struct mesh *)malloc(sizeof(struct mesh) * this->header.nummesh);
            ASSERT(this->meshes != NULL, "Failed to allocate memory for model meshes.\n");
            for (size_t i = 0; i < this->header.nummesh; i++) {
                this->meshes[i].header = meshheaders[i];
                this->bounds.merge(OMath::AABB(meshheaders[i].bmin, meshheaders[i].bmax));
                this->meshes[i].vertices = (struct mesh::vertex *)malloc(sizeof(struct mesh::vertex) * this->meshes[i].header.vertexcount);
                ASSERT(this->meshes[i].vertices != NULL, "Failed to allocate memory for mesh vertices.\n");
                this->meshes[i].indices = (uint16_t *)malloc(sizeof(uint16_t) * this->meshes[i].header.indexcount);
                ASSERT(this->meshes[i].indices != NULL, "Failed to allocate memory for mesh indices.\n");

                ASSERT(rpak->read(
                    res->path, this->meshes[i].vertices, sizeof(struct mesh::vertex) * this->meshes[i].header.vertexcount,
                    this->meshes[i].header.offset) > 0,
                    "Failed to read OMod file mesh %lu vertices from RPak.\n", i
                );
                ASSERT(rpak->read(
                    res->path, this->meshes[i].indices, sizeof(uint16_t) * this->meshes[i].header.indexcount,
                    this->meshes[i].header.offset + (sizeof(struct mesh::vertex) * this->meshes[i].header.vertexcount)) > 0,
                    "Failed to read OMod file mesh %lu indices from RPak.\n", i
                );
            }
            free(meshheaders);
        } else {
            ASSERT(false, "Invalid resource source type for OMod file load.\n");
        }
    }

    static void processnode(std::vector<struct Model::mesh> *output, std::vector<struct Model::material> *materials, const aiScene *scene, aiNode *node) {
        for (size_t i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];

            struct Model::mesh omesh = { };
            omesh.vertices = (struct Model::mesh::vertex *)malloc(sizeof(struct Model::mesh::vertex) * mesh->mNumVertices);
            ASSERT(omesh.vertices != NULL, "Failed to allocate memory for mesh vertices.\n");
            omesh.header.vertexcount = mesh->mNumVertices;

            for (size_t i = 0; i < mesh->mNumVertices; i++) {
                struct Model::mesh::vertex vertex;
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

                omesh.vertices[i] = vertex;
            }

            omesh.header.indexcount = mesh->mNumFaces * 3; // Expect 3 indices (triangle)
            omesh.indices = (uint16_t *)malloc(sizeof(uint16_t) * omesh.header.indexcount);
            ASSERT(omesh.indices != NULL, "Failed to allocate memory for mesh indices.\n");

            size_t indicesidx = 0;
            for (size_t i = 0; i < mesh->mNumFaces; i++) {
                aiFace face = mesh->mFaces[i];
                ASSERT(face.mNumIndices == 3, "Degenerate triangle of %u indices.\n", mesh->mFaces[i].mNumIndices);
                omesh.indices[indicesidx++] = face.mIndices[0];
                omesh.indices[indicesidx++] = face.mIndices[1];
                omesh.indices[indicesidx++] = face.mIndices[2];
            }

            // omesh.header.material = UINT32_MAX; // Invalid material ID
            if (mesh->mMaterialIndex >= 0) {
                aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
                size_t id = materials->size();
                struct Model::material omaterial = { };
                float temp = 0.0f;
                material->Get(AI_MATKEY_BASE_COLOR, temp);
                omaterial.basefactor = temp;
                material->Get(AI_MATKEY_METALLIC_FACTOR, temp);
                omaterial.metallicfactor = temp;
                material->Get(AI_MATKEY_ROUGHNESS_FACTOR, temp);
                omaterial.roughnessfactor = temp;
                omesh.header.material = id;

                aiString str;
                if (material->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &str) == AI_SUCCESS) {
                   strncpy(omaterial.base, str.C_Str(), sizeof(omaterial.base));
                }
                if (material->GetTexture(aiTextureType_NORMALS, 0, &str) == AI_SUCCESS) {
                    strncpy(omaterial.normal, str.C_Str(), sizeof(omaterial.normal));
                }
                if (material->GetTexture(aiTextureType_METALNESS, 0, &str) == AI_SUCCESS) {
                    strncpy(omaterial.mr, str.C_Str(), sizeof(omaterial.mr));
                }
                materials->push_back(omaterial);
            }
            omesh.header.bmin = glm::vec3(mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z);
            omesh.header.bmax = glm::vec3(mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z);
            output->push_back(omesh);
        }

        for (size_t i = 0; i < node->mNumChildren; i++) {
            processnode(output, materials, scene, node->mChildren[i]);
        }
    }

    void Model::fromassimp(const char *path, const char *output) {
        ASSERT(path != NULL, "NULL input path.\n");
        ASSERT(output != NULL, "NULL output path.\n");

        const uint32_t defaultflags =
            aiProcess_CalcTangentSpace |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ImproveCacheLocality |
            aiProcess_PreTransformVertices |
            aiProcess_LimitBoneWeights |
            aiProcess_RemoveRedundantMaterials |
            aiProcess_SplitLargeMeshes |
            aiProcess_Triangulate |
            aiProcess_GenUVCoords |
            aiProcess_SortByPType |
            aiProcess_FindInstances |
            aiProcess_ValidateDataStructure |
            aiProcess_FindInvalidData |
            aiProcess_GenBoundingBoxes | // Generate AABBs per mesh
            aiProcess_OptimizeMeshes;
        const aiScene *scene = aiImportFile(path, defaultflags);
        ASSERT(scene != NULL && !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && scene->mRootNode != NULL, "Failed to import model with assimp error: `%s`.\n", aiGetErrorString());

        std::vector<struct mesh> meshes;
        std::vector<struct material> materials;
        processnode(&meshes, &materials, scene, scene->mRootNode);
        aiReleaseImport(scene);

        struct header header = { };
        strcpy(header.magic, "OMOD");
        header.nummaterial = materials.size();
        header.nummesh = meshes.size();

        FILE *f = fopen(output, "w");
        ASSERT(f != NULL, "Failed to open OMod output file.\n");

        ASSERT(fwrite(&header, sizeof(struct header), 1, f), "Failed to write OMod model header.\n");
        for (size_t i = 0; i < header.nummaterial; i++) {
            ASSERT(fwrite(&materials[i], sizeof(struct material), 1, f), "Failed to write OMod material.\n");
        }

        size_t approxoff = sizeof(struct header) + (sizeof(struct material) * header.nummaterial) + (sizeof(struct meshhdr) * header.nummesh);

        for (size_t i = 0; i < header.nummesh; i++) {
            meshes[i].header.offset = approxoff;
            ASSERT(fwrite(&meshes[i].header, sizeof(struct meshhdr), 1, f), "Failed to write OMod mesh header.\n");
            approxoff += (sizeof(struct mesh::vertex) * meshes[i].header.vertexcount) + (sizeof(uint16_t) * meshes[i].header.indexcount);
        }

        for (size_t i = 0; i < header.nummesh; i++) {
            ASSERT(fwrite(meshes[i].vertices, sizeof(struct mesh::vertex) * meshes[i].header.vertexcount, 1, f), "Failed to write OMod mesh vertex data.\n");
            ASSERT(fwrite(meshes[i].indices, sizeof(uint16_t) * meshes[i].header.indexcount, 1, f), "Failed to write OMod mesh indices data.\n");
            free(meshes[i].vertices);
            free(meshes[i].indices);
        }

        fseek(f, 0, SEEK_SET);
        fwrite(&header, sizeof(struct header), 1, f); // Update header with new info

        fclose(f);
    }

}
