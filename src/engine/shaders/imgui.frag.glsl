// OMICRON_FRAGMENT
#version 460

layout(location = 0) in vec2 v_texcoord;
layout(location = 1) in vec4 v_colour;

layout(location = 0) out vec4 f_outcolour;

layout(binding = 3) uniform sampler2D s_texture;

void main() {
    f_outcolour = texture(s_texture, v_texcoord) * v_colour;
}
