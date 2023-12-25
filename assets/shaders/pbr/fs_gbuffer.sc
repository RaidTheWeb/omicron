$input v_position, v_texcoord0, v_tbn

#include <bgfx_shader.sh>
#include "../shaderlib.sh"

SAMPLER2D(s_texture_base, 0);
SAMPLER2D(s_texture_normal, 1);
SAMPLER2D(s_texture_metallicroughness, 2);
SAMPLER2D(s_texture_emissive, 3);
SAMPLER2D(s_texture_occlusion, 4);
// TODO: factors?

#if BGFX_SHADER_LANGUAGE_GLSL >= 400
// workaround because with OpenGL on BGFX this isn't automatically inserted (because the optimiser doesn't support newer OpenGL versions)
out vec4 bgfx_FragData[gl_MaxDrawBuffers];
#define gl_FragData bgfx_FragData
#endif

void main() {
    vec4 base = toLinear(texture2D(s_texture_base, v_texcoord0));
    if (base.w < 0.5) {
        discard;
        return; // prevents us from doing further calculations when we don't need to
    }

    vec3 normal = texture2D(s_texture_normal, v_texcoord0).rgb;
    normal = normal * 2.0 - 1.0;
    normal = normalize(mul(v_tbn, normal));

    vec2 roughnessmetallic = texture2D(s_texture_metallicroughness, v_texcoord0).gb;
    float roughness = roughnessmetallic.x;
    float metallic = roughnessmetallic.y;
    float occlusion = texture2D(s_texture_occlusion, v_texcoord0).r;
    vec3 emissive = toLinear(texture2D(s_texture_emissive, v_texcoord0)).rgb;

    gl_FragData[0] = vec4(base.rgb, roughness);
    gl_FragData[1] = vec4(normal, metallic);
    gl_FragData[2] = vec4(emissive, occlusion);
}
