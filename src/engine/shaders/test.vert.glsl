// OMICRON_VERTEX
#version 430

// layout(binding = 0) uniform ubo {
//     mat4 model;
//     mat4 view;
//     mat4 proj;
//     mat4 viewproj;
//     vec3 pos;
// } u_ubo;

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

layout(push_constant, scalar) uniform constants {
    uint samplerid;
    uint baseid;
    uint normalid;
    uint mrid;
    mat4 model;
    mat4 viewproj;
    vec3 pos;
} pcs;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texcoord;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec3 a_tangent;
layout(location = 4) in vec3 a_bitangent;

layout(location = 0) out vec2 v_texcoord;
layout(location = 1) out mat3 v_tbn;
layout(location = 4) out vec3 v_position;
layout(location = 5) out vec3 v_normal;

void main() {
    gl_Position = pcs.viewproj * pcs.model * vec4(a_position, 1.0);
    v_position = (pcs.model * vec4(a_position, 1.0)).xyz;
    // gl_Position = vec4(a_position, 1.0);
    v_texcoord = a_texcoord;
    v_normal = a_normal;
    // v_tangent = a_tangent;
    // v_bitangent = a_bitangent;
    // mat4 normal = transpose(inverse(pcs.model));
    mat3 normal = transpose(inverse(mat3(pcs.model)));
    // vec3 T = normalize((normal * vec4(a_tangent, 0.0)).xyz);
    // vec3 N = normalize((normal * vec4(a_normal, 0.0)).xyz);
    vec3 T = normalize(normal * a_tangent);
    vec3 N = normalize(normal * a_normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = normalize(normal * a_bitangent);
    // vec3 B = normalize((normal * vec4(a_bitangent, 0.0)).xyz);
    // vec3 B = cross(N, T);
    v_tbn = mat3(T, B, N);
}
