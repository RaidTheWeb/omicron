$input v_spos

#include <bgfx_shader.sh>
#include "../shaderlib.sh"

SAMPLER2D(s_deferred_baserough, 0);
SAMPLER2D(s_deferred_normalmetallic, 1);
SAMPLER2D(s_deferred_emissiveocclusion, 2);
SAMPLER2D(s_deferred_depth, 3);

uniform vec4 u_campos;
uniform vec4 u_params[2];
#define u_params0 u_params[0]
#define u_params1 u_params[1]

#define u_lightpos u_params0.xyz
#define u_lightradius u_params0.w
#define u_lightcolour u_params1.rgb
#define u_lightintensity u_params1.w

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
    vec2 texcoord = gl_FragCoord.xy / u_viewRect.zw; // texture coordinate from view rectangle
    float depth = toClipSpaceDepth(texture2D(s_deferred_depth, texcoord).r);

    vec3 clip = vec3(texcoord * 2.0 - 1.0, depth);
#if !BGFX_SHADER_LANGUAGE_GLSL
    clip.y = -clip.y;
#endif

    vec3 position = clipToWorld(u_invViewProj, clip);

    vec4 baserough = texture2D(s_deferred_baserough, texcoord);
    vec4 normalmetallic = texture2D(s_deferred_normalmetallic, texcoord);

    vec3 base = baserough.rgb;
    vec3 normal = normalmetallic.rgb;
    float rough = max(baserough.a, 0.045);
    float metallic = normalmetallic.a;
    float occlusion = texture2D(s_deferred_emissiveocclusion, texcoord).a;

    vec3 viewdir = normalize(u_campos.xyz - position);
    vec3 lightdir = u_lightpos - position.xyz;
    float distance = length(lightdir);
    lightdir = lightdir / distance; // normalise

    float attenuation = u_lightintensity * falloff(distance, u_lightradius);
    vec3 light = attenuation * u_lightcolour * clampdot(normal, lightdir);

    vec3 colour = (diffuse(base, metallic) + PI * specular(lightdir, viewdir, normal, base, rough, metallic)) * light;

    gl_FragColor = vec4(colour * occlusion, 1.0);
}
