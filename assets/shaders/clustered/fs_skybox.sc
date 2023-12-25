$input v_dir

#include <bgfx_shader.sh>
#include "../shaderlib.sh"

uniform vec4 u_skyboxparams[5];

SAMPLERCUBE(s_quadtexture, 0);

// TODO: Skybox looks like shit (doesn't look the same as what it looks like in the prefiltered env map)
void main() {
    vec3 dir = normalize(v_dir);
    gl_FragColor = (textureCubeLod(s_quadtexture, dir, 0));
    // gl_FragColor = vec4(gl_FragCoord.x, gl_FragCoord.y, 0.0, 1.0);
    // vec2 st = gl_FragCoord.xy / u_viewRect.zw;
    // st.x *= u_viewRect.z / u_viewRect.w;

    // vec3 colour = vec3_splat(0.0);
    // colour = vec3(st.x, st.y, sin(u_skyboxparams[0].y));
    // gl_FragColor = vec4(colour, 1.0);
}

/* vim: set sw=4 ts=4 expandtab: */
