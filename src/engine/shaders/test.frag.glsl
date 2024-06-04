// OMICRON_FRAGMENT
#version 430

layout(location = 0) in vec2 v_texcoord;
layout(location = 1) in mat3 v_tbn;
layout(location = 4) in vec3 v_position;
layout(location = 5) in vec3 v_normal;

layout(location = 0) out vec4 f_outcolour;

layout(binding = 0) uniform ubo {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec3 pos;
} u_ubo;

layout(binding = 1) uniform sampler2D s_texture;
layout(binding = 2) uniform sampler2D s_normal;
layout(binding = 3) uniform sampler2D s_mr;

#define PI 3.141592653589793

float distributionggx(vec3 N, vec3 H, float rough) {
    float a = rough * rough;
    float a2 = a * a;

    float nDotH = max(dot(N, H), 0.0);
    float nDotH2 = nDotH * nDotH;

    float num = a2;
    float denom = (nDotH2 * (a2 - 1.0) + 1.0);
    denom = 1 / (PI * denom * denom);

    return num * denom;
}

float geometryschlickggx(float nDotV, float rough) {
    float r = (rough + 1.0);
    float k = r * r / 8.0;

    float num = nDotV;
    float denom = 1 / (nDotV * (1.0 - k) + k);

    return num * denom;
}

float geometrysmith(float nDotV, float nDotL, float rough) {
    float ggx2 = geometryschlickggx(nDotV, rough);
    float ggx1 = geometryschlickggx(nDotL, rough);

    return ggx1 * ggx2;
}

vec3 fresnelschlick(float theta, vec3 F0) {
    float val = 1.0 - theta;
    return F0 + (1.0 - F0) * (val * val * val * val * val);
}

vec3 fresnelschlickrough(float costheta, vec3 F0, float roughness) {
    float val = 1.0 - costheta;
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * (val * val * val * val * val);
}

vec3 calculatedirectional(vec3 normal, vec3 viewdir, vec3 base, float rough, float metal, vec3 F0, float shadow) {
    vec3 lightdir = normalize(-vec3(0.6f, -0.7f, -0.3f));
    vec3 halfway = normalize(lightdir + viewdir);
    float nDotV = max(dot(normal, viewdir), 0.0);
    float nDotL = max(dot(normal, lightdir), 0.0);
    vec3 colour = vec3(1.0, 1.0, 1.0);

    float NDF = distributionggx(normal, halfway, rough);
    float G = geometrysmith(nDotV, nDotL, rough);
    vec3 F = fresnelschlick(max(dot(halfway, viewdir), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metal;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * nDotV * nDotL;
    vec3 specular = numerator / max(denominator, 0.0001);

    vec3 radiance = (kD * (base / PI) + specular) * colour * nDotL;
    radiance *= (1.0 - shadow);

    return radiance;
}

vec3 sRGBToLinear(vec3 color) {
    const bvec3 cutoff = lessThan(color, vec3(0.04045));
    const vec3 higher = pow((color + 0.055) / 1.055, vec3(2.4));
    const vec3 lower = color / 12.92;
    return mix(higher, lower, cutoff);
}

void main() {

    vec4 outcolour = texture(s_texture, v_texcoord);
    if (outcolour.a < 0.9) { // threshold
        discard;
    }

    // RM
    vec2 metalrough = texture(s_mr, v_texcoord).bg;
    float metallic = metalrough.x;
    float roughness = metalrough.y;

    vec3 normal = texture(s_normal, v_texcoord).rgb * 2.0 - 1.0;
    normal = normalize(v_tbn * normal);

    vec3 viewdir = normalize(u_ubo.pos - v_position);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, outcolour.rgb, metallic);

    vec3 radiance = vec3(0.0);

    radiance = calculatedirectional(normal, viewdir, outcolour.rgb, roughness, metallic, F0, 0.0);

    vec3 ambient = vec3(0.025) * outcolour.rgb;

    ambient *= metalrough.r;

    radiance += ambient;

    f_outcolour = vec4(radiance, outcolour.a);
}
