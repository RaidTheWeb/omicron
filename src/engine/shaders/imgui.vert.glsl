// OMICRON_VERTEX
#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

layout(location = 0) out vec2 v_texcoord;
layout(location = 1) out vec4 v_colour;

struct vertexdata {
    float x, y, u, v;
    uint colour;
};

layout(std430, buffer_reference, buffer_reference_align = 8) readonly buffer VertexBuffer {
    vertexdata vertices[];
};
layout(std430, buffer_reference, buffer_reference_align = 8) readonly buffer IndexBuffer {
    uint indices[];
};

layout(push_constant, scalar) uniform constants {
    VertexBuffer vtx;
    IndexBuffer idx;
    mat4 viewproj;
    uint samplerid;
    uint textureid;
} pcs;

void main() {
    uint idx = pcs.idx.indices[gl_VertexIndex] + gl_BaseInstance;

    vertexdata v = pcs.vtx.vertices[idx];
    v_texcoord = vec2(v.u, v.v);
    v_colour = unpackUnorm4x8(v.colour);
    gl_Position = pcs.viewproj * vec4(vec2(v.x, v.y), 0.0, 1.0);
}
