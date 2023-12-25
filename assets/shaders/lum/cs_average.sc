#include <bgfx_compute.sh>

#define GROUP_SIZE 256

uniform vec4 u_avgparams;
#define u_params0 u_avgparams

IMAGE2D_RW(s_target, r16f, 0);
BUFFER_RW(histogram, uint, 1);

SHARED uint histogramShared[GROUP_SIZE];

float bintolum(float bin, float min, float range) {
    if (bin <= 1e-5) {
        return 0;
    }

    float t = saturate((bin - 1) / 254.0);
    return exp2(t * range + min);
}

NUM_THREADS(GROUP_SIZE, 1, 1)
void main() {
    uint count = histogram[gl_LocalInvocationIndex];
    histogramShared[gl_LocalInvocationIndex] = count * gl_LocalInvocationIndex;

    groupMemoryBarrier(); // wait until all threads reach this point
    barrier();

    histogram[gl_LocalInvocationIndex] = 0;

    UNROLL // mark that this loop should be unrolled (optimisation)
    for (uint cutoff = (GROUP_SIZE >> 1); cutoff > 0; cutoff >>= 1) {
        if (uint(gl_LocalInvocationIndex) < cutoff) {
            histogramShared[gl_LocalInvocationIndex] += histogramShared[gl_LocalInvocationIndex + cutoff];
        }

        groupMemoryBarrier(); // wait until threads done
        barrier();
    }

    if (gl_LocalInvocationIndex == 0) { // calculate on thread 0
        // TODO: adaptation is too flickery!
        float weightedaverage = (histogramShared[0] / max(u_params0.w - float(count), 1.0)) - 1.0;
        float weightedaveragelum = exp2(weightedaverage / 254.0 * u_params0.y + u_params0.x);
        float lumlast = imageLoad(s_target, ivec2(0, 0)).r; // load image luminescence of last frame (since this image is a 1x1 image, our texture coordinate UV will work at 0, 0)
        float adapted = lumlast + (weightedaveragelum - lumlast) * u_params0.z;
        // float adapted = mix(lumlast, (weightedaveragelum - lumlast), 0.5);
        // float adapted = mix(lumlast, 0.5 / weightedaveragelum * 0.5, 0.05);

        // HACK: sometimes luminescence is wacky and gives us 0 luminescence (which is not good)
        if (lumlast > 0.0) {
            imageStore(s_target, ivec2(0, 0), vec4(adapted, 0.0, 0.0, 0.0)); // store the adapted luminescence into the read value of our target
        } else {
            imageStore(s_target, ivec2(0, 0), vec4(0.1, 0.0, 0.0, 0.0));
        }
    }
    return;

    histogramShared[gl_LocalInvocationIndex] = histogram[gl_LocalInvocationIndex] * gl_LocalInvocationIndex;

    groupMemoryBarrier();
    barrier();

    UNROLL
    for (uint offset = GROUP_SIZE >> 1; offset > 0; offset >>= 1) {
        if (gl_LocalInvocationIndex < offset) {
            histogramShared[gl_LocalInvocationIndex] += histogramShared[gl_LocalInvocationIndex + offset];
        }
        groupMemoryBarrier();
        barrier();
    }

    if (gl_LocalInvocationIndex == 0) {
        float avgbin = histogramShared[0] / float(u_viewRect.z * u_viewRect.w);
        float avglum = bintolum(avgbin, u_params0.x, u_params0.y);
        // float avglum = bintolum(avgbin, -6, 5);
        // float avglum = 0.5;

        float lumlast = imageLoad(s_target, ivec2(0, 0)).r;
        // float lumlast = 0.1;

        float adapted = lumlast + (avglum - lumlast) * u_params0.z;
        // float adapted = mix(lumlast, (avglum - lumlast), u_params0.z);
        imageStore(s_target, ivec2(0, 0), vec4(adapted, 0.0, 0.0, 0.0));
        // imageStore(s_target, ivec2(0, 0), vec4_splat(0.1));
    }
}

/* vim: set sw=4 ts=4 expandtab: */
