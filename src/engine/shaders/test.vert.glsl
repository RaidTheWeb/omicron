// OMICRON_VERTEX
#version 430

layout(binding = 0) uniform ubo {
    mat4 model;
    mat4 view;
    mat4 proj;
} u_ubo;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texcoord;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec3 a_tangent;
layout(location = 4) in vec3 a_bitangent;

layout(location = 0) out vec2 v_texcoord;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec3 v_tangent;
layout(location = 3) out vec3 v_bitangent;

void main() {
    gl_Position = u_ubo.proj * u_ubo.view * u_ubo.model * vec4(a_position, 1.0);
    // gl_Position = vec4(a_position, 1.0);
    v_texcoord = a_texcoord;
    v_normal = a_normal;
    v_tangent = a_tangent;
    v_bitangent = a_bitangent;
}
