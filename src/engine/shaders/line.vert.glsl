// OMICRON_VERTEX
#version 460

layout(location = 0) out vec4 v_colour;

layout(binding = 0) uniform ubo {
    mat4 vp;
    float time;
} u_ubo;

struct vertexdata {
    vec3 pos;
    vec4 colour;
};

layout (binding = 1) readonly buffer ssbo {
    vertexdata u_vertexdata[];
};

void main() {
    vertexdata v = u_vertexdata[gl_VertexIndex];
    gl_Position = u_ubo.vp * vec4(v.pos, 1.0);
    v_colour = v.colour;
}
