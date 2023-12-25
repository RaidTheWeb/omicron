$input a_position, a_texcoord0
$output v_dir

#include <bgfx_shader.sh>

uniform vec4 u_skyboxparams[5];

void main() {
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
#if BGFX_SHADER_LANGUAGE_SPIRV // vulkan
    gl_Position.y *= -1;
#endif
    gl_Position.z = 1.0;

    float height = tan(radians(u_skyboxparams[0].x) * 0.5);
    float aspect = height * (u_viewRect.z / u_viewRect.w);

    vec2 tex = (2.0 * a_texcoord0 - 1.0) * vec2(aspect, height);

    mat4 mtx;
    mtx[0] = u_skyboxparams[1];
    mtx[1] = u_skyboxparams[2];
    mtx[2] = u_skyboxparams[3];
    mtx[3] = u_skyboxparams[4];
    v_dir = instMul(mtx, vec4(tex, 1.0, 0.0)).xyz;
}

/* vim: set sw=4 ts=4 expandtab: */
