
$input a_position, a_texcoord0, a_normal, a_tangent, a_bitangent
$output v_position, v_texcoord0, v_tbn, v_lposition

#include <bgfx_shader.sh>
#include "../shaderlib.sh"

uniform mat4 u_lightspacemtx;

void main() {
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_position = mul(u_model[0], vec4(a_position, 1.0)).xyz; // get us a position based on the actual model (this contrasts to the screen view and rather just the actual position in the screen)
    v_lposition = mul(u_lightspacemtx, vec4(v_position, 1.0));
    v_texcoord0 = a_texcoord0;
    vec3 T = normalize(mul(u_model[0], vec4(a_tangent, 0.0)).xyz);
    vec3 N = normalize(mul(u_model[0], vec4(a_normal, 0.0)).xyz);
    T = normalize(T - dot(T, N) * N);
    // vec3 B = normalize(mul(u_model[0], vec4(a_bitangent, 0.0)).xyz);
    vec3 B = cross(N, T);
    v_tbn = mtxFromCols(T, B, N);
    // v_normal = mul(u_model[0], vec4(a_normal, 0.0)).xyz; // this should be relative to the model matrix
}
