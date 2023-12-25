
#include <bgfx_compute.sh>

#define GROUP_SIZE 64
#define THREADS_X 8
#define THREADS_Y 8

uniform vec4 u_params[1];
#define u_params0 u_params[0]
#define nearz u_params0.z
#define farz u_params0.w
uniform mat4 u_projection;

SAMPLER2D(s_depth, 0);

IMAGE2D_WR(s_output, rg16f, 1);

SHARED vec2 depthshared[GROUP_SIZE];

NUM_THREADS(THREADS_X, THREADS_Y, 1)
void main() {
    vec2 sampleuv = vec2(gl_GlobalInvocationID.xy) / (u_params0.xy - 1.0);
    float sampleddepth = texture2DLod(s_depth, sampleuv, 0).r;
    if (sampleddepth < 1.0) {
#if BGFX_SHADER_LANGUAGE_GLSL
        sampleddepth = u_projection[3][2] / (sampleddepth - u_projection[2][2]);
#else
        sampleddepth = u_projection[2][3] / (sampleddepth - u_projection[2][2]);
#endif
        sampleddepth = saturate((sampleddepth - nearz) / (farz - nearz));
        depthshared[gl_LocalInvocationIndex] = vec2_splat(sampleddepth);
    } else {
        depthshared[gl_LocalInvocationIndex] = vec2(1.0, 1.0);
    }

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
