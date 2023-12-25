$input v_position, v_texcoord0, v_tbn, v_lposition

#include <bgfx_shader.sh>
#include "../shaderlib.sh"

// TODO: Consider PBR lighting model for realism. (Ideal because that way I don't need to worry about making textures look convincing in every scenario and rather can focus on simply using ones that work)

SAMPLER2D(s_texture_diffuse, 0); // declare a sampler
SAMPLER2D(s_texture_specular, 1);
SAMPLER2D(s_texture_normal, 2);
SAMPLER2D(s_depthmap, 3);
uniform vec4 u_campos;

// data structure:
//
// DIRECTIONAL:
// 0: direction
// 1: ambient
// 2: diffuse
// 3: specular
//
// POINT:
// 0: position
// 1: ambient
// 2: diffuse
// 3: specular
//
// SPOT:
// 0: position
// 1: direction
// 2: cut off
// 3: ambient
// 4: diffuse
// 5: specular
#define MAX_LIGHTS 32
uniform vec4 u_light[6 * MAX_LIGHTS]; // average aligned
uniform vec4 u_lightcounts; // 0: directional, 1: point, 2: spotlight

vec3 nonsrgb(vec3 light) {
    // this is actually needed because bgfx sets backbuffers to sRGB against my will (meaning colours would be gamma corrected twice)
    return toLinear(light);
}

float shadowcalculation(vec3 lightdir, vec3 normal, vec4 lpos) {
    vec3 projcoords = lpos.xyz / lpos.w;
    projcoords = projcoords * 0.5 + 0.5;
    float closest = texture2D(s_depthmap, projcoords.xy).r;
    float current = projcoords.z;
    // float bias = max(0.05 * (1.0 - dot(normal, lightdir)), 0.005); // calculate a bias to prevent shadow acne
    float shadow = current > closest ? 1.0 : 0.0;
    // vec2 texelsize = 1.0 / textureSize(s_depthmap, 0);
    // for (int x = -1; x <= 1; ++x) {
    //     for (int y = -1; y <= 1; ++y) {
    //         float pcfdepth = texture2D(s_depthmap, projcoords.xy + vec2(x, y) * texelsize).r;
    //         shadow += current - bias > pcfdepth ? 1.0 : 0.0;
    //     }
    // }
    // shadow /= 9.0;
    //
    if (projcoords.z > 1.0) {
        shadow = 0.0;
    }

    return shadow;
}

vec3 calculatedirectional(int light, vec3 normal, mat3 tbn, vec3 viewdir, vec2 coords, vec4 lpos) {
    vec3 lightdir = normalize(mul(-u_light[light + 0].xyz, tbn));
    vec3 halfwaydir = normalize(lightdir + viewdir);
    float diff = max(dot(normal, lightdir), 0.0);
    vec3 reflectdir = reflect(-lightdir, normal);
    float spec = pow(max(dot(normal, halfwaydir), 0.0), 32); // TODO: Make material
    vec3 ambient = u_light[light + 1].rgb * nonsrgb(texture2D(s_texture_diffuse, coords).rgb);
    vec3 diffuse = u_light[light + 2].rgb * diff * nonsrgb(texture2D(s_texture_diffuse, coords).rgb);
    vec3 specular = u_light[light + 3].rgb * spec * nonsrgb(texture2D(s_texture_specular, coords).rgb);
    float shadow = shadowcalculation(lightdir, normal, lpos);
    return (ambient + diffuse + specular); // apply shadow (colour when lit is full and otherwise it is the colour of the ambient lighting)
}

