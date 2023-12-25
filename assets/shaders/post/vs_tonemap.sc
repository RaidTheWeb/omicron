$input a_position, a_texcoord0
$output v_texcoord0

#include <bgfx_shader.sh>
#include "../shaderlib.sh"

void main() {
    gl_Position = vec4(a_position, 1.0); // it's positioned on screen space so it really does not matter all that much
    v_texcoord0 = a_texcoord0;
}
