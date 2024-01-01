#ifndef _ENGINE__MODEL_HPP
#define _ENGINE__MODEL_HPP

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <engine/mesh.hpp>
#include <renderer/camera.hpp>
#include <renderer/renderer.hpp>
#include <utils.hpp>

struct model {
    VECTOR_TYPE(struct mesh *) meshes;
    bool swaphanded;
    bool pbr;
    const char *directory;
};

void model_loadmodel(struct model *model, const char *path);
struct model *model_init(const char *path, bool swaphanded, bool pbr);
void model_processnode(struct model *model, struct aiNode *node, const struct aiScene *scene);
struct mesh *model_processmesh(struct model *model, struct aiMesh *mesh, const struct aiScene *scene);
void model_loadmaterials(struct model *model, const struct aiScene *scene, struct aiMaterial *matrial, enum aiTextureType type, const char *name, mesh_textures_t *textures);

#endif
