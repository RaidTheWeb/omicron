$input v_texcoord0

#include <bgfx_shader.sh>
#include "../shaderlib.sh"

SAMPLER2D(s_texture_diffuse, 0);

void main() {
    vec4 colour = texture2D(s_texture_diffuse, v_texcoord0);
    // if (colour.a < 0.1) {
        // discard; // discard fragments that are transparent
    // }
    gl_FragColor = colour;
}
