#include <bgfx_compute.sh>
#include "../shaderlib.sh"

#define GROUP_SIZE 256

// struct volumeaabb {
    // vec4 min;
    // vec4 max;
// };

BUFFER_WR(clustermin, vec4, 0);
BUFFER_WR(clustermax, vec4, 1);

uniform vec4 u_params[2];
#define u_params0 u_params[0]
#define u_params1 u_params[1]

#define znear u_params0.x
#define zfar u_params0.y
#define dim u_params0.zw
#define tilesizes u_params1

vec3 screentoview(mat4 invproj, vec4 screen, vec2 a) {
    vec2 texcoord = screen.xy / a.xy;
    float depth = toClipSpaceDepth(screen.z);

#if BGFX_SHADER_LANGUAGE_GLSL
    vec4 clip = vec4(texcoord * 2.0 - 1.0, depth, screen.w);
#else
    vec4 clip = vec4(vec2(texcoord.x, 1.0 - screen.y) * 2.0 - 1.0, depth, screen.w);
#endif

    return clipToWorld(invproj, clip.xyz);
}

vec3 lineintersecttozplane(vec3 A, vec3 B, float zdistance) {
    vec3 normal = vec3(0.0, 0.0, 1.0);

    vec3 ab = B - A;

    float t = (zdistance - dot(normal, A)) / dot(normal, ab);

    vec3 result = A + t * ab;

    return result;
}

NUM_THREADS(8, 8, 8)
void main() {
    const vec3 viewpos = vec3(0.0);
    const uvec3 workgroups = uvec3(u_params1.xyz);

    uint tilesize = uint(tilesizes[3]);
    uint tileidx = gl_WorkGroupID.x + gl_WorkGroupID.y * workgroups.x + gl_WorkGroupID.z * (workgroups.x * workgroups.y);

    vec4 maxpt = vec4(vec2(gl_WorkGroupID.x + 1, gl_WorkGroupID.y + 1) * tilesize, -1.0, 1.0);
    vec4 minpt = vec4(gl_WorkGroupID.xy * tilesize, -1.0, 1.0);

    vec3 vmaxpt = screentoview(u_invViewProj, maxpt, dim);
    vec3 vminpt = screentoview(u_invViewProj, minpt, dim);

    float tilenear = -znear * pow(zfar / znear, gl_WorkGroupID.z / float(workgroups.z));
    float tilefar = -znear * pow(zfar / znear, (gl_WorkGroupID.z + 1) / float(workgroups.z));

    vec3 minptnear = lineintersecttozplane(viewpos, vminpt, tilenear);
    vec3 minptfar = lineintersecttozplane(viewpos, vminpt, tilefar);
    vec3 maxptnear = lineintersecttozplane(viewpos, vmaxpt, tilenear);
    vec3 maxptfar = lineintersecttozplane(viewpos, vmaxpt, tilefar);

    vec3 minaabb = min(min(minptnear, minptfar), min(maxptnear, maxptfar));
    vec3 maxaabb = max(max(minptnear, minptfar), max(maxptnear, maxptfar));

    clustermin[tileidx] = vec4(minaabb, 0.0);
    clustermax[tileidx] = vec4(maxaabb, 0.0);
}
