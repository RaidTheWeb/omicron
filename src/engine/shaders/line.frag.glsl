// OMICRON_FRAGMENT
#version 460

layout(location = 0) in vec4 v_colour;

layout(location = 0) out vec4 f_outcolour;

void main() {
    f_outcolour = v_colour;
}
