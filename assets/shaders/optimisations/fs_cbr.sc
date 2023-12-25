$input v_texcoord0

#include <bgfx_shader.sh>
#include "../shaderlib.sh"

SAMPLER2D(s_cbr0, 0);
SAMPLER2D(s_cbr1, 1);

void main() {
    // ivec2 coords = ivec2(floor(gl_FragCoord.xy));
    // ivec2 halfcoords = coords >> 1;

    // gl_FragColor = vec4(colour, 1.0);
}
