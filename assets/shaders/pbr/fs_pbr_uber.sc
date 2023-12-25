// Forward PBR lighting shader

$input v_position, v_texcoord0, v_tbn

#include <bgfx_shader.sh>
#include "../shaderlib.sh"

SAMPLER2D(s_texture_base, 0);
SAMPLER2D(s_texture_normal, 1);
SAMPLER2D(s_texture_metallicroughness, 2);
SAMPLER2D(s_texture_emissive, 3);
SAMPLER2D(s_texture_occlusion, 4);

uniform vec4 u_campos;
#define MAX_LIGHTS 32
uniform vec4 u_light[6 * MAX_LIGHTS];
uniform vec4 u_lightcounts; // 0: directional, 1: point, 2: spotlight

// material specific factors (among other things)
uniform vec4 u_material[3];
#define basefactor u_material[0]
#define emissivefactor u_material[1].rgb
#define metallicfactor u_material[2].x
#define roughnessfactor u_material[2].y

#define PI 3.141592653589793

float ggx(float NoH, float roughness) {
    float alpha = roughness * roughness;
    float a = NoH * alpha;
    float k = alpha / (1.0 - NoH * NoH + a * a);
    return k * k * (1.0 / PI);
}

vec3 fresnelschlick(float VoH, float metallic, vec3 base) {
    vec3 f0 = mix(vec3_splat(0.04), base, metallic);
    float f = pow(1.0 - VoH, 5.0);
    return f + f0 * (1.0 - f);
}

float smithggx(float NoV, float NoL, float roughness) {
    float a2 = pow(roughness, 4.0);
    float ggxv = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float ggxl = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5 / (ggxv + ggxl);
}

vec3 diffuse(vec3 base, float metallic) {
    return base * (1.0 - 0.04) * (1.0 - metallic);
}

vec3 specular(vec3 lightdir, vec3 viewdir, vec3 normal, vec3 base, float roughness, float metallic) {
    vec3 h = normalize(lightdir + viewdir);
    float NoV = clamp(dot(normal, viewdir), 1e-5, 1.0);
    float NoL = clampdot(normal, lightdir);
    float NoH = clampdot(normal, h);
    float VoH = clampdot(viewdir, h);

    float D = ggx(NoH, roughness);
    vec3 F = fresnelschlick(VoH, metallic, base);
    float V = smithggx(NoV, NoL, roughness);
    return vec3(D * V * F);
}

float falloff(float distance, float radius) {
    return pow(clamp(1.0 - pow(distance / radius, 4), 0.0, 1.0), 2) / (distance * distance + 1); // radius account for distance by inverse square law
}

void main() {
    vec4 base = toLinear(texture2D(s_texture_base, v_texcoord0)) * basefactor;

    vec3 normal = texture2D(s_texture_normal, v_texcoord0).rgb;
    normal = normal * 2.0 - 1.0;
    normal = normalize(mul(v_tbn, normal));

    vec3 viewdir = normalize(u_campos.xyz - v_position);

    vec2 roughnessmetallic = texture2D(s_texture_metallicroughness, v_texcoord0).ga;
    float roughness = roughnessmetallic.x * roughnessfactor;
    float metallic = roughnessmetallic.y * metallicfactor;
    float occlusion = texture2D(s_texture_occlusion, v_texcoord0).r;
    vec3 emissive = toLinear(texture2D(s_texture_emissive, v_texcoord0)).rgb * emissivefactor;

    vec3 colour = vec3_splat(0.0);
    int off = 0;
    for (int i = 0; i < u_lightcounts.x; i++) {
        off += 6;
    }

    for (int i = 0; i < u_lightcounts.y; i++) {
        vec3 lpos = u_light[off + 0].xyz;
        float radius = u_light[off + 4].x;
        float intensity = u_light[off + 4].y;
        vec3 lcolour = u_light[off + 2].rgb;
        vec3 lightdir = lpos - v_position;
        float distance = length(lightdir);
        lightdir = lightdir / distance; // normalise (save an instruction here by not calculating length again)

        float attenuation = falloff(distance, radius) * intensity;
        if (attenuation == 0.0) {
            off += 6;
            continue; // don't bother doing lighting calculations if we exceed the radius of our light
        }

        vec3 light = attenuation * lcolour * clampdot(normal, lightdir);

        colour += (diffuse(base.rgb, metallic) + PI * specular(lightdir, viewdir, normal, base.rgb, roughness, metallic)) * light;

        off += 6;
    }

    gl_FragColor = vec4(colour * occlusion + emissive, base.a);
}
