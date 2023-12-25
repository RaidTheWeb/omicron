
$input a_position

#include <bgfx_shader.sh>
#include "../shaderlib.sh"

uniform mat4 u_lightspacemtx;

void main() {
    gl_Position = mul(mul(u_lightspacemtx, u_model[0]), vec4(a_position, 1.0));
    // gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
}
