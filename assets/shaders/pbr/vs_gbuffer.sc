$input a_position, a_texcoord0, a_normal, a_tangent, a_bitangent
$output v_position, v_texcoord0, v_tbn

#include <bgfx_shader.sh>
#include "../shaderlib.sh"

uniform mat4 u_normaltransform;

void main() {
    v_position = mul(u_model[0], vec4(a_position, 1.0)).xyz;
    v_texcoord0 = a_texcoord0;

    vec3 T = normalize(mul(u_model[0], vec4(a_tangent, 0.0)).xyz);
    vec3 N = normalize(mul(u_normaltransform, vec4(a_normal, 0.0)).xyz);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    v_tbn = mtxFromCols(T, B, N);

    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
}
