// OMICRON_FRAGMENT
#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 v_texcoord;
layout(location = 1) in vec4 v_colour;

layout(location = 0) out vec4 f_outcolour;

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

layout(binding = 0) uniform sampler samplers[];
layout(binding = 1) uniform texture2D textures[];

void main() {
    f_outcolour = texture(nonuniformEXT(sampler2D(textures[pcs.textureid], samplers[pcs.samplerid])), v_texcoord) * v_colour;
}
