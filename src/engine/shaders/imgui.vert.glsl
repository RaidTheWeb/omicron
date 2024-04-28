// OMICRON_VERTEX
#version 460

layout(location = 0) out vec2 v_texcoord;
layout(location = 1) out vec4 v_colour;

struct vertexdata {
    float x, y, u, v;
    uint colour;
};

layout(binding = 0) uniform ubo {
    mat4 viewproj;
} u_ubo;
layout(binding = 1) readonly buffer ssbo {
    vertexdata u_vertexdata[];
};
layout(binding = 2) readonly buffer ssbo2 {
    uint u_indexdata[];
};

void main() {
    uint idx = u_indexdata[gl_VertexIndex] + gl_BaseInstance;

    vertexdata v = u_vertexdata[idx];
    v_texcoord = vec2(v.u, v.v);
    v_colour = unpackUnorm4x8(v.colour);
    gl_Position = u_ubo.viewproj * vec4(vec2(v.x, v.y), 0.0, 1.0);
}
