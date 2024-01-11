// OMICRON_FRAGMENT
#version 430

layout(location = 0) in vec2 v_texcoord;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_tangent;
layout(location = 3) in vec3 v_bitangent;

layout(location = 0) out vec4 f_outcolour;

layout(binding = 1) uniform sampler2D s_texture;

void main() {
   f_outcolour = vec4(texture(s_texture, v_texcoord).rgb * 1.0, 1.0);
    // f_outcolour = vec4(v_colour, 1.0);
}
