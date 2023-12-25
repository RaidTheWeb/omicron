#version 430

layout(binding = 0) uniform ubo {
    mat4 model;
    mat4 view;
    mat4 proj;
} u_ubo;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_colour;
layout(location = 2) in vec2 a_texcoord;

layout(location = 0) out vec3 v_colour;
layout(location = 1) out vec2 v_texcoord;

void main() {
    gl_Position = u_ubo.proj * u_ubo.view * u_ubo.model * vec4(a_position, 1.0);
    // gl_Position = vec4(a_position, 1.0);
    v_colour = a_colour;
    v_texcoord = a_texcoord;
}