vec3 calculatepoint(int light, vec3 normal, mat3 tbn, vec3 viewdir, vec2 coords, vec3 pos, vec4 lpos) {
    vec3 lightdir = normalize(u_light[light + 0].xyz - pos);
    vec3 halfwaydir = normalize(lightdir + viewdir);
    float diff = max(dot(normal, lightdir), 0.0);
    vec3 reflectdir = reflect(-lightdir, normal);
    float spec = pow(max(dot(normal, halfwaydir), 0.0), 32); // TODO: Make material
    float distance = length(u_light[light + 0].xyz - pos);
    float attenuation = 1.0 / (distance * distance);
    vec3 ambient = u_light[light + 1].rgb * nonsrgb(texture2D(s_texture_diffuse, coords).rgb);
    vec3 diffuse = u_light[light + 2].rgb * diff * nonsrgb(texture2D(s_texture_diffuse, coords).rgb);
    vec3 specular = u_light[light + 3].rgb * spec * nonsrgb(texture2D(s_texture_specular, coords).rgb);
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return vec3(ambient + diffuse + specular);
}

vec3 calculatespot(int light, vec3 normal, mat3 tbn, vec3 viewdir, vec2 coords, vec3 pos, vec4 lpos) {
    vec3 lightdir = normalize(u_light[light + 0].xyz - pos);
    vec3 halfwaydir = normalize(lightdir + viewdir);
    float diff = max(dot(normal, lightdir), 0.0);
    vec3 reflectdir = reflect(-lightdir, normal);
    float spec = pow(max(dot(normal, halfwaydir), 0.0), 32); // TODO: Make material
    float theta = dot(lightdir, normalize(-u_light[light + 1].xyz));
    float epsilon = u_light[light + 2].x - u_light[light + 2].y;
    float intensity = clamp((theta - u_light[light + 2].y) / epsilon, 0.0, 1.0);
    vec3 ambient = u_light[light + 3].rgb * nonsrgb(texture2D(s_texture_diffuse, coords).rgb);
    vec3 diffuse = u_light[light + 4].rgb * diff * nonsrgb(texture2D(s_texture_diffuse, coords).rgb);
    vec3 specular = u_light[light + 5].rgb * spec * nonsrgb(texture2D(s_texture_specular, coords).rgb);
    diffuse *= intensity;
    specular *= intensity;
    float distance = length(u_light[light + 0].xyz - pos);
    float attenuation = 1.0 / (distance * distance);
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return vec3(ambient + diffuse + specular);
}

// TODO: Better specular attenuation to prevent specular highlights at full force when they should not exist

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    // vec3 normal = normalize(mul(v_tbn, 2.0f * texture2D(s_texture_normal0, v_texcoord0).rgb - 1.0f));
    vec3 normal = texture2D(s_texture_normal, v_texcoord0).rgb;
    normal = normal * 2.0 - 1.0;
    normal = normalize(mul(v_tbn, normal));
    // vec3 viewdir = mul(v_tbn, normalize(u_campos.xyz - v_position));
    vec3 viewdir = normalize(u_campos.xyz - v_position);

    vec3 result = vec3(0.0); // total colour vector

    int off = 0;
    for (int i = 0; i < u_lightcounts.x; i++) {
        result += calculatedirectional(off, normal, v_tbn, viewdir, v_texcoord0, v_lposition);
        off += 6;
    }

    for (int i = 0; i < u_lightcounts.y; i++) {
        result += calculatepoint(off, normal, v_tbn, viewdir, v_texcoord0, v_position, v_lposition);
        off += 6;
    }

    for (int i = 0; i < u_lightcounts.z; i++) {
        result += calculatespot(off, normal, v_tbn, viewdir, v_texcoord0, v_position, v_lposition);
        off += 6;
    }

    // white noise dithering (eliminates colour banding at the expense of noisey output)
    // result.r += (rand(gl_FragCoord.xy) - 0.01) / 255;
    // result.g += (rand(gl_FragCoord.xy) - 0.01) / 255;
    // result.b += (rand(gl_FragCoord.xy) - 0.01) / 255;

    gl_FragColor = vec4(result, 1.0); // final result colour of pixel is dictated by ambient, diffuse and specular lighting of all lighting components
  }
