
#version 430

layout(location = 0) out vec4 f_outcolour;
layout(location = 0) in vec2 v_texcoord;
layout(binding = 0) uniform sampler2D s_texture;

void main() {
    f_outcolour = vec4(texture(s_texture, v_texcoord).rgb, 1.0);
}
