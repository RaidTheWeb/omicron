

#version 430

layout(location = 0) out vec2 v_texcoord;

void main() {
    v_texcoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(v_texcoord * 2.0 - 1.0, 0.0, 1.0);
}
