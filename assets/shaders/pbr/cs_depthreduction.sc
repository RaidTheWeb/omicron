
#include <bgfx_compute.sh>
#include "../shaderlib.sh"

#define GROUP_SIZE 64
#define THREADS_X 8
#define THREADS_Y 8


uniform vec4 u_params[1];
#define u_params0 u_params[0]

#define nearz u_params0.z
#define farz u_params0.w
uniform mat4 u_projection;

IMAGE2D_RO(s_depth, rg16f, 0);

IMAGE2D_WR(s_output, rg16f, 1);

SHARED vec2 depthshared[GROUP_SIZE];

NUM_THREADS(THREADS_X, THREADS_Y, 1)
void main() {
    ivec2 imagedim = ivec2(u_params0.xy);
    ivec2 sampleuv = min(imagedim - 1, ivec2(gl_GlobalInvocationID.xy));
    vec2 sampleddepth = imageLoad(s_depth, sampleuv).rg;
    if (sampleddepth.x == 0.0) {
        sampleddepth.x = 1.0;
    }

    depthshared[gl_LocalInvocationIndex] = sampleddepth;

    groupMemoryBarrier();

    UNROLL
    for (uint binidx = (GROUP_SIZE >> 1); binidx > 0; binidx >>= 1) {
        if (uint(gl_LocalInvocationIndex) < binidx) {
            depthshared[gl_LocalInvocationIndex].x = min(depthshared[gl_LocalInvocationIndex].x, depthshared[gl_LocalInvocationIndex + binidx].x);
            depthshared[gl_LocalInvocationIndex].y = max(depthshared[gl_LocalInvocationIndex].y, depthshared[gl_LocalInvocationIndex + binidx].y);
        }

        groupMemoryBarrier();
    }

    if (gl_LocalInvocationIndex == 0) {
        imageStore(s_output, ivec2(gl_WorkGroupID.xy), vec4(depthshared[0], 0.0, 0.0));
    }
}
