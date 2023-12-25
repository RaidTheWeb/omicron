$input v_position, v_texcoord0, v_tbn, v_lsposition

#include <bgfx_shader.sh>
#include "../shaderlib.sh"

SAMPLER2D(s_texture_base, 0);
SAMPLER2D(s_texture_normal, 1);
SAMPLER2D(s_texture_metallicroughness, 2);
SAMPLER2D(s_texture_emissive, 3);
SAMPLER2D(s_texture_occlusion, 4);

// IBL (Environment maps)
SAMPLERCUBE(s_env_irradiance, 5);
SAMPLERCUBE(s_env_prefiltered, 6);

// Roughness precalculation
SAMPLER2D(s_brdflut, 7);

// material specific factors (among other things)
uniform vec4 u_material[3];
#define basefactor u_material[0]
#define emissivefactor u_material[1].rgb
#define metallicfactor u_material[2].x
#define roughnessfactor u_material[2].y

uniform vec4 u_campos;
uniform vec4 u_params[3];
#define u_params0 u_params[0]
#define u_params1 u_params[1]
#define u_params2 u_params[2]

#define scale u_params0.x
#define bias u_params0.y
#define dim u_params0.zw
#define tilesizes u_params1
#define zfar u_params2.x
#define znear u_params2.y

#define PI 3.141592653589793

float lineardepth(float depth, float near, float far) {
    float range = 2.0 * depth;
    float l = 2.0 * near * far / (far + near - depth * (far - near));
    return l;
}

vec3 colours[8] = {
    vec3(0, 0, 0), vec3(0, 0, 1), vec3(0, 1, 0), vec3(0, 1, 1),
    vec3(1, 0, 0), vec3(1, 0, 1), vec3(1, 1, 0), vec3(1, 1, 1)
};

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
    vec3 kD = vec3_splat(1.0) - kS;
    kD *= 1.0 - metal;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * nDotV * nDotL;
    vec3 specular = numerator / max(denominator, 0.0001);

    vec3 radiance = (kD * (base / PI) + specular) * colour * nDotL;
    radiance *= (1.0 - shadow);

    return radiance;
}

void main() {
    // if (uint(gl_FragCoord.x * gl_FragCoord.y) % 2 == 0) { // only render ever 2nd pixel in checkerboard pattern
        // discard;
        // return;
    // }

    // linear should always be used on textures (as they're usually in sRGB or something of the sort)
    vec4 base = toLinearAccurate(texture2D(s_texture_base, v_texcoord0)) * basefactor;
    vec3 emissive = toLinearAccurate(texture2D(s_texture_emissive, v_texcoord0)).rgb * emissivefactor;
    // vec3 emissive = vec3(0.0, 0.0, 0.0);

    float occlusion = texture2D(s_texture_occlusion, v_texcoord0).r;
    // float occlusion = 1.0;
    vec2 metalrough = texture2D(s_texture_metallicroughness, v_texcoord0).bg;
    // vec2 metalrough = vec2_splat(0.0);

    // glTF format is G for roughness and B for metallic
    // XXX: These fields aren't properly represented in metallicroughness texture somehow!
    float metallic = metalrough.x * metallicfactor;
    float roughness = metalrough.y * roughnessfactor;

    vec3 normal = texture2D(s_texture_normal, v_texcoord0).rgb * 2.0 - 1.0;
    normal = normalize(mul(v_tbn, normal));

    vec3 viewdir = normalize(u_campos.xyz - v_position);
    vec3 R = reflect(-viewdir, normal);

    vec3 F0 = vec3_splat(0.04); // dielectric specular
    F0 = mix(F0, base.rgb, metallic); // mix reflectivity with the albedo and metallic to produce our reflectivity value

    uint ztile = uint(max(log2(lineardepth(gl_FragCoord.z, znear, zfar)) * scale + bias, 0.0));
    uvec3 tiles = uvec3(uvec2(gl_FragCoord.xy / tilesizes[3]), ztile);
    uint tileidx = uint(tiles.x + tilesizes.x + tiles.y + (tilesizes.x * tilesizes.y) * tiles.z);

    vec3 radiance = vec3(0.0);

    float shadow = 0.0;
    radiance = calculatedirectional(normal, viewdir, base.rgb, roughness, metallic, F0, shadow);

    vec3 ambient = vec3_splat(0.025) * base.rgb;

    // IBL: This should basically just be a fallback from SSR in the event that SSR cannot accurately display reflections
    vec3 ks = fresnelschlickrough(max(dot(normal, viewdir), 0.0), F0, roughness);
    vec3 kd = 1.0 - ks;
    kd *= 1.0 - metallic;
    vec3 irradiance = textureCubeLod(s_env_irradiance, normal, 0).rgb;
    vec3 diffuse = irradiance * base.rgb;

    // TODO: We should be using LODs on prefilter based on distance from viewer
    const float MAX_CUBEMAP_LOD = 4.0; // maximum cubemap mip LOD (difference between highest)
    const float MIN_CUBEMAP_LOD = 0.0;
    vec3 prefiltered = textureCubeLod(s_env_prefiltered, R, MIN_CUBEMAP_LOD + roughness * MAX_CUBEMAP_LOD).rgb;
    vec2 bdrf = texture2D(s_brdflut, vec2(max(dot(normal, viewdir), 0.0), roughness)).rg;
    vec3 specular = prefiltered * (ks * bdrf.x + bdrf.y);
    ambient = (kd * diffuse + specular);
    ambient *= occlusion;

    radiance += ambient;
    radiance += emissive;

    // show slices:
    // gl_FragColor = vec4(colours[uint(mod(ztile, 8.0))], 1.0);
    // else:
    gl_FragColor = vec4(radiance, base.a);
}

/* vim: set sw=4 ts=4 expandtab: */
