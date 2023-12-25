#version 430

layout(location = 0) in vec3 v_colour;
layout(location = 0) out vec4 f_outcolour;

void main() {
    f_outcolour = vec4(v_colour, 1.0);
}
