#ifndef _ENGINE__MESH_HPP
#define _ENGINE__MESH_HPP

#include <assimp/scene.h>
#include <engine/math.hpp>
#include <engine/renderer/camera.hpp>
#include <engine/renderer/envmap.hpp>
#include <engine/renderer/renderer.hpp>
#include <engine/utils.hpp>

struct mesh_vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texcoords;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};

struct mesh_texture {
    uint16_t id;
    const struct aiScene *scene;
    enum aiTextureType type;
    char *path;
    bool clamp;
    bool rebase;
};

struct mesh_uniformbind {
    const char *name;
    // bgfx_uniform_handle_t handle;
};

struct mesh_texturebind {
    const char *name;
    // bgfx_texture_handle_t *handle;
};

typedef VECTOR_TYPE(struct mesh_vertex) mesh_vertices_t;
typedef VECTOR_TYPE(uint16_t) mesh_indices_t;
typedef VECTOR_TYPE(struct mesh_texture) mesh_textures_t;
typedef VECTOR_TYPE(struct mesh_texturebind) mesh_texturehandles_t;
typedef VECTOR_TYPE(struct mesh_uniformbind) mesh_uniformhandles_t;

struct mesh {
    mesh_vertices_t vertices;
    mesh_indices_t indices;
    mesh_textures_t textures;
    struct resource_gpu *texture_diffuse;
    struct resource_gpu *texture_specular;
    struct resource_gpu *texture_normal;
    struct resource_gpu *texture_base;
    struct resource_gpu *texture_metallicroughness;
    struct resource_gpu *texture_emissive;
    struct resource_gpu *texture_occlusion;
    struct resource_gpu *vbh;
    struct resource_gpu *ibh;
    // bgfx_vertex_layout_t layout;
    bool pbr;

    const char *basepath;

    struct aiAABB aabb;

    glm::vec4 material_basefactor;
    glm::vec4 material_emissivefactor;
    float material_metallicfactor;
    float material_roughnessfactor;
};

extern VECTOR_TYPE(struct mesh *) mesh_meshes;

struct mesh *mesh_init(mesh_vertices_t vertices, mesh_indices_t indices, mesh_textures_t textures, bool pbr, const char *basepath);
// void mesh_draw(struct mesh *mesh, struct resource_gpu *program, bgfx_view_id_t view, uint64_t state, glm::mat4 mtx, bool frustumcull, struct camera *camera, bool prepass, struct envmap *env);
void mesh_setup(struct mesh *mesh);
void mesh_free(struct mesh *mesh);

#endif
